// ============================ SET RECORDS ============================

// This library controls an array that can hold data pertaining to sets.

#include <stdlib.h>
#include <stdbool.h>

#include "setRec.h"

// Individual Set Record Structure
typedef struct Rec Rec;
struct Rec {
    char bits;
} __attribute__((__packed__));

// Set Record Information Structure
typedef struct Base Base;
struct Base {
    Rec *rec;
    size_t size;
    unsigned long max;
};

// Other Typedefs
typedef enum SR_QueryMode QueryMode;

// Helper Function Declarations
static unsigned long long mcn(size_t, size_t);
static long long mark(Rec *, unsigned long, size_t,
        const unsigned long *, size_t,
        char, char, unsigned long, size_t);
static long long query(const Rec *, unsigned long, size_t,
        unsigned long *, size_t, char, char,
        void (*)(const unsigned long *, size_t));

static long long queryIterative(const Rec *, unsigned long, size_t,
        size_t, size_t, char, char,
        void (*)(const unsigned long *, size_t));
static int incSetValues(unsigned long *, size_t, unsigned long, size_t);

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
int sr_mark(const Base *base, const unsigned long *set, size_t setc,
        char mask, char bits)
{
    // Exit if Null Pointer
    if (base == NULL) return -1;

    // Check if set is valid: values must be increasing, and between 1
    // and M, and size must not be greater than N
    if (setc > base->size) return -2;
    if (set[0] < 1) return -2;
    if (set[0] > base->max) return -2;
    for (size_t i = 1; i < setc; i++)
    {
        if (set[i] <= set[i - 1]) return -2;
        if (set[i] > base->max) return -2;
    }

    // Mark Records with Set as Constraining Values
    long long res = mark(base->rec, base->max, base->size,
            set, setc, mask, bits, 1, 0);

    if (res == -1) return -1;
    return res > 0 ? 1 : 0;
}

// Output Sets with Particular Mark Status
// Returns number of sets on success, -1 on memory error
long long sr_query(const Base *base, char mask, char bits,
        void (*out)(const unsigned long *, size_t))
{
    // Exit if Null Pointer
    if (base == NULL) return -1;

    // Allocate Space for Values
    unsigned long *values = calloc(base->size, sizeof(unsigned long));
    if (values == NULL) return -1;

    // Output Sets that Match Query
    long long res = query(base->rec, base->max, base->size,
            values, 0, mask, bits, out);

    // Deallocate Memory
    free(values);

    return res;
}

// ============ Helper Functions

// These functions are helper functions for the main user-level
// functions.

// Recursively Mark Sets with Constraining Values
// Returns number of sets newly marked on success, -1 on memory error

// This is a recursive function for marking records that match a set of
// constraining values. The function uses two counters, one for value
// and one for position, to keep track of where it is within the whole
// record. The function recurses on the record pointer when it wants to
// use the value at the position, and otherwise moves the record pointer
// beyond all those sets. The function must use the value if it's the
// next constraint. But otherwise, it must be sure to account for
// potential intermediary values between constraints, so it will also
// split itself up when it's able to use the value as an intermediary,
// and by default move on to the next value at the position.
long long mark(Rec *rec, unsigned long max, size_t size,
        const unsigned long *constr, size_t constrc,
        char mask, char bits, unsigned long value, size_t position)
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
            // If the bits we care about aren't already set correctly,
            // count this
            if ((rec[i].bits & mask) != (bits & mask)) setc++;

            // Set the bits we care about to the desired state
            rec[i].bits &= ~mask;
            rec[i].bits |= (bits & mask);
        }
    }

    // If we're at the next constraint, we must mark these sets, as the
    // constraint can't come up again
    else if (value == *constr)
    {
        // Recurse, advancing to the next position and the next
        // constraint
        long long res = mark(rec, max, size, constr + 1, constrc - 1,
                mask, bits, value + 1, position + 1);

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
            long long res = mark(rec, max, size, constr, constrc,
                    mask, bits, value + 1, position + 1);

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
            long long res = mark(rec, max, size, constr, constrc,
                    mask, bits, value + 1, position);

            // Pass along an error and otherwise keep count
            if (res == -1) return -1;
            setc += res;
        }
    }

    return setc;
}

// Iteratively Check Records and Output Sets
// Returns number of sets on success, -1 on memory error
long long queryIterative(const Rec *rec, unsigned long max, size_t size,
        size_t mod, size_t parallels, char mask, char bits,
        void (*out)(const unsigned long *, size_t))
{
    // Exit if Null Pointer
    if (rec == NULL) return -1;

    // Counter for Number of Sets
    long long setc = 0;

    // Initialize a Set with Lowest Values
    unsigned long *values = calloc(size, sizeof(unsigned long));
    for (size_t i = 0; i < size; i++) values[i] = i + 1;

    // Starting Point
    rec += mod;
    int res = incSetValues(values, size, max, mod);
    if (res == 1) return setc;

    // Loop over All Sets

    return setc;
}

// Increment Set Value Array
// Returns 0 on success, 1 on overflow
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

// Recursively Check Records and Output Sets
// Returns number of sets on success, -1 on memory error

// This is a recursive function for going through every record. It uses
// an array to keep track of the values it's working with, and a
// position counter to keep track of position within the set. It goes
// through all the possible values at that position, recursing as it
// does so, being sure to advance the record pointer beyond all records
// covered. If a call gives values for a complete set, it will compare
// the record against the settings provided, and output the set if there
// is a match.

// Normally, a set will be deemed a match if, in the record, ALL the
// bits specified by the bitmask match those bits in the provided
// settings. However, if the bitmask is set to all zeros, then the
// settings will be interpreted as a bitmask, and a set will be deemed a
// match if ANY of the bits specified by it are set in the record.
// Except, if that new bitmask is also all zeros, all sets will be
// deemed a match.
long long query(const Rec *rec, unsigned long max, size_t size,
        unsigned long *values, size_t position, char mask, char bits,
        void (*out)(const unsigned long *, size_t))
{
    // Exit if Null Pointer
    if (rec == NULL) return -1;

    // Number of Sets Output
    long long setc = 0;

    // If we're at the level of a complete set, check the record
    if (position == size)
    {
        // Whether this set is a match
        bool match = false;

        // Specific Bitmask Case: if the bits in the bitmask are all set
        // according to the settings
        if (mask != 0) match = (rec->bits & mask) == (bits & mask);

        // Zero Bitmask Case: treat the settings as the bitmask; match
        // if any of the bits in the bitmask are set, or if we have the
        // wildcard bitmask of all zeros
        else match = (rec->bits & bits) != 0 || bits == 0;

        // If we have a match, output and keep count
        if (match)
        {
            if (out != NULL) out(values, size);
            setc++;
        }
    }

    // Otherwise, go through all the possible values for this position
    else
    {
        // Minimum: 1 if we're just starting, and one above previous
        // value otherwise
        unsigned long lmin = 1;
        if (position != 0) lmin = values[position - 1] + 1;

        // Maximum: increments towards global max as we approach the
        // final position
        size_t remaining = size - position - 1;
        unsigned long lmax = max - remaining;

        // Iterate over all these values
        for (unsigned long i = lmin; i <= lmax; i++)
        {
            // Append the next value to our set values
            values[position] = i;

            // Recurse, pass on any error, and add to our counter
            long long res = query(rec, max, size,
                    values, position + 1, mask, bits, out);
            if (res == -1) return -1;
            setc += res;

            // Advance beyond the sets that call covered, which can be
            // expressed as a number of combinations
            rec += mcn(max - i, remaining);
        }
    }

    return setc;
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
