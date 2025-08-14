// =============================== TESTS ===============================

// Copyright (c) 2025, Jacob Bates
// SPDX-License-Identifier: BSD-2-Clause

// This library implements tests for integer sets. It is primarily used
// in testing sets' nullifiability, but can be used for other reduction-
// based properties, like unifiability.

// Bisectability tests are implemented for sizes up to 4, and
// successibility is implemented for sizes up to 3. Larger sizes must
// utilize conversions to be tested. Additionally, this library
// implements a bisectability tester for subsets of a particular
// testable length.

// Sets are allowed to have multiplicities and are assumed to be given
// in non-descending order.

#include <stdlib.h>

#include "tests.h"

// Test Bisectability on All Subsets of a Certain Size Containing a
// Given 'New' Value
// If newIdx >= size, any subset
int bisectSubs(const unsigned long *set, size_t size,
        size_t subN, size_t newIdx)
{
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
        for (size_t idxA = 0; idxA < size - 3; idxA++)
            for (size_t idxB = idxA + 1; idxB < size - 2; idxB++)
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

// Check a Set's Successibility
int success(unsigned long a_1, unsigned long a_2,
        unsigned long a_3, size_t size)
{
    // N = 1: a one
    if (size == 1) return a_1 == 1;

    // N = 2: successive values
    else if (size == 2) return a_1 + 1 == a_2;

    // N = 3: nine formulations (one special case)
    else if (size == 3)
    {
        // Additive Formulations (a +/- b = c +/- 1)
        if (a_1 + a_2 + 1 == a_3) return 1;
        else if (a_1 + a_2 - 1 == a_3) return 1;

        // Times Formulations (a * b = c +/- 1)
        else if (a_1 * a_2 + 1 == a_3) return 1;
        else if (a_1 * a_2 - 1 == a_3) return 1;
        else if (a_1 * a_3 - 1 == a_2) return 1;    // implies a_1 == 1

        // Divide Formulations (a / b = c +/- 1)
        else if (a_1 * (a_2 + 1) == a_3) return 1;
        else if (a_1 * (a_2 - 1) == a_3) return 1;
        else if (a_2 * (a_1 + 1) == a_3) return 1;
        else if (a_2 * (a_1 - 1) == a_3) return 1;
    }

    return 0;
}
