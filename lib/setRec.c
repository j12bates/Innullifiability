// ============================ SET RECORDS ============================

// Copyright (c) 2023, Jacob Bates
// SPDX-License-Identifier: BSD-2-Clause

// This library controls an array that can hold data pertaining to sets,
// called a 'Set Record'. In essence, it provides a single byte for
// every possible combination of N positive integers (i.e. 1 or greater)
// within the range of sets allocated. An element corresponding to a set
// can be directly addressed using its value representation, an array of
// the integers in the set, strictly in ascending order (e.g. {2, 5, 6,
// 14}). This means the last value in the representation is the largest,
// and it's called the M-value. Sets can be addressed to have particular
// bits on their corresponding byte-sized bit-field set ('Mark'), and
// then the entire record can be scanned to return back the original set
// representations ('Query').

// When a Set Record is initialized, a small data structure is created.
// This structure is opaque to the user, and it's used by the library to
// hold parameters about the record and point to the array. A pointer to
// this structure is passed into library functions to identify the
// Record.

// A Record is initialized with a Set Size, which defines how many
// numbers are in the set representations the Record accepts. After
// initialization, a record can be Allocated any number of times to
// create the actual array. This can be done with the Allocate function,
// which creates an empty Record, or by Importing an existing Record
// from a file. The range of sets represented is determined upon
// Allocation.

// The range works by essentially splitting up set representations into
// two 'segments': there's the Variable segment, which are lower values
// that are the ones actually changing throughout the record, and
// there's the Fixed segment, which stores some upper values that remain
// static throughout. The M-range controlled through the min/max M-value
// parameters set in Allocation or Importing actually corresponds to the
// M-value of the Variable segment, not the full sets themselves. The
// Fixed segment can be thought of some high values just tacked on the
// end, not actually making any difference in the backend, outside of
// the set representations. The maximum size for the Fixed segment is
// four values.

// The bytes are like bit-fields for each set, and different bits can be
// OR'd on by using the 'Mark' function. The Mark function takes in the
// representation of the set to mark, as well as a bitmask. It will mark
// the bits given on the input set's corresponding element.

// Sets with their bit-fields set a certain way can be retrieved using
// the 'Query' function. It takes in two parameters for the bit-field
// criteria: a bitmask and a bit-field. The function will scan the
// record and output all sets for which the bits in the bitmask are set
// according to the bit-field. It outputs set representations by way of
// a function pointer.

// The sets are in a lexicographic order sorted highest values. This is
// called 'Combinadics' (see 'Combinatorial Number System' on the
// English Wikipedia). It means that the sets are essentially grouped
// together by M-value, least to greatest. Here's an example of how this
// would look using length-3 set representations:
//      1   2   3
//      1   2   4
//      1   3   4
//      2   3   4
//      1   2   5
//      ...

// The library is completely thread-safe (as far as I can tell), as it
// uses atomic characters as bit-fields. It provides a variant of the
// Query function that allows for running it multiple times in parallel
// whilst retaining full scan coverage for performing quick
// multithreaded operations. It does this by skipping N sets (N being
// the number of concurrent calls) each iteration, which I found to be
// faster than splitting the query space up into N segments.

#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <errno.h>

#include "setRec.h"

#define FIXED_MAX 4
#define PERIOD 0x1000

// Individual Set Record Type
typedef _Atomic char Rec;

// Set Record Information Structure
typedef struct Base Base;
struct Base {
    Rec *rec;
    size_t size;            // real size of sets (variable and fixed)
    size_t varSize;         // size up to the M-value (ignoring fixed)
    unsigned long mval_min;
    unsigned long mval_max;
    size_t fixedSize;       // number of fixed values
    unsigned long fixedv[FIXED_MAX];
};

// Output Function
typedef void OutFun(const unsigned long *, size_t, char);

// Header Format Strings
const char *hdrFmtFull =
        "Full Set -- Size: %lu\n";
const char *hdrFmtVar =
        "Variable Segment -- Size: %lu, "
        "M-Value Range: %lu to %lu\n";
