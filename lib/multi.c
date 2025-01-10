// ========================== MULTIPLE EXPAND ==========================

// Copyright (c) 2025, Jacob Bates
// SPDX-License-Identifier: BSD-2-Clause

#include <stdlib.h>
#include <stdbool.h>

#include <errno.h>

#include "multi.h"

// Helper Function Declarations
static void insert(unsigned long *, size_t, int,
        size_t, unsigned long,
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
    for (size_t i = 1; i < srcSize; i++)
        if (set[i] <= set[i - 1]) return -1;

    // Validate Sizes, Fixed Values
    if (destSize < srcSize) return -1;
    if (fixedc > 0 && fixedv[0] <= minM) return -1;
    for (size_t i = 1; i < fixedc; i++)
        if (fixedv[i] <= fixedv[i - 1]) return -1;
    errno = 0;
#endif

    // If no output, skip all this work
    if (out == NULL) return 0;

    // Total Values to Insert
    int inserts = destSize - srcSize;

    // First count until we reach a value in the Fixed Range
    size_t i = 0;
    for (; i < srcSize; i++) if (set[i] > maxM) break;

    // Countinue counting and verify that those fixed values are allowed
    size_t segIdx = i;
    for (size_t fixedIdx = 0; fixedIdx < fixedc; fixedIdx++) {
        if (i == srcSize) break;
        if (fixedv[fixedIdx] == set[i]) i++;
        else if (set[i] < fixedv[fixedIdx]) return 0;
    }
    if (i != srcSize) return 0;

    // Calculate how many fixed value insertions we're doing
    int insertsFixed = fixedc - (srcSize - segIdx);

    // Create the array for the superset and copy in the values for the
    // variable segment
    unsigned long *super = calloc(destSize, sizeof(unsigned long));
    if (super == NULL) return -1;
    for (size_t i = 0; i < segIdx; i++) super[i] = set[i];

    // If our max value in the variable segment is already in the
    // M-range, we can just do supersets of this alone
    if (segIdx > 0 && set[segIdx - 1] >= minM)
    {
        // Copy the values for the fixed segment
        for (size_t i = 0; i < fixedc; i++)
            super[segIdx + i] = fixedv[i];

        // Perform Insertions
        insert(super, destSize, inserts - insertsFixed, 0, maxM, out);
    }

    // Otherwise, we must manually insert M-values
    else
    {
        // Copy the values for the fixed segment, leaving one spot
        for (size_t i = 0; i < fixedc; i++)
            super[segIdx + i + 1] = fixedv[i];

        // Iterate through the values in the M-range
        for (unsigned long valM = minM; valM <= maxM; valM++)
        {
            // Manually Insert, Perform Insertions
            super[segIdx] = valM;
            insert(super, destSize, inserts - insertsFixed - 1,
                    0, valM - 1, out);
        }
    }

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
void insert(unsigned long *super, size_t size, int inserts,
        size_t idxStart, unsigned long valEnd,
        void (*out)(const unsigned long *, size_t))
{
    // If no more insertions, output complete set
    if (inserts == 0) out(super, size);
    else if (inserts < 0) return;

    // Shift our tail to the right so we have a spot to insert
    for (size_t i = size - inserts; i > idxStart; i--)
        super[i] = super[i - 1];

    // Insert values starting from the successor to what's on our left
    size_t idx = idxStart;
    unsigned long val = 1;
    if (idx > 0) val = super[idx - 1] + 1;

    // Try to insert every value up until our range ends
    for (; val <= valEnd - inserts + 1; val++)
    {
        // Insert Value
        super[idx] = val;

        // If we've reached the next index, skip and advance
        if (idx < size) if (super[idx + 1] == val) {
            idx++;
            continue;
        }

        // Recurse for another insertion, starting from the next value
        insert(super, size, inserts - 1, idx + 1, valEnd, out);
    }

    // Shift the remaining tail back to get rid of our insertion spot
    for (size_t i = idx; i < size - inserts; i++)
        super[i] = super[i + 1];

    return;
}
