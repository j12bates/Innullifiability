// ============================ SET RECORDS ============================

// This library controls an array that can hold data pertaining to sets,
// called a 'Set Record'. In essence, you initialize it with a set size
// and max value, and it creates an array where each byte represents a
// particular combination. The sets are in a lexicographic order sorted
// in ascending order by highest values. This is called 'Combinadics'
// (see 'Combinatorial Number System' on the English Wikipedia). The
// sets themselves are represented with values in ascending order. The
// value zero is unused, so sets start from (1, 2, 3, 4).

// The bytes are like bit-fields for each set, and different bits can be
// OR'd on by using the 'Mark' function. The Mark function takes in the
// set to mark, as well as a bitmask. It will mark the bits given on the
// input set.

// Sets with their bit-fields set a certain way can be retrieved using
// the 'Query' function. It takes in two parameters for the bit-field
// criteria: a bitmask and a bit-field. The function will scan the
// record and output all sets for which the bits set in the bitmask are
// set according to the bit-field. It outputs by way of a function
// pointer.

// The library is completely thread-safe (as far as I can tell), as it
// uses atomic characters as bit-fields. It provides a variant of the
// Query function that allows for running it multiple times in parallel
// whilst retaining full scan coverage for performing quick
// multithreaded operations. It does this by skipping N sets (N being
// the number of concurrent calls) each iteration, which I found to be
// faster than splitting the query space up into N segments.

#include <stdlib.h>
#include <stdbool.h>
#include <stdatomic.h>

#include "setRec.h"

// Individual Set Record Type
typedef _Atomic char Rec;

// Set Record Information Structure
typedef struct Base Base;
struct Base {
    Rec *rec;
    size_t size;
    unsigned long mval_min;
    unsigned long mval_max;
};

// Strings
const char *headerFormat = "setRec -- N = %lu, M = %lu\n";
const char *headerMsg = "Data begins 4K (4096) into the file\n";

// Helper Function Declarations
static int mark(Rec *, unsigned long,
        const unsigned long *, size_t, char);
static ssize_t query(const Rec *,
        unsigned long, unsigned long, size_t,
        size_t, size_t, char, char,
        void (*)(const unsigned long *, size_t));

static void incSetValues(unsigned long *, size_t, size_t);
static void indexToSet(unsigned long *, size_t, size_t);
static size_t setToIndex(const unsigned long *, size_t);
static unsigned long long mcn(size_t, size_t);

// ============ User-Level Functions

// These functions are for the main program to interact with, and they
// make reference to proper values and the base information structure,
// abstracting nodes away from the user level. They make use of the
// helper functions, which are defined later on.

// Initialize a Set Record
// Returns NULL on memory or input error
Base *sr_initialize(size_t size, unsigned long max)
{
    // We can't have more elements than possible values
    if (max < size) return NULL;

    // Allocate Memory for Information Structure
    Base *base = malloc(sizeof(Base));
    if (base == NULL) return NULL;

    // Populate Information Structure
    base->size = size;
    base->mval_min = size;
    base->mval_max = max;

    // Allocate Memory for Record Array
    Rec *rec = calloc(mcn(max, size), sizeof(Rec));
    if (rec == NULL) return NULL;
    base->rec = rec;

    return base;
}

// Release a Set Record
void sr_release(Base *base)
{
    // Free the Record Array
    free(base->rec);

    // Free the Information Structure itself
    free(base);

    return;
}

// Mark a Certain Set
// Returns 1 if newly marked, 0 if already marked, -1 on memory error,
// -2 on input error

// The input must be a valid set, in increasing order, with all values
// less than the max.
int sr_mark(const Base *base, const unsigned long *set, size_t setc,
        char mask)
{
    // Exit if Null Pointer
    if (base == NULL) return -1;

    // Check if set is valid: values must be increasing, and between 1
    // and M, M-values in bounds, and size must be N
    if (setc != base->size) return -2;
    if (set[0] < 1 || set[0] > base->mval_max) return -2;
    if (set[setc - 1] < base->mval_min) return -2;
    for (size_t i = 1; i < setc; i++)
    {
        if (set[i] <= set[i - 1]) return -2;
        if (set[i] > base->mval_max) return -2;
    }

    // Mark this set on the record
    int res = mark(base->rec, base->mval_min, set, setc, mask);

    return res;
}

// Output Sets with Particular Mark Status
// Returns number of sets on success, -1 on memory error
ssize_t sr_query(const Base *base, char mask, char bits,
        void (*out)(const unsigned long *, size_t))
{
    // Exit if Null Pointer
    if (base == NULL) return -1;

    // Output Sets that Match Query
    ssize_t res = query(base->rec,
            base->mval_min, base->mval_max, base->size,
            0, 1, mask, bits, out);

    return res;
}

// Output Sets with Particular Mark Status, for Parallelism
// Returns number of sets on success, -1 on memory error, -2 on input
// error

// The mod is a number less than the number of concurrent calls. Each
// call should give a different value for that parameter, to ensure full
// coverage.
ssize_t sr_query_parallel(const Base *base, char mask, char bits,
        size_t concurrents, size_t mod,
        void (*out)(const unsigned long *, size_t))
{
    // Exit if Null Pointer
    if (base == NULL) return -1;

    // Check if parallelism is valid
    if (mod >= concurrents) return -2;

    // Output Sets that Match Query
    ssize_t res = query(base->rec,
            base->mval_min, base->mval_max, base->size,
            mod, concurrents, mask, bits, out);

    return res;
}