const char *hdrFmtFixed =
        "Fixed Segment -- Size: %lu, "
        "Values: %lu, %lu, %lu, %lu\n";     // matches FIXED_MAX
const char *hdrMsgData =
        "Data begins 4K (4096) into the file\n";

// Macros for Calculating Size of Record
#define TOTAL(minm, maxm, varSize) \
            (mcn(maxm, varSize) - mcn(minm - 1, varSize))
#define TOTAL_B(base) \
            TOTAL(base->mval_min, base->mval_max, base->varSize)

// Helper Function Declarations
static int mark(Rec *, unsigned long,
        const unsigned long *, size_t, char);
static ssize_t query(const Rec *,
        unsigned long, unsigned long, size_t,
        const unsigned long *, size_t,
        size_t, size_t, char, char,
        size_t *, size_t, OutFun *);

static void incSetValues(unsigned long *, size_t, size_t);
static void indexToSet(unsigned long *, size_t, size_t);
static size_t setToIndex(const unsigned long *, size_t);
static unsigned long long mcn(size_t, size_t);

// ============ User-Level Functions

// These functions are for the main program to interact with, and they
// make reference to proper values and the base information structure.
// They make use of the helper functions, which are defined later on.
// They should be the only functions that do input validation.

// Initialize a Set Record
// Returns NULL on error (read errno)

// Creates the information structure used by this library to access the
// record. Instantiated with the Set Size, which is immutable, and the
// record is empty.
Base *sr_initialize(size_t size)
{
    // Allocate Information Structure
    Base *base = malloc(sizeof(Base));
    if (base == NULL) return NULL;

    // Populate to indicate empty
    base->rec = NULL;
    base->size = size;
    base->varSize = size;
    base->mval_min = 1; // avoid uflow when decrementing for total calc
    base->mval_max = 0;
    base->fixedSize = 0;

    return base;
}

// Allocate a Set Record
// Returns 0 on success, -1 on error (read errno)

// Allocates a specific M-Value range to a Set Record. Min can be set to
// any low number safely to include every set up to Max. On error,
// record is preserved.
int sr_alloc(Base *base,
        size_t varSize, unsigned long minm, unsigned long maxm,
        size_t fixedSize, const unsigned long *fixedv)
{
    // Adjust input values if necessary
    if (minm < varSize) minm = varSize;
    if (maxm < minm) maxm = minm - 1;

    // Validate Size and Fixed Values
    errno = EINVAL;
    if (fixedSize > FIXED_MAX) return -1;
    if (varSize + fixedSize != base->size) return -1;
    if (fixedSize > 0) if (fixedv[0] <= maxm) return -1;
    for (size_t i = 1; i < fixedSize; i++)
        if (fixedv[i] <= fixedv[i - 1]) return -1;
    errno = 0;

    // Populate Information Structure
    base->varSize = varSize;
    base->mval_min = minm;
    base->mval_max = maxm;
    base->fixedSize = fixedSize;
    for (size_t i = 0; i < fixedSize; i++)
        base->fixedv[i] = fixedv[i];

    // Allocate Memory for Record Array
    Rec *rec = calloc(TOTAL_B(base), sizeof(Rec));
    if (rec == NULL) return -1;

    // Deallocate existing array and replace it
    free(base->rec);
    base->rec = rec;

    return 0;
}

// Release a Set Record

// Deallocates the record and its information structure. Record is
// destroyed.
void sr_release(Base *base)
{
    // Free the array, then the information structure
    free(base->rec);
    free(base);

    return;
}

// Get Property: Set Size
size_t sr_getSize(const Base *base)
{
    return base->size;
}

// Get Property: Variable Segment Size
size_t sr_getVarSize(const Base *base)
{
    return base->varSize;
}

// Get Property: Minimum M-Value
unsigned long sr_getMinM(const Base *base)
{
    return base->mval_min;
}

// Get Property: Maximum M-Value
unsigned long sr_getMaxM(const Base *base)
{
    return base->mval_max;
}

