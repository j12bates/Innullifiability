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
// This function will insert values into a set to generate a range of
// supersets of a certain order. We start at a given index in the set
// and end on a particular value. So we start by shifting the tail end
// of the set rightwards, so we can insert values. We scan across all
// our values, giving each one a chance. We advance when needed. At the
// end we shift any remaining tail back to where it started, so we have
// the original set back. This process is done recursively for higher-
// order supersets.
void insert(unsigned long *super, size_t size, size_t subcalls,
        unsigned long idxStart, size_t valEnd,
        void (*out)(const unsigned long *, size_t))
{
    // Shift our tail to the right so we have a spot to insert
    for (size_t i = size - subcalls - 1; i > idxStart; i--)
        super[i] = super[i - 1];

    // Insert values starting from the successor to what's on our left
    size_t idx = idxStart;
    unsigned long val = 1;
    if (idx > 0) val = super[idx - 1] + 1;

    // Try to insert every value up until our range ends
    for (; val <= valEnd - subcalls; val++)
    {
        // Insert Value
        super[idx] = val;

        // If we've reached the next index, skip and advance
        if (idx < size) if (super[idx + 1] == val) {
            idx++;
            continue;
        }

        // Either recurse or output the complete superset
        if (subcalls > 0) insert(super, size, subcalls - 1,
            idx + 1, valEnd, out);
        else out(super, size);
    }

    // Shift the remaining tail back to get rid of our insertion spot
    for (size_t i = idx; i < size - subcalls - 1; i++)
        super[i] = super[i + 1];

    return;
}
