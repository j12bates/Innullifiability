// ========================== MULTIPLE EXPAND ==========================

// Copyright (c) 2025, Jacob Bates
// SPDX-License-Identifier: BSD-2-Clause

#include <stdlib.h>
#include <stdbool.h>

// Helper Function Declarations
void insert(unsigned long *, size_t, size_t,
        unsigned long, unsigned long,
        void (*)(const unsigned long *, size_t));

// Produce Expansions of a Set to a Specific Size
int multiExpand(const unsigned long *set, size_t srcSize,
        unsigned long minM, unsigned long maxM,
        size_t fixedc, const unsigned long *fixedv,
        size_t destSize, void (*out)(const unsigned long *, size_t))
{
#ifndef NO_VALIDATE
    // Validate Input Set: values are positive and ascending
    errno = EINVAL;
    if (set[0] < 1) return -1;
    for (size_t i = 1; i < size; i++)
        if (set[i] <= set[i - 1]) return -1;

    // Validate Sizes, Fixed Values
    if (destSize > srcSize) return -1;
    if (fixedc > 0) if (fixedv[0] <= minM) return -1;
    for (size_t i = 1; i < fixedc; i++)
        if (fixedv[i] <= fixedv[i - 1]) return -1;
    errno = 0;
#endif

    // If no output, skip all this work
    if (out == NULL) return 0;

    // Number of Values to Insert
    size_t inserts = destSize - srcSize;

    // We're gonna look at the fixed values segment
    // First count until we reach those values
    size_t i = 0;
    for (; i < srcSize; i++) if (set[i] >= fixedv[0]) break;
    size_t insertsFixed = fixedc - (srcSize - i);

    // Then verify all the values up there are actually allowed
    for (size_t fixedIdx = 0; fixedIdx < fixedc; fixedIdx++) {
        if (fixed[fixedIdx] == set[i]) i++;
        else if (set[i] < fixed[fixedIdx]) return 0;
        if (i == srcSize) break;
    }
    if (i != srcSize) return 0;

    // we can't remove values so there is nothing
    if (insertsFixed > inserts) return 0;

    // now we just create an array with all the fixed values at the top.
    // for each allowed M-value, call a function that does all possible
    // insertions on the non-M variable segment, outputting.

    return 0;
}

// Insert Values into a Set
// hoping this one can be fully in-place, without the need to allocate
// more memory on each subcall, that'd be quite a lot. so i may need
// extra params like indices, may not need M-values
void insert(unsigned long *super, size_t size, size_t subcalls,
        unsigned long minM, unsigned long maxM,
        void (*out)(const unsigned long *, size_t))
{
    return;
}
