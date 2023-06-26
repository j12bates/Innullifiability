// ============================ SET RECORDS ============================

// This library controls an array that can hold data pertaining to sets,
// called a 'Set Record'. In essence, you initialize it with a set size
// and max value, and it creates an array where each byte represents a
// particular combination. The sets are in lexicographic order, with
// lowest-numbered sets first (e.g. (1, 2, 3, 4)) and highest-numbered
// sets last.

// The bytes are like bit-fields for each set, and different bits can be
// OR'd on by using the 'mark' function. The mark function takes in a
// combination, which can be of any size smaller than or equal to the
// record's set size, as the pattern of sets to mark. It will mark all
// the supersets of the input pattern with the bits given.

// Sets with their bit-fields set a certain way can be retrieved using
// the 'query' function. It takes in two parameters for the bit-field
// criteria: a bitmask and a bit-field. The function will scan the
// record and output all sets for which the bits set in the bitmask are
// set according to the bit-field. It outputs by way of a function
// pointer.

// The library is completely thread-safe (as far as I can tell), as it
// uses atomic characters as bit-fields. It provides a variant of the
// query function that allows for running it multiple times in parallel
// whilst retaining full scan coverage for performing quick
// multithreaded operations. It does this by skipping N sets (N being
// the number of concurrent calls) each iteration, which I found to be
// the fastest in terms of memory-access speed.

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
    unsigned long max;
};

// Other Typedefs
typedef enum SR_QueryMode QueryMode;

// Strings
const char *headerFormat = "setRec -- N = %lu, M = %lu\n";
const char *headerMsg = "Data begins 4K (4096) into the file\n";

// Helper Function Declarations
static unsigned long long mcn(size_t, size_t);

static ssize_t mark(Rec *, unsigned long, size_t,
        const unsigned long *, size_t,
        char, unsigned long, size_t);
static ssize_t mark_it(Rec *, unsigned long, size_t,
        const unsigned long *, size_t, char);
static ssize_t query(const Rec *, unsigned long, size_t,
        size_t, size_t, size_t, size_t,
        char, char,
        void (*)(const unsigned long *, size_t));
static int incSetValues(unsigned long *, size_t, unsigned long,
        size_t);
static int incSetValues_b(unsigned long *, size_t, unsigned long,
        size_t);
static size_t setToIndex(const unsigned long *, size_t, unsigned long);
static size_t setToIndex_b(const unsigned long *, size_t);

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
    base->max = max;

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

// Mark a Certain Set and Supersets
// Returns 0 on success, or 1 if at least 1 newly marked set, -1 on
// memory error, -2 on input error

// The input combination must be in increasing order, and the size must
// be less than or equal to that of the record's sets. One thing to
// remember is that the return value may be inconsistent if it is run in
// parallel with other calls that mark the same bits, as it all depends
// on which call gets to a set first.
int sr_mark(const Base *base, const unsigned long *set, size_t setc,
        char mask)
{
    // Exit if Null Pointer
    if (base == NULL) return -1;

    // Check if set is valid: values must be increasing, and between 1
    // and M, and size must not be greater than N
    if (setc > base->size) return -2;
    if (set[0] < 1 || set[0] > base->max) return -2;
    for (size_t i = 1; i < setc; i++)
    {
        if (set[i] <= set[i - 1]) return -2;
        if (set[i] > base->max) return -2;
    }

    // Mark Records with Set as Constraining Values
    ssize_t res = mark_it(base->rec, base->max, base->size,
            set, setc, mask);

    if (res == -1) return -1;
    return res > 0 ? 1 : 0;
}

// Output Sets with Particular Mark Status
// Returns number of sets on success, -1 on memory error
ssize_t sr_query(const Base *base, char mask, char bits,
        void (*out)(const unsigned long *, size_t))
{
    // Exit if Null Pointer
    if (base == NULL) return -1;

    // Output Sets that Match Query
    ssize_t res = query(base->rec, base->max, base->size,
            0, 1, 0, 1, mask, bits, out);

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
    ssize_t res = query(base->rec, base->max, base->size,
            mod, concurrents, 0, 1, mask, bits, out);

    return res;
}

