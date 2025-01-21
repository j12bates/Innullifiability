// ====================== GENERAL EXHAUSTIVE TEST ======================

// Copyright (c) 2025, Jacob Bates
// SPDX-License-Identifier: BSD-2-Clause

#include <stdlib.h>
#include <stdbool.h>

#include <errno.h>
#include <limits.h>

// Helper Function Declarations
static int recursiveTest(const unsigned long *, size_t,
        unsigned long, unsigned long, size_t, size_t,
        unsigned long, size_t);
static int checkSubsets(const unsigned long *, size_t,
        size_t, size_t);
static int bisect(unsigned long, unsigned long,
        unsigned long, unsigned long, size_t);

// Test an Mset's Nullifiability
// Returns 1 on Nullifiable, 0 on Innullifiable, -1 on Error
int exTest(const unsigned long *set, size_t size,
        unsigned long minM, unsigned long maxM)
{
#ifndef NO_VALIDATE
    // Validate Input Mset
    errno = EINVAL;
    if (set[0] < 1) return -1;
    for (size_t i = 1; i < size; i++)
        if (set[i - 1] > set[i]) return -1;
    errno = 0;
#endif

    // The parameters we're using
    unsigned long baseN = 4, subN = 2;

    // Check Subsets
    for (size_t i = 1; i <= subN; i++)
        if (checkSubsets(set, size, i, size)) return 1;

    // Now just recursively test!
    return recursiveTest(set, size, minM, maxM, baseN, subN, 0, size);
}

// Recursively Test an Mset's Nullifiability
// Returns 1 on Nullifiable, 0 on Innullifiable, -1 on Error

// Input mset must be in ascending order. Input mset is assumed to not
// have any bisectable subsets of length subN. Recursively tests
// reductions, each time checking for new bisectable subsets of length
// subN, until the base case length baseN is reached.
int recursiveTest(const unsigned long *set, size_t size,
        unsigned long minM, unsigned long maxM,
        size_t baseN, size_t subN,
        unsigned long minOrig, size_t idxNew)
{
    // Base Case
    if (size <= baseN)
    {
        // First check any unchecked subsets
        for (size_t i = subN + 1; i < baseN; i++)
            if (checkSubsets(set, size, i, size)) return 1;

        // Finally test the whole set
        return bisect(set[0], set[1 % size],
            set[2 % size], set[3 % size], size);
    }

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

        // This isn't going to work if we have a poking value
        if (maxM != 0 && reduction[size - 2] > maxM) continue;

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
                        baseN, subN, a, idx);
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
// If newIdx >= size, any subset
int checkSubsets(const unsigned long *set, size_t size,
        size_t subN, size_t newIdx)
{
    int insertSort(unsigned long, unsigned long, unsigned long,
            unsigned long, size_t, int (*)(unsigned long, unsigned long,
            unsigned long, unsigned long, size_t));

    // N = 1: a zero, guaranteed at beginning
    if (subN == 1) {
        if (newIdx < size) return set[newIdx] == 0;
        else return set[0] == 0;
    }

    // N = 2: double values, guaranteed consecutive
    else if (subN == 2) {
        if (newIdx + 1 < size) return set[newIdx] == set[newIdx + 1];
        else for (size_t idxA = 0; idxA < size - 1; idxA++)
            if (set[idxA] == set[idxA + 1]) return 1;
    }

    // N = 3: check manually
    else if (subN == 3) {
        for (size_t idxA = 0; idxA < size - 2; idxA++)
            for (size_t idxB = idxA + 1; idxB < size - 1; idxB++)
                for (size_t idxC = idxB + 1; idxC < size; idxC++)
        {
            if (idxA != newIdx && idxB != newIdx && idxC != newIdx
                    && newIdx < size) continue;
            if (bisect(set[idxA], set[idxB], set[idxC], 0, 3)) return 1;
        }
    }

    // N = 4: check manually
    else if (subN == 4) {
        for (size_t idxA = 0; idxA < size - 2; idxA++)
            for (size_t idxB = idxA + 1; idxB < size - 1; idxB++)
                for (size_t idxC = idxB + 1; idxC < size - 1; idxC++)
                    for (size_t idxD = idxC + 1; idxD < size; idxD++)
        {
            if (idxA != newIdx && idxB != newIdx && idxC != newIdx
                    && idxD != newIdx && newIdx < size) continue;
            if (bisect(set[idxA], set[idxB], set[idxC], set[idxD], 4))
                    return 1;
        }
    }

    return 0;
}

// Check a Set's Bisectability
int bisect(unsigned long a_1, unsigned long a_2,
        unsigned long a_3, unsigned long a_4, size_t size)
{
    // N = 1: a zero
    if (size == 1) return a_1 == 0;

    // N = 2: double values
    else if (size == 2) return a_1 == a_2;

    // N = 3: two formulations
    else if (size == 3) {
        if (a_1 + a_2 == a_3) return 1;
        else if (a_1 * a_2 == a_3) return 1;
    }

    // N = 4: nineteen formulations
    else if (size == 4)
    {
        // Totally Additive/Multiplicative Formulations
        if (a_1 + a_4 == a_2 + a_3) return 1;
        else if (a_1 + a_2 + a_3 == a_4) return 1;
        else if (a_1 * a_4 == a_2 * a_3) return 1;
        else if (a_1 * a_2 * a_3 == a_4) return 1;

        // Plus-Times Formulations
        else if (a_2 + a_3 == a_1 * a_4) return 1;
        else if (a_1 + a_4 == a_2 * a_3) return 1;
        else if (a_2 + a_4 == a_1 * a_3) return 1;
        else if (a_3 + a_4 == a_1 * a_2) return 1;

        // Minus-Times Formulations
        else if (a_4 - a_1 == a_2 * a_3) return 1;
        else if (a_4 - a_2 == a_1 * a_3) return 1;
        else if (a_4 - a_3 == a_1 * a_2) return 1;

        // Plus-Divide Formulations
        else if ((a_1 + a_2) * a_3 == a_4) return 1;
        else if ((a_1 + a_3) * a_2 == a_4) return 1;
        else if ((a_2 + a_3) * a_1 == a_4) return 1;

        // Minus-Divide Formulations
        else if ((a_2 - a_1) * a_3 == a_4) return 1;
        else if ((a_3 - a_1) * a_2 == a_4) return 1;
        else if ((a_3 - a_2) * a_1 == a_4) return 1;
        else if ((a_4 - a_2) * a_1 == a_3) return 1;
        else if ((a_4 - a_3) * a_1 == a_2) return 1;
    }

    return 0;
}
