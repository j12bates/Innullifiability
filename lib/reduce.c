// ============================== REDUCE ===============================

// Copyright (c) 2025, Jacob Bates
// SPDX-License-Identifier: BSD-2-Clause

#include <stdlib.h>

#include "reduce.h"

// Generate Set Reductions Outside a Range
// Return Values
// 0    - Completed or Exited
// 1    - Enumeration was Incomplete (due to wrap-around potential)
// -1   - Error

// Input mset must be in ascending order. First-order reductions that
// are outside the given M-range, and then any further reductions, are
// outputted along with the index of the replacement value. If the
// output function returns a nonzero value, this process will exit.
int reduceGeneral(const unsigned long *set, size_t size,
        unsigned long minM, unsigned long maxM,
        size_t repeat, unsigned long minOrig, size_t idxNew,
        int (*out)(const unsigned long *, size_t, size_t))
{
    bool incomplete = false;

    // Allocate Space for Reduction
    unsigned long *reduction = calloc(size - 1, sizeof(unsigned long));
    if (reduction == NULL) return -1;

    // Iterate through all the possible pairs of values
    for (size_t pairA = 0; pairA < size - 1; pairA++)
        for (size_t pairB = pairA + 1; pairB < size; pairB++)
    {
        // Get the values of that pair
        unsigned long a = set[pairA];
        unsigned long b = set[pairB];

        // If this pair is one we could've operated on last time, skip
        // it if it's comprised of a lesser value (only do parallel
        // operations in ascending order by smaller value)
        if (a < minOrig && pairA != idxNew && pairB != idxNew) continue;

        // Fill in the reduction with all the other values, leaving the
        // spot at the beginning to start inserting replacements
        size_t idx = 1;
        for (size_t i = 0; i < size; i++)
            if (i == pairA || i == pairB) continue;
            else reduction[idx++] = set[i];

        // This will be our list of replacement values for the pair
        unsigned long replv[4] = {0};
        bool div = a != 0 && b % a == 0;
        bool mult = a <= ULONG_MAX / b;
        incomplete |= !mult;
        size_t replc = 2 + div + mult;

        // Take care to produce an ordered list, and skip over any
        // illegal division
        if (div) {
            if (b < a * (b - a)) {      // (b / a) < (b - a)
                replv[0] = b / a;
                replv[1] = b - a;
            } else {                    // (b - a) < (b / a)
                replv[0] = b - a;
                replv[1] = b / a;
            } if (a > 1) {              // (a + b) < (a * b)
                replv[2] = a + b;
                replv[3] = a * b;
            } else {                    // (a * b) < (a + b)
                replv[2] = a * b;
                replv[3] = a + b;
            }
        } else {                        // same as above but no division
            replv[0] = b - a;
            if (a > 1) {
                replv[1] = a + b;
                replv[2] = a * b;
            } else {
                replv[1] = a * b;
                replv[2] = a + b;
            }
        }

        // Insert Each Replacement Value
        idx = 0;
        size_t i = 0;
        while (i < replc)
        {
            // Advance our insertion index if needed and try again
            if (idx + 1 < size - 1 && replv[i] > reduction[idx + 1]) {
                reduction[idx] = reduction[idx + 1];
                idx++;
            }

            // Otherwise, insert our replacement, and we have a
            // reduction
            else {
                reduction[idx] = replv[i];

                // If our M-value isn't outside the completed range,
                // skip this
                unsigned long M = reduction[size - 2];
                if (M >= minM && M <= maxM) continue;

                // Output/Recursion Calls
                int res;
                res = out(reduction, size - 1, idx);
                if (res) goto exit;
                if (repeat) {
                    res = recursiveTest(reduction, size - 1, 0, 0,
                        repeat - 1, a, idx);
                    if (res == 1) incomplete = true;
                    else if (res) goto exit;
                }

                // Advance to the next value
                i++;
            }
        }
    }

exit:
    free(reduction);
    return 0 + incomplete;
}

