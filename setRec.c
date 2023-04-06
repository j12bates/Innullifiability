// ============================ SET RECORDS ============================

// This library controls an array that can hold data pertaining to sets.

#include <stdlib.h>
#include <stdbool.h>

#include "setRec.h"

// Individual Set Record Structure
typedef struct Rec Rec;
struct Rec {
    bool marked;
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
static long long query(const Rec *, unsigned long, size_t,
        unsigned long *, size_t, QueryMode,
        void (*)(const unsigned long *, size_t));

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

    // TODO REMOVE -- this is just a little test to see if indexing
    // works properly
    rec[6].marked = true;

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
// Returns 0 on success, or 1 if everything's already marked, -1 on
// memory error, -2 on input error
int sr_mark(const Base *base, const unsigned long *set, size_t setc)
{
    return 0;
}

// Output Sets with Particular Mark Status
// Returns number of sets on success, -1 on memory error
long long sr_query(const Base *base, QueryMode mode,
        void (*out)(const unsigned long *, size_t))
{
    // Exit if Null Pointer
    if (base == NULL) return -1;

    // Allocate Space for Values
    unsigned long *values = calloc(base->size, sizeof(unsigned long));
    if (values == NULL) return -1;

    // Output Sets that Match Query
    long long res = query(base->rec, base->max, base->size,
            values, 0, mode, out);

    // Deallocate Memory
    free(values);

    return res;
}

// ============ Helper Functions

// These functions are helper functions for the main user-level
// functions.

// Recursively Check Records and Output Sets
// Returns number of sets on success, -1 on memory error

// This is a recursive function for going through every record. It uses
// an array to keep track of the values it's working with, and a
// position counter to keep track of position within the set. It goes
// through all the possible values at that position, recursing as it
// does so, being sure to advance the record pointer beyond all records
// covered. If a call gives values for a complete set, it will check the
// mark status, and pass the set to the output function depending on the
// query mode.
long long query(const Rec *rec, unsigned long max, size_t size,
        unsigned long *values, size_t position, QueryMode mode,
        void (*out)(const unsigned long *, size_t))
{
    // Exit if Null Pointer
    if (rec == NULL) return -1;

    // Number of Sets Output
    long long setc = 0;

    // If we're at the level of a complete set, check the record
    if (position == size)
    {
        // If the mark status checks out, output
        if (rec->marked == (mode == QUERY_SETS_MARKED)
                || mode == QUERY_SETS_ALL)
        {
            out(values, size);
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
                    values, position + 1, mode, out);
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
    for (size_t i = 0; i < m - n; i++)
    {
        total *= m - i;
        perms *= i + 1;
    }

    // Don't divide by zero
    if (perms == 0) return 0;

    return total / perms;
}
