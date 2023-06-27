// ============================= SUPERSETS =============================

// This program will take in a set and expand it to supersets one
// element larger. It will output all such sets within a range of
// values.

#include <stdlib.h>

// Max-Value
static unsigned long maxValue;

// Configure Superset Maximum Value
// Returns 0 on success
int supersInit(unsigned long max)
{
    // Set Max Value
    maxValue = max;

    return 0;
}

// Expand Set into Supersets with One Added Element
// Returns 0 on success, -1 on memory error, -2 on input error
int supers(const unsigned long *set, size_t setc,
        void (*out)(const unsigned long *, size_t))
{
    return 0;
}
