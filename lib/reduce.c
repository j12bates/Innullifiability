// ============================== REDUCE ===============================

// Copyright (c) 2025, Jacob Bates
// SPDX-License-Identifier: BSD-2-Clause

#include <stdbool.h>
#include <stdlib.h>

#include <limits.h>

#include "reduce.h"

// Helper Function Declarations
int reduceGeneral(const unsigned long *, size_t,
        unsigned long, unsigned long, bool,
        size_t, unsigned long, size_t,
        int (*)(const unsigned long *, size_t, size_t));

// Produce Set Reductions
int reduce(const unsigned long *set, size_t srcSize,
        unsigned long minM, unsigned long maxM, bool inRange,
        int mode, size_t destSize,
        int (*out)(const unsigned long *, size_t, size_t))
{
    return reduceGeneral(set, srcSize, minM, maxM, inRange,
            srcSize - destSize - 1, 0, srcSize, out);
}

// Recursively Generate Subsets, Relative to a Range First-Order

// bro what? why did i make this recursive??? this should be done
// iteratively and have the range apply at the end. range constraints on
// the first step only makes sense for reductions as we do further
// reductions without care, if that maeks sense
int subsGeneral(const unsigned long *set, size_t size,
        unsigned long minM, unsigned long maxM, bool inRange,
        size_t repeat,
        int (*out)(const unsigned long *, size_t, size_t))
{
    // Allocate Space for Reduction
    unsigned long *reduction = calloc(size - 1, sizeof(unsigned long));
    if (reduction == NULL) return -1;

    // If the max value is ineligible, we can only remove it
    unsigned long a_n = set[size - 1];
    if ((a_n >= minM && a_n <= maxM) != inRange) goto greatest;

    // The first subset will be missing the first value
    for (size_t i = 1; i < size; i++) reduction[i - 1] = set[i];
    unsigned long missing = set[0];

    // Iteratively take out values and place them back to create the
    // other subsets
    for (size_t i = 0; i < size - 1; i++) {
        int res = out(reduction, size - 1, size - 1);
        if (res) goto exit;
        if (repeat) {
            res = subsGeneral(reduction, size - 1, 0, 0, false,
                    repeat - 1, out);
            if (res) goto error;
        }
        unsigned long temp = reduction[i];
        reduction[i] = missing;
        missing = temp;
    }

    // For removing the max value, check the new max to ensure it's
    // eligible
greatest:
    unsigned long a_pre_n = reduction[size - 2];
    if ((a_pre_n >= minM && a_pre_n <= maxM) == inRange) {
        int res = out(reduction, size - 1, size - 1);
        if (repeat && !res) {
            res = subsGeneral(reduction, size - 1, 0, 0, false,
                    repeat - 1, out);
            if (res) goto error;
        }
    }

exit:
    free(reduction);
    return 0;

error:
    free(reduction);
    return -1;
}

// Recursively Generate Set Reductions, Relative to a Range First-Order
// Return Values
// 0    - Completed or Exited
// 1    - Enumeration was Incomplete (due to wrap-around potential)
// -1   - Error

// Input mset must be in ascending order. First-order reductions that
// are eligible with the given M-range (inside or outside, as
// specified) are outputted along with the indices of the replacement
// values. If specified, further reductions of those sets will also be
// outputted by the same means. If the output function returns a nonzero
// value at any time, this process will exit.

// The process is optimized in a way to avoid outputting some multiple-
// reductions more than once: if successive reduction step operations
// are each using elements from the set given initially, they could
// theoretically take place in any order, so for this procedure we
// constrain them to take place in ascending order by smaller value.

// Due to limitations with integer storage, some multiplication
// operations given sufficiently large sets cannot be performed, and so
// these sets are not computed, but the process will return a special
// code to indicate the omission. For nullifiability purposes, this is
// of little concern as there would have to be two really high products
// somehow being close enough.
int reduceGeneral(const unsigned long *set, size_t size,
        unsigned long minM, unsigned long maxM, bool inRange,
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

                // If our M-value isn't relative to the range the way
                // we want, skip this
                unsigned long M = reduction[size - 2];
                if ((M >= minM && M <= maxM) != inRange) continue;

                // Output/Recursion Calls
                int res;
                res = out(reduction, size - 1, idx);
                if (res) goto exit;
                if (repeat) {
                    res = reduceGeneral(reduction, size - 1, 0, 0,
                            false, repeat - 1, a, idx, out);
                    if (res == 1) incomplete = true;
                    else if (res) goto error;
                }

                // Advance to the next value
                i++;
            }
        }
    }

exit:
    free(reduction);
    return 0 + incomplete;

error:
    free(reduction);
    return -1;
}

