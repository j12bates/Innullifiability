// ====================== GENERAL EXHAUSTIVE TEST ======================

// Copyright (c) 2025, Jacob Bates
// SPDX-License-Identifier: BSD-2-Clause

#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>

// Recursively Test an Mset's Nullifiability
// Returns 1 on Nullifiable, 0 on Innullifiable, -1 on Error

// Input mset must be in ascending order. Input mset is assumed to not
// have any bisectable subsets of length subN. Recursively tests
// reductions, each time checking for new bisectable subsets of length
// subN, until the base case length baseN is reached.
int recursiveTest(const unsigned long *set, size_t size,
        unsigned long minM, unsigned long maxM,
        size_t baseN, size_t subN)
{
    // Allocate Space for Reduction
    unsigned long *reduction = calloc(size - 1, sizeof(unsigned long));
    if (reduction == NULL) return -1;

    // Iterate through all the possible pairs of values
    for (size_t pairA = 0; pairA < size - 1; pairA++)
        for (size_t pairB = pairA; pairB < size; pairB++)
    {
        // Fill in the reduction with all the other values, leaving the
        // spot at the beginning to start inserting replacements
        size_t idx = 1;
        for (size_t i = 0; i < size; i++)
            if (i == pairA || i == pairB) continue;
            else reduction[idx] = set[i];

        // This isn't going to work if we have a poking value
        if (maxM != 0 && reduction[size - 2] > maxM) continue;

        // Get the values of that pair
        unsigned int a = set[pairA];
        unsigned int b = set[pairB];

        // This will be our list of replacement values for the pair
        unsigned long replv[4] = {0};
        bool div = a != 0 && b % a == 0;
        bool mult = a <= ULONG_MAX / b;
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
        size_t i = 0, idx = 0;
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

                // If our M-value isn't in range, skip this
                if (reduction[size - 2] < minM) continue;
                if (maxM != 0 && reduction[size - 2] > maxM) continue;

                // Check New Subsets, then check the Reduction itself
                if checkSubsets(reduction, size - 1, subsN, idx)
                    goto nullif;
                if recursiveTest(reduction, size - 1, 0, 0,
                        baseN, subsN) goto nullif;

                // Advance to the next value
                i++;
            }
        }
    }

    free(reduction);
    return 0;

nullif:
    free(reduction);
    return 1;
}

// Check All Subsets of a Certain Size Containing a New Value
int checkSubsets(const unsigned long *set, size_t size,
        size_t subsN, size_t newIdx)
{
    return 0;
}