// Get Property: Fixed Segment Size
size_t sr_getFixedSize(const Base *base)
{
    return base->fixedSize;
}

// Get Property: Particular Fixed Segment Value
unsigned long sr_getFixedValue(const Base *base, size_t fixed)
{
    if (fixed < base->fixedSize) return base->fixedv[fixed];
    else return 0;
}

// Get Property: Total Addressable Sets
size_t sr_getTotal(const Base *base)
{
    return TOTAL_B(base);
}

// Mark a Certain Set
// Returns 1 if newly marked, 0 if already marked or unallocated, -1 on
// error (read errno)

// ORs on the given bits on the specified set in the record, thus
// 'Marking' that set. The input must be a valid set, in increasing
// order, within the record's allocated M-Value Range.
int sr_mark(const Base *base, const unsigned long *set, size_t size,
        char mask)
{
#ifndef NO_VALIDATE
    // Validate input set: values must be positive and ascending, and
    // size must be N
    errno = EINVAL;
    if (size != base->size) return -1;
    if (set[0] < 1) return -1;
    for (size_t i = 1; i < size; i++)
        if (set[i] <= set[i - 1]) return -1;
    errno = 0;
#endif

    // Skip if set M-value is out of range
    size_t varSize = base->varSize;
    if (set[varSize - 1] > base->mval_max
            || set[varSize - 1] < base->mval_min) return 0;

    // Skip if fixed values don't match
    for (size_t i = 0; i < base->fixedSize; i++)
        if (set[varSize + i] != base->fixedv[i]) return 0;

    // Mark this set on the record
    int res = mark(base->rec, base->mval_min, set, varSize, mask);

    return res;
}

// Output Sets with Particular Mark Status
// Returns number of sets on success, -1 on error (read errno)

// Scans the entire record, outputting raw sets that are marked in it
// according to the given bit settings. Progress reference and output
// function can be set to NULL if not desired.
ssize_t sr_query(const Base *base, char mask, char bits,
        size_t *prog, OutFun *out)
{
    // Output Sets that Match Query
    ssize_t res = query(base->rec,
            base->mval_min, base->mval_max, base->varSize,
            base->fixedv, base->fixedSize,
            0, 1, mask, bits, prog, PERIOD, out);

    return res;
}

// Output Sets with Particular Mark Status, for Parallelism
// Returns number of sets on success, -1 on error (read errno)

// Same as above, but for parallelism. The mod is a number less than the
// number of concurrent calls. Each call should give a different value
// for that parameter, giving full coverage.
ssize_t sr_query_parallel(const Base *base, char mask, char bits,
        size_t concurrents, size_t mod,
        size_t *prog, OutFun *out)
{
#ifndef NO_VALIDATE
    // Validate Parallelism
    errno = EINVAL;
    if (mod >= concurrents) return -1;
    errno = 0;
#endif

    // Output Sets that Match Query
    ssize_t res = query(base->rec,
            base->mval_min, base->mval_max, base->varSize,
            base->fixedv, base->fixedSize,
            mod, concurrents, mask, bits, prog, PERIOD, out);

    return res;
}

// Import Record from Binary File
// Returns 0 on success, -1 on error (read errno), -2 on wrong size, -3
// on invalid file

// Loads a record's data from a file into the record provided. File must
// be of matching set size.
int sr_import(Base *base, FILE *restrict f)
{
    int res;

    // Header follows the Reserved Space
    res = fseek(f, 0x0800, SEEK_SET);
    if (res < 0) return -1;

    // Read Numbers for Full Set
    size_t size;
    res = fscanf(f, hdrFmtFull, &size);
    if (res == EOF && ferror(f)) return -1;
    else if (res != 1) return -3;

    // Exit if record is wrong size
    if (size != base->size) return -2;

    // Read Numbers for Variable Segment
    size_t varSize;
    unsigned long minm, maxm;
    res = fscanf(f, hdrFmtVar, &varSize, &minm, &maxm);
    if (res == EOF && ferror(f)) return -1;
    else if (res != 3) return -3;

    // Read Numbers for Fixed Segment
    size_t fixedSize;
    unsigned long fixed[4];
    res = fscanf(f, hdrFmtFixed, &fixedSize,
            fixed, fixed + 1, fixed + 2, fixed + 3);
    if (res == EOF && ferror(f)) return -1;
    else if (res != 5) return -3;

    // Allocate New Array
    res = sr_alloc(base, varSize, minm, maxm, fixedSize, fixed);
    if (res == -1) {
        if (errno == EINVAL) return -3;
        else return -1;
    }

    // Raw array is one block into the file
    res = fseek(f, 0x1000, SEEK_SET);
    if (res < 0) return -1;

    // Attempt to read entire array
    size_t total = TOTAL_B(base);
    size_t written = fread(base->rec, sizeof(Rec), total, f);
    if (written != total) {
        if (ferror(f)) return -1;
        else return -3;
    }

    return 0;
}

