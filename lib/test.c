// ====================== GENERAL EXHAUSTIVE TEST ======================

// Copyright (c) 2025, Jacob Bates
// SPDX-License-Identifier: BSD-2-Clause

#include <stdlib.h>
#include <stdbool.h>

#include <errno.h>
#include <limits.h>

// Helper Function Declarations
static int recursiveTest(const unsigned long *, size_t,
        unsigned long, unsigned long, size_t, size_t);
static int checkSubsets(const unsigned long *, size_t,
        size_t, size_t);
static int bisect(unsigned long, unsigned long,
        unsigned long, unsigned long, size_t);

// Test a Set's Nullifiability
// Returns 1 on Nullifiable, 0 on Innullifiable, -1 on Error
int test(const unsigned long *set, size_t size,
        unsigned long minM, unsigned long maxM)
{
#ifndef NO_VALIDATE
    // Validate Input Set
    errno = EINVAL;
    if (set[0] < 1) return -1;
    for (size_t i = 1; i < size; i++)
        if (set[i - 1] >= set[i]) return -1;
    errno = 0;
#endif

    // The parameters we're using
    unsigned long baseN = 3, subN = 2;

    // Check Subsets
    for (size_t i = 0; i <= subN; i++)
        for (size_t idx = 0; idx < size; idx++)
            if (checkSubsets(set, size, i, idx)) return 1;

    // Now just recursively test!
    return recursiveTest(set, size, minM, maxM, baseN, subN);
}

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
    // Base Case
    if (size <= baseN) return bisect(set[0], set[1 % size],
            set[2 % size], set[3 % size], size);

    // Allocate Space for Reduction
    unsigned long *reduction = calloc(size - 1, sizeof(unsigned long));
    if (reduction == NULL) return -1;

    // Iterate through all the possible pairs of values
    for (size_t pairA = 0; pairA < size - 1; pairA++)
        for (size_t pairB = pairA + 1; pairB < size; pairB++)
    {
        // Fill in the reduction with all the other values, leaving the
        // spot at the beginning to start inserting replacements
        size_t idx = 1;
        for (size_t i = 0; i < size; i++)
            if (i == pairA || i == pairB) continue;
            else reduction[idx++] = set[i];

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

                // If our M-value isn't in range, skip this
                if (reduction[size - 2] < minM) continue;
                if (maxM != 0 && reduction[size - 2] > maxM) continue;

                // Check New Subsets
                if (subN && checkSubsets(reduction, size - 1, subN,
                        idx)) {
                    free(reduction);
                    return 1;
                }

                // Check the Reduction Itself, carry any error
                int res = recursiveTest(reduction, size - 1, 0, 0,
                        baseN, subN);
                if (res) {
                    free(reduction);
                    return res;
                }

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
        size_t subN, size_t newIdx)
{
    if (subN == 1) return set[newIdx] == 0;
    else if (subN == 2) {
        bool pair = false;
        if (newIdx + 1 < size) pair |= (set[newIdx] == set[newIdx + 1]);
        if (newIdx > 0) pair |= (set[newIdx] == set[newIdx - 1]);
        return pair;
    }
    // TODO: N = 3, N = 4 case
    return 0;
}

// Check a Set's Bisectability
int bisect(unsigned long a_1, unsigned long a_2,
        unsigned long a_3, unsigned long a_4, size_t size)
{
    if (size == 1) return a_1 == 0;
    else if (size == 2) return a_1 == a_2;
    else if (size == 3) {
        if (a_1 + a_2 == a_3) return 1;
        if (a_1 * a_2 == a_3) return 1;
    }
    // TODO: N = 4 case
    return 0;
}
