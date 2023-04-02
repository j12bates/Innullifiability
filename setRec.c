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

// Array of Factorials
unsigned long long *fact = NULL;
size_t factc = 0;

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
    return NULL;
}

// Release a Set Record
void sr_release(Base *base)
{
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