// Export Record to Binary File
// Returns 0 on success, -1 on error (read errno)

// Writes a record's state to a data file, to be Imported later.
int sr_export(const Base *base, FILE *restrict f)
{
    int res;

    // Header follows the Reserved Space
    res = fseek(f, 0x0800, SEEK_SET);
    if (res < 0) return -1;

    // Write Header for Full Set
    res = fprintf(f, hdrFmtFull, base->size);
    if (res < 0) return -1;

    // Write Header for Variable Segment
    res = fprintf(f, hdrFmtVar, base->varSize,
            base->mval_min, base->mval_max);
    if (res < 0) return -1;

    // Write Header for Fixed Segment
    res = fprintf(f, hdrFmtFixed, base->fixedSize,
            base->fixedv[0], base->fixedv[1],
            base->fixedv[2], base->fixedv[3]);
    if (res < 0) return -1;

    // Write Header Message
    res = fprintf(f, hdrMsgData);
    if (res < 0) return -1;

    // Write entire raw array one block into the file
    res = fseek(f, 0x1000, SEEK_SET);
    if (res < 0) return -1;

    size_t total = TOTAL_B(base);
    size_t written = fwrite(base->rec, sizeof(Rec), total, f);
    if (written != total) return -1;

    return 0;
}

// ============ Helper Functions

// These functions are helper functions for the main user-level
// functions. They shouldn't have any input validation, they only need
// to do the calculations. They don't refer to information structures of
// user-space.

// Mark a Set
// Returns 1 if newly marked (new bits set), 0 if already marked

// This function marks a particular set in the record, OR'ing the given
// bits. Assumes given set is in ascending order, the right size, and in
// the right range of values. This function only deals with the variable
// portion of sets, as the fixed values have no bearing on anything.
int mark(Rec *rec, unsigned long minm,
        const unsigned long *set, size_t varSize, char mask)
{
    // OR the bits we care about
    size_t index = setToIndex(set, varSize) - mcn(minm - 1, varSize);
    char prev = atomic_fetch_or(rec + index, mask);

    // Whether they were already set
    return (prev & mask) != mask;
}

// Iteratively Check Records and Output Sets
// Returns number of sets on success, -1 on error (read errno)

// This is a function which looks across an entire record. It iterates
// across all the sets, keeping a pointer to the current entry in the
// record as well as an array of values that corresponds to that set.
// If a set matches the bit-field criteria provided, it will be output
// to the given function.

