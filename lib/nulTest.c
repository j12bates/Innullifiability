// ================== EXHAUSTIVE NULLIFIABILITY TEST ===================

// Copyright (c) 2023, Jacob Bates
// SPDX-License-Identifier: BSD-2-Clause

// This code, honestly, is pretty bad. It works, but I know it could be
// so much better. I just have no idea how to optimize it. This will be
// replaced at some point, and I feel like it'll probably end up being
// done in a high-level language like Haskell or something. This is just
// so slow. For goodness sake, adding in a base case for zero-length
// increases runtime by like 20-30%.

#include <stdlib.h>
#include <stdbool.h>

// Test if a set is Nullifiable or Not
// Returns 0 if nullifiable, 1 if innullifiable, -1 on memory error
int nulTest(const unsigned long *set, size_t setc)
{
    int recursiveTest(const unsigned long *, size_t);

    // Simple cases to not use recursion on
    if (setc == 0) return 1;
    if (setc == 1) return !!set[0];

    // Use recursion
    return recursiveTest(set, setc);
}

// Returns 0 if nullifiable, 1 if innullifiable, -1 on memory error

// This function will simply determine whether or not a set is
// nullifiable. It is a recursive function, and it starts by checking if
// there is a trivial way to nullify the set, and then it performs every
// single possible operation on the set, before recursing and checking
// again as though that were a new set. If it finds that the set is
// nullifiable at any point, that means the set was always nullifiable.
// A set is only innullifiable if the test always returns that result
// after every operation. This function only works on sets that are
// size-2 or larger.
int recursiveTest(const unsigned long *set, size_t setc)
{
    // Base case
    if (setc == 2) return set[0] != set[1];

    // Here we're going to pass over everything once, just to check if
    // we can immediately say this set is nullifiable without doing a
    // bunch of hard work

    // Iterate over every value
    for (size_t pairA = 0; pairA < setc; pairA++)
    {
        // If this element is a zero, easy
        if (set[pairA] == 0) return 0;

        // If it's equal to any later element, easy
        for (size_t pairB = pairA + 1; pairB < setc; pairB++)
            if (set[pairA] == set[pairB]) return 0;
    }

    // If we can't prove nullifiability in this state, we're gonna have
    // to do some arithmetic and change up how the set looks. We'll try
    // every arithmetic operation we can on every pair we can, and pass
    // in that new 'set' to ourselves. If we can show nullifiability at
    // any point, that carries on

    // Allocate Space for New Set
    unsigned long *newSet = calloc(setc - 1, sizeof(unsigned long));
    if (newSet == NULL) return -1;

    // Iterate through all the possible pairs of values
    for (size_t pairA = 0; pairA < setc; pairA++)
        for (size_t pairB = pairA + 1; pairB < setc; pairB++)
        {
            // Fill the new set with all the other values, leave the
            // first position for the operation result
            size_t index = 1;
            for (size_t i = 0; i < setc; i++) {
                if (i == pairA || i == pairB) continue;
                else newSet[index++] = set[i];
            }

            // Get the values of that pair
            unsigned int a = set[pairA];
            unsigned int b = set[pairB];

            // List all the possible results obtained from performing
            // arithmetic operations on them, or replacement values
            unsigned long replacements[4];
            replacements[0] = a + b;
            replacements[1] = a * b;

            // There is one difference
            if (a > b) replacements[2] = a - b;
            else replacements[2] = b - a;

            // Up to one quotient is possible
            bool quotient = true;
            if (a % b == 0) replacements[3] = a / b;
            else if (b % a == 0) replacements[3] = b / a;
            else quotient = false;

            // Iterate through those replacement values
            for (size_t i = 0; i < 3ul + quotient; i++)
            {
                // Place into the new set
                newSet[0] = replacements[i];

                // Recurse on this set
                int ret = recursiveTest(newSet, setc - 1);

                // If we get an error or if it's been nullified, carry
                // that on
                if (ret != 1) {
                    free(newSet);
                    return ret;
                }
            }
        }

    // If we haven't shown nullifiability at any stage, the set is
    // innullifiable
    free(newSet);
    return 1;
}
