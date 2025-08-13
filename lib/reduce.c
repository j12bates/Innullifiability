// ============================== REDUCE ===============================

// Copyright (c) 2025, Jacob Bates
// SPDX-License-Identifier: BSD-2-Clause

#include <stdbool.h>
#include <stdlib.h>

#include <errno.h>
#include <limits.h>

#include "reduce.h"

// Helper Function Declarations
int remove(const unsigned long *, size_t,
        size_t, size_t,
        unsigned long *, size_t, size_t,
        int (*)(const unsigned long *, size_t));
int recursiveReduce(const unsigned long *, size_t,
        unsigned long, unsigned long, bool,
        size_t, unsigned long, size_t,
        int (*)(const unsigned long *, size_t, size_t));

// TODO: change `size' to `srcSize'

// Produce Subsets Relative to a Range
// Return Values
// 0    - Completed or Exited
// -1   - Error
int subset(const unsigned long *set, size_t size,
        unsigned long minM, unsigned long maxM, bool inRange,
        size_t destSize, int (*out)(const unsigned long *, size_t))
{
#ifndef NO_VALIDATE
    // Validate Input Set: values are positive and non-descending
    errno = EINVAL;
    if (set[0] < 1) return -1;
    for (size_t i = 1; i < size; i++)
        if (set[i] < set[i - 1]) return -1;

    // Validate Sizes
    if (destSize == 0) return -1;
    if (destSize > size) return -1;
    errno = 0;
#endif

    // If the max value isn't eligible, we can only remove it
    unsigned long a_n = set[size - 1];
    if ((a_n >= minM && a_n <= maxM) != inRange) goto greatest;

    // If we've got nothing to remove, only output our (eligible) set
    if (size == destSize) return out(set, size), 0;

    // Allocate Space for Reduction
    unsigned long *reduction = calloc(destSize, sizeof(unsigned long));
    if (reduction == NULL) return -1;

    // Construct Subsets, not touching the max value
    int res = remove(set, size, 0, size - 1,
            reduction, destSize, 0, out);
    free(reduction);
    if (res) return 0;

    // For removing the max value, simply replicate this process with a
    // smaller size
greatest:
    if (destSize < size) return subset(set, size - 1,
            minM, maxM, inRange, destSize, out);
    return 0;
}

// Produce Set Contractions, Relative to a Range First-Order
// Return Values
// 0    - Completed or Exited
// 1    - Enumeration was Incomplete (due to wrap-around potential)
// -1   - Error
int contraction(const unsigned long *set, size_t srcSize,
        unsigned long minM, unsigned long maxM, bool inRange,
        size_t destSize,
        int (*out)(const unsigned long *, size_t, size_t))
{
#ifndef NO_VALIDATE
    // Validate Input Set: values are positive and non-descending
    errno = EINVAL;
    if (set[0] < 1) return -1;
    for (size_t i = 1; i < srcSize; i++)
        if (set[i] < set[i - 1]) return -1;

    // Validate Sizes
    if (destSize == 0) return -1;
    if (destSize >= srcSize) return -1;
    errno = 0;
#endif

    return recursiveReduce(set, srcSize, minM, maxM, inRange,
            srcSize - destSize - 1, 0, srcSize, out);
}

// ============ Helper Functions

// Remove Set Values Recursively

// This function in effect iteratively removes values from a set to
// create subsets of a given order. We're given some starting index for
// the reduction set, and one for the original set. We will 'remove'
// original set values to the right of the starting index iteratively,
// one at a time, by simply not inserting them into the reduction,
// recursing to copy set values to the right, with any more needed
// removals, before finally inserting the value and moving to the next
// one. The iteration will stop short of the ending original set index.
// There must be at least one value to remove.
int remove(const unsigned long *set, size_t size,
        size_t startIdx, size_t endIdx,
        unsigned long *reduction, size_t destSize, size_t destIdx,
        int (*out)(const unsigned long *, size_t))
{
    // How many values to remove: as many are left in the set minus as
    // many slots remain in our reduction
    size_t removals = (size - startIdx) - (destSize - destIdx);

    // If there is only one more removal, insert values to start with
    // our starting value removed, then do our normal iterative holding
    if (removals == 1)
        for (size_t i = 0; i < destSize - destIdx; i++)
            reduction[destIdx + i] = set[startIdx + i + 1];

    // Hold values from the original set one at a time before placing
    // them in the reduction, recursing each time to perform value
    // insertions (and removals) to the right
    int res = 0;
    for (size_t i = startIdx; i <= endIdx - removals; i++) {
        if (removals == 1) res = out(reduction, destSize);
        else res = remove(set, size, i + 1, endIdx,
                reduction, destSize, destIdx, out);
        if (res) break;
        reduction[destIdx++] = set[i];
    }

    return res;
}

// Recursively Generate Set Contractions, Relative to a Range
// First-Order

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
// these sets are not computed, but the process will return a code of 1
// to indicate the omission. For nullifiability purposes, this is of
// little concern as there would have to be two really high products
// somehow being close enough.
int recursiveReduce(const unsigned long *set, size_t size,
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
        for (size_t i = 0; i < replc; i++)
        {
            // Advance our insertion index as needed, shifting values
            // leftward
            while (idx + 1 < size - 1 && replv[i] > reduction[idx + 1])
            {
                reduction[idx] = reduction[idx + 1];
                idx++;  // this could be one line. it's not undefined!
            }

            // Now insert our replacement, and we have a reduction
            reduction[idx] = replv[i];

            // If our M-value isn't relative to the range the way we
            // want, skip this
            unsigned long M = reduction[size - 2];
            if ((M >= minM && M <= maxM) != inRange) continue;

            // Output/Recursion Calls
            int res;
            res = out(reduction, size - 1, idx);
            if (res) goto exit;
            if (repeat) {
                res = recursiveReduce(reduction, size - 1, 0, 0, false,
                        repeat - 1, a, idx, out);
                if (res == 1) incomplete = true;
                else if (res) goto error;
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

