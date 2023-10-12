// ============================== EXPAND ===============================

// Copyright (c) 2023, Jacob Bates
// SPDX-License-Identifier: BSD-2-Clause

// This program is for expanding sets, the inverse operation to 'merging
// and reducing,' in order to find all the sets that could reduce
// immediately to whatever set is input, based on the given rules. A set
// is input, as well as some configuration, and the expansions are
// output through a function pointer.

#include <stdlib.h>

// Helper Function Declarations
int supers(const unsigned long *, size_t, unsigned long,
        void (*out)(const unsigned long *, size_t));

// Produce All Set Expansions
// Returns 0 on success, -1 on error (check errno)
int expand(const unsigned long *set, size_t size,
        unsigned long max, int mode,
        void (*out)(const unsigned long *, size_t))
{
#ifndef NO_VALIDATE
    // Validate Input Set: values are ascending and in-range
    errno = EINVAL;
    if (set[0] < 1) return -1;
    for (size_t i = 1; i < size; i++)
        if (set[i] <= set[i - 1]) return -1;
    if (set[setc - 1] > max) return -1;
    errno = 0;
#endif

    return 0;
}

// ============ Helper Functions

// Enumerate Supersets
// Returns 0 on success, -1 on error (check errno)
int supers(const unsigned long *set, size_t size, unsigned long max,
        void (*out)(const unsigned long *, size_t))
{
    // Set Representation
    unsigned long *super = calloc(size + 1, sizeof(unsigned long));
    if (super == NULL) return -1;

    // Initialize with the input, leaving a spot for insertion
    for (size_t i = 0; i < size; i++) super[i + 1] = set[i];

    // Iterate over values to insert, keeping track of index
    size_t pos = 0;
    for (size_t i = 1; i <= max; i++)
    {
        // Insert Value
        super[pos] = i;

        // If we've reached the next value, skip and advance
        if (pos < size) if (super[pos + 1] == i) {
            pos++;
            continue;
        }

        // Otherwise, we have a superset to output
        if (out != NULL) out(super, size + 1);
    }

    free(super);

    return 0;
}