// Import Record from Binary FIle
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
    if (size != base->size || max != base->max) return -2;

    // Number of Sets (elements in array)
    size_t total = mcn(base->max, base->size);

    // Raw array is one block into the file
    res = fseek(f, 0x1000, SEEK_SET);
    if (res < 0) return -1;

    // Attempt to read entire array
    size_t written = fread(base->rec, sizeof(Rec), total, f);
    if (written != total)
    {
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

    res = fprintf(f, headerFormat, base->size, base->max);
    if (res < 0) return -1;

    res = fprintf(f, headerMsg);
    if (res < 0) return -1;

    // Number of Sets (elements in array)
    size_t total = mcn(base->max, base->size);

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

// Recursively Mark Sets with Constraining Values
// Returns number of sets newly marked on success, -1 on memory error

// This is a recursive function for marking supersets of a set of
// constraining values. These values are assumed to be in ascending
// order, as this is how sets are ordered within the record. The
// function uses two counters, one for value and one for position, to
// keep track of where it is within the whole record. The function
// recurses on the record pointer when it wants to use the value at the
// position, and otherwise moves the record pointer beyond all those
// sets. The function must use the value if it's the next constraint.
// But otherwise, it must be sure to account for potential intermediary
// values between constraints, so it will also split itself up when it's
// able to use the value as an intermediary, and by default move on to
// the next value at the position.
ssize_t mark(Rec *rec, unsigned long max, size_t size,
        const unsigned long *constr, size_t constrc,
        char mask, unsigned long value, size_t position)
{
    // Exit if Null Pointer
    if (rec == NULL) return -1;

    // Counter for Newly Marked Sets
    long long setc = 0;

    // If we've met all the constraints, mark the records and exit
    if (constrc == 0)
    {
        // Number of sets to mark expressed as combinations
        size_t toMark = mcn(max - value + 1, size - position);

        // Mark all these sets
        for (size_t i = 0; i < toMark; i++)
        {
            // OR the bits we care about
            char prev = atomic_fetch_or(rec + i, mask);

            // If they weren't already set, count this
            if ((prev & mask) != mask) setc++;
        }
    }

    // If we're at the next constraint, we must mark these sets, as the
    // constraint can't come up again
    else if (value == *constr)
    {
        // Recurse, advancing to the next position and the next
        // constraint
        ssize_t res = mark(rec, max, size, constr + 1, constrc - 1,
                mask, value + 1, position + 1);

        // Pass along an error and otherwise keep count
        if (res == -1) return -1;
        setc += res;
    }

    // Otherwise, we aren't going to advance to the next constraint this
    // time
    else
    {
        // If we have positions spare, we'll have sets that use this
        // value here anyways, so split the call up and mark all those
        // sets too
        if (position + constrc < size)
        {
            // Recurse, advancing to the next position
            ssize_t res = mark(rec, max, size, constr, constrc,
                    mask, value + 1, position + 1);

            // Pass along an error and otherwise keep count
            if (res == -1) return -1;
            setc += res;
        }

        // Either way, advance to the next value, unless we can't
        if (value < max)
        {
            // Advance beyond the sets with this value at this position,
            // which can be expressed as a number of combinations
            rec += mcn(max - value, size - position - 1);

            // Simple recurse without advancing position
            ssize_t res = mark(rec, max, size, constr, constrc,
                    mask, value + 1, position);

            // Pass along an error and otherwise keep count
            if (res == -1) return -1;
            setc += res;
        }
    }

    return setc;
}

// Iteratively Mark Supersets
// Returns number of sets newly marked on success, -1 on memory error

// This function marks all the supersets of a set of constraining
// values. It works iteratively, inserting all possible values into the
// set, then recursing to either expand further or mark the individual
// set by way of the set address function.
ssize_t mark_it(Rec *rec, unsigned long max, size_t size,
        const unsigned long *constr, size_t constrc, char mask)
{
    // Counter for newly marked sets
    ssize_t setc = 0;

    // If too many constraints, break
    if (constrc > size) return -1;

    // If our constraints make up a complete set, mark record
    if (constrc == size)
    {
        // Get the address
        size_t index = setToIndex_b(constr, constrc);

        // OR the bits we care about
        char prev = atomic_fetch_or(rec + index, mask);

        // If they weren't already set, count this
        if ((prev & mask) != mask) setc++;

        return setc;
    }

    // Array for Supersets
    unsigned long *super = calloc(constrc + 1, sizeof(unsigned long));
    if (super == NULL) return -1;

    // Copy Constraining Values Over
    for (size_t i = 0; i < constrc; i++)
        super[i + 1] = constr[i];

    // Iterate over all Values to Insert
    size_t pos = 0;
    for (unsigned long i = 1; i <= max; i++)
    {
        // Insert Value
        super[pos] = i;

        // If we've reached the next value, move ahead of it so we stay
        // in ascending order
        if (pos < constrc) if (super[pos + 1] == i) {
            pos++;
            continue;
        }

        // Otherwise, we have a superset: recurse to mark or expand
        // further
        ssize_t res = mark_it(rec, max, size,
                super, constrc + 1, mask);

        // Pass along an error and otherwise keep count
        if (res == -1) {
            setc = -1;
            break;
        }
        setc += res;
    }

    // Deallocate Memory
    free(super);

    return setc;
}

// Compute Index of Set
// Returns the index, no error checking
size_t setToIndex(const unsigned long *set, size_t setc,
        unsigned long max)
{
    size_t index = 0;
    unsigned long value = 0;

    // Consider each position in the set
    for (size_t pos = 0; pos < setc; pos++)
    {
        // Iterate over all the values skipped from the previous
        // position
        for (value++; value < set[pos]; value++)
        {
            // All the values to the right of our position, add in all
            // the possibilities as a number of combinations
            index += mcn(max - value, setc - pos - 1);
        }
    }

    return index;
}

// Compute Index from Set -- Highest-Value Combinatorics Ordering
// Returns the index, no error checking
size_t setToIndex_b(const unsigned long *set, size_t setc)
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

// Compute Set from Index -- Highest-Value Combinatorics Ordering
void indexToSet_b(unsigned long *set, size_t setc, size_t index)
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
ssize_t query(const Rec *rec, unsigned long max, size_t size,
        size_t offset, size_t skip, size_t seg, size_t divs,
        char mask, char bits,
        void (*out)(const unsigned long *, size_t))
{
    // Exit if Null Pointer
    if (rec == NULL) return -1;

    // Exit if parallel parameters don't make sense
    if (seg >= divs || offset >= skip) return -1;

    // Counter for Number of Sets
    ssize_t setc = 0;

    // Initialize a Set with Lowest Values
    unsigned long *values = calloc(size, sizeof(unsigned long));
    if (values == NULL) return -1;
    for (size_t i = 0; i < size; i++) values[i] = i + 1;

    // Find the Segment Bounds
    size_t total = mcn(max, size);
    size_t segBegin = total * seg / divs;
    size_t segEnd = total * (seg + 1) / divs;

    // Starting Point
    size_t start = segBegin + offset;
    int res = incSetValues_b(values, size, max, start);

    // Loop over every Nth set, checking and outputting
    for (size_t i = start; i < segEnd && res == 0; i += skip)
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
        if (match)
        {
            if (out != NULL) out(values, size);
            setc++;
        }

        // Advance to Next Nth Set
        res = incSetValues_b(values, size, max, skip);
    }

    return setc;
}