// Import Record from Binary File
// Returns 0 on success, -1 on error (read errno), -2 on wrong size, -3
// on invalid file
int sr_import(const SR_Base *base, FILE *restrict f)
{
    int res;

    // Read and interpret the header
    res = fseek(f, 0, SEEK_SET);
    if (res < 0) return -1;

    size_t size;
    unsigned long max;
    res = fscanf(f, headerFormat, &size, &max);
    if (res == EOF && ferror(f)) return -1;
    else if (res != 2) return -3;

    // Exit if record is wrong size
    if (size != base->size || max != base->mval_max) return -2;

    // Number of Sets (elements in array)
    size_t total = mcn(base->mval_max, base->size);

    // Raw array is one block into the file
    res = fseek(f, 0x1000, SEEK_SET);
    if (res < 0) return -1;

    // Attempt to read entire array
    size_t written = fread(base->rec, sizeof(Rec), total, f);
    if (written != total) {
        if (feof(f)) return -3;
        else return -1;
    }

    return 0;
}

// Export Record to Binary File
// Returns 0 on success, -1 on error (read errno)
int sr_export(const SR_Base *base, FILE *restrict f)
{
    int res;

    // Write an info header at the start of the file
    res = fseek(f, 0, SEEK_SET);
    if (res < 0) return -1;

    res = fprintf(f, headerFormat, base->size, base->mval_max);
    if (res < 0) return -1;

    res = fprintf(f, headerMsg);
    if (res < 0) return -1;

    // Number of Sets (elements in array)
    size_t total = mcn(base->mval_max, base->size);

    // Write raw array one block into the file
    res = fseek(f, 0x1000, SEEK_SET);
    if (res < 0) return -1;

    size_t written = fwrite(base->rec, sizeof(Rec), total, f);
    if (written != total) return -1;

    return 0;
}

// ============ Helper Functions

// These functions are helper functions for the main user-level
// functions.

// Mark a Set
// Returns 1 if newly marked (new bits set), 0 if already marked

// This function marks a particular set in the record, OR'ing the given
// bits. Assumes given set is in ascending order, the right size, and in
// the right range of values.
int mark(Rec *rec, unsigned long minm,
        const unsigned long *set, size_t setc, char mask)
{
    // Get the address
    size_t index = setToIndex(set, setc) - mcn(minm - 1, setc);

    // OR the bits we care about
    char prev = atomic_fetch_or(rec + index, mask);

    // If they weren't already set, return 1, else 0
    return (prev & mask) != mask;
}

// Compute Index from Set
// Returns the index, no error checking
size_t setToIndex(const unsigned long *set, size_t setc)
{
    size_t index = 0;

    // Go from most significant (highest) to least
    for (size_t vals = setc; vals > 0; vals--)
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
void indexToSet(unsigned long *set, size_t setc, size_t index)
{
    // Go from most significant (highest) to least
    for (size_t vals = setc; vals > 0; vals--)
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

// Iteratively Check Records and Output Sets
// Returns number of sets on success, -1 on memory error

// This is a function which looks across an entire record. It iterates
// across all the sets, keeping a pointer to the current entry in the
// record as well as an array of values that corresponds to that set.
// If a set matches the bit-field criteria provided, it will be output
// to the given function.

// The function also can be configured for running in parallel. It gives
// options for splitting the record into segments and/or querying every
// Nth element.
ssize_t query(const Rec *rec,
        unsigned long minm, unsigned long maxm, size_t size,
        size_t offset, size_t skip, char mask, char bits,
        void (*out)(const unsigned long *, size_t))
{
    // Counter for Number of Sets
    ssize_t setc = 0;

    // Initialize a Set
    unsigned long *values = calloc(size, sizeof(unsigned long));
    if (values == NULL) return -1;

    // First set with the min M-value
    indexToSet(values, size, 0);
    values[size - 1] = minm;

    // Find the Segment Bounds
    size_t total = mcn(maxm, size) - mcn(minm - 1, size);
    // size_t begin = total * <seg> / <divs>;
    // size_t end = total * (<seg> + 1) / <divs>;

    // Starting Point
    incSetValues(values, size, offset);

    // Loop over every Nth set, checking and outputting
    for (size_t i = offset; i < total; i += skip)
    {
        // Whether this set is a match
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
            if (out != NULL) out(values, size);
            setc++;
        }

        // Advance to Next Nth Set
        incSetValues(values, size, skip);
    }

    // Deallocate Memory
    free(values);

    return setc;
}

// Increment Set Value Array

// This is a helper function for the query function, and it takes an
// array of set values and advances it N sets lexicographically. If the
// operation can be done simply by increasing the first value, it'll do
// so, and otherwise it'll increment the next value, using a loop to
// deal with chains of overflowing place values. It'll repeat this
// process until it's able to settle the first value.
void incSetValues(unsigned long *set, size_t setc, size_t add)
{
    // Handle Trivial Size Cases
    if (setc == 0);
    else if (setc == 1) set[0] += add;

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
        for (i = 1; i < setc; i++)
        {
            // Reset previous value
            set[i - 1] = i;

            // If we can increment this value, do so
            if (i == setc - 1) ++set[i];
            else if (++set[i] < set[i + 1]) break;
        }

        // Account for additional set from increment
        add -= avail + 1;
    }

    return;
}

// M Choose N
// Returns 0 on error
unsigned long long mcn(size_t m, size_t n)
{
    // Invalid Case
    if (m < n) return 0;

    // Total Ordered Combinations, Permutations
    unsigned long long total = 1, perms = 1;
    for (size_t i = 0; i < n; i++)
    {
        total *= m - i;
        perms *= i + 1;
    }

    // Don't divide by zero
    if (perms == 0) return 0;

    return total / perms;
}