// The function also can be configured for running in parallel. It gives
// an option for querying every Nth element. Additionally, it can give
// periodic progress tracking by updating an object with the number of
// sets remaining.
ssize_t query(const Rec *rec,
        unsigned long minm, unsigned long maxm, size_t varSize,
        const unsigned long *fixedv, size_t fixedSize,
        size_t offset, size_t skip, char mask, char bits,
        size_t *progress, size_t period, OutFun *out)
{
    // Number of Sets
    ssize_t setc = 0;

    // The set representation we'll use
    size_t size = varSize + fixedSize;
    unsigned long *values = calloc(size, sizeof(unsigned long));
    if (values == NULL) return -1;

    // The representation of the first allocated set, including fixed
    // values, adjusted to our starting point
    indexToSet(values, varSize - 1, 0);
    values[varSize - 1] = minm;
    for (size_t i = 0; i < fixedSize; i++)
        values[varSize + i] = fixedv[i];
    incSetValues(values, size, offset);

    // Loop over every Nth set, checking, outputting, and updating
    // progress
    size_t total = TOTAL(minm, maxm, varSize);
    for (size_t i = offset; i < total; i += skip)
    {
        bool match = false;

        // Get current bits
        char cur = atomic_load(rec + i);

        // Specific Bitmask Case: if the bits in the bitmask are all set
        // according to the settings
        if (mask != 0) match = (cur & mask) == (bits & mask);

        // Zero Bitmask Case: treat the settings as the bitmask; match
        // if any of the bits in that bitmask are set, or if we have the
        // wildcard bitmask of all zeros
        else match = (cur & bits) != 0 || bits == 0;

        // If we have a match, output and keep count
        if (match) {
            if (out != NULL) out(values, size, cur);
            setc++;
        }

        // Advance to Next Nth Set
        incSetValues(values, varSize, skip);

        // Update Progress every so often
        if (progress != NULL) if (i / skip % period == 0)
            *progress = i / skip;
    }

    // Final progress update
    if (progress != NULL) *progress = (total - offset - 1) / skip + 1;

    free(values);

    return setc;
}

// Compute Index from Set
// Returns the index, no error checking
size_t setToIndex(const unsigned long *set, size_t varSize)
{
    size_t index = 0;

    // Go from most significant (highest) to least
    for (size_t vals = varSize; vals > 0; vals--)
    {
        // Get set value, decrement since we're not using zero, add
        // combinations to index
        size_t i = set[vals - 1] - 1;
        index += mcn(i, vals);
    }

    return index;
}

// Compute Set from Index
// Set is written into given array pointer
void indexToSet(unsigned long *set, size_t varSize, size_t index)
{
    // Go from most significant (highest) to least
    for (size_t vals = varSize; vals > 0; vals--)
    {
        // Find the last value for which the number of combinations is
        // within the remainder
        size_t i;
        for (i = vals - 1; mcn(i, vals) <= index; i++);
        i--;

        // Increment since we're not using zero, subtract combinations
        // for new remainder
        set[vals - 1] = i + 1;
        index -= mcn(i, vals);
    }

    return;
}

// Increment Set Value Array

// This is a helper function for the query function, and it takes an
// array of set values and advances it N sets lexicographically. If the
// operation can be done simply by increasing the first value, it'll do
// so, and otherwise it'll increment the next value, using a loop to
// deal with chains of overflowing place values. It'll repeat this
// process until it's able to settle the first value.
void incSetValues(unsigned long *set, size_t varSize, size_t add)
{
    // Handle Trivial Size Cases
    if (varSize == 0);
    else if (varSize == 1) set[0] += add;

    // Repeat until we're done increasing
    else while (add > 0)
    {
        // The furthest we can increase the first value
        unsigned long avail = set[1] - set[0] - 1;
        if (add <= avail) {
            set[0] += add;
            break;
        }

        // If there's more, increment the next value, dealing with the
        // further ones if necessary
        size_t i;
        for (i = 1; i < varSize; i++)
        {
            // Reset previous value
            set[i - 1] = i;

            // If we can increment this value, do so
            if (i == varSize - 1) ++set[i];
            else if (++set[i] < set[i + 1]) break;
        }

        // Account for additional set from increment
        add -= avail + 1;
    }

    return;
}

// M Choose N
unsigned long long mcn(size_t m, size_t n)
{
    // Zero Case
    if (m < n) return 0;

    // Total Ordered Combinations, Permutations
    unsigned long long total = 1, perms = 1;
    for (size_t i = 0; i < n; i++)
    {
        total *= m - i;
        perms *= i + 1;
    }

    // Don't divide by zero
    // (wait hold up, this isn't even possible is it? if we get an
    // overflow, bigger problems...)
    if (perms == 0) return 0;

    return total / perms;
}
