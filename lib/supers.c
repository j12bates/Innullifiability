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

// This function iteratively expands a set by one element, giving all of
// a set's supersets. It takes in a set, which must be in ascending
// order and contain no repetitions. All possible values will be
// inserted, and each resulting superset will be passed into the output
// function.
int supers(const unsigned long *set, size_t setc,
        void (*out)(const unsigned long *, size_t))
{
    // Validate input set: values must be ascending and in-range
    for (size_t i = 1; i < setc; i++)
        if (set[i - 1] >= set[i]) return -2;
    if (set[setc - 1] > maxValue) return -2;

    // Set representation for expansions
    unsigned long *super = calloc(setc + 1, sizeof(unsigned long));
    if (super == NULL) return -1;

    // Initialize with input set
    for (size_t i = 0; i < setc; i++)
        super[i + 1] = set[i];

    // Iterate over all values to insert, keeping track of insertion
    // index
    size_t pos = 0;
    for (unsigned long i = 1; i <= maxValue; i++)
    {
        // Insert Value
        super[pos] = i;

        // If we've reached the next value, move ahead of it so we stay
        // in ascending order, skip since we have a duplicate
        if (pos < setc) if (super[pos + 1] == i) {
            pos++;
            continue;
        }

        // Otherwise, we have a superset and we'll output it
        if (out != NULL) out(super, setc + 1);
    }

    free(super);

    return 0;
}
