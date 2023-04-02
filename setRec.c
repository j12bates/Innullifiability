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
    if (max < levels) return NULL;

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
// Returns 0 on success, or 1 if everything's already marked, -1 on
// memory error, -2 on input error
int sr_mark(const Base *base, const unsigned long *set, size_t setc)
{
    return 0;
}

// Output Sets with Particular Mark Status
// Returns number of sets on success, -1 on memory error
long long sr_query(const Base *base, QueryMode,
        void (*out)(const unsigned long *, size_t))
{
    return 0;
}

// ============ Helper Functions

// These functions are helper functions for the main user-level
// functions.

// M Choose N
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