// Increment Set Value Array
// Returns 0 on success, 1 on overflow

// This is a helper function for the query function, and it takes an
// array of set values and advances it N sets lexicographically. It
// works by increasing the final value by N if permitted by M, and
// otherwise it'll increase up to an overflow into the previous value
// before recursing to increase by the remainder. It's used to keep
// track of the current set values as the query function advances.
int incSetValues(unsigned long *set, size_t setc, unsigned long max,
        size_t add)
{
    // Handle Complete Overflow
    if (setc > max) return 1;
    if (setc == 0) return 1;

    // Final Value in the Set, Available Increase
    unsigned long final = set[setc - 1];
    if (final > max) return 1;
    unsigned long avail = max - final;

    // Base Case: Simply Increase Final Value if Possible
    if (add <= avail)
        set[setc - 1] = final + add;

    // Otherwise, increase as far as possible plus one, replacing the
    // previous digit
    else
    {
        // Increment Previous Value
        int res = incSetValues(set, setc - 1, max - 1, 1);
        if (res != 0) return res;

        // Set Final Appropriately
        set[setc - 1] = set[setc - 2] + 1;

        // Increase by whatever remains
        return incSetValues(set, setc, max, add - avail - 1);
    }

    return 0;
}

// Increment Set Value Array -- Highest-Value Combinatics Ordering
// Returns 0 on success, 1 on overflow

// This is a helper function for the query function, and it takes an
// array of set values and advances it N sets lexicographically. If the
// operation can be done simply by increasing the first value, it'll do
// so, and otherwise it'll increment the next value, using a loop to
// deal with chains of overflowing place values. It'll repeat this
// process until it's able to settle the first value.
int incSetValues_b(unsigned long *set, size_t setc, unsigned long max,
        size_t add)
{
    // Handle Complete Overflow
    if (setc > max) return 1;
    if (setc == 0) return 1;

    // Repeat until we're done increasing
    while (add > 0)
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
            unsigned long next = i == setc - 1 ? max + 1 : set[i + 1];
            if (++set[i] < next) break;
        }

        // Handle overflow, account for additional set from increment
        if (i == setc) return 1;
        add -= avail + 1;
    }

    return 0;
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
