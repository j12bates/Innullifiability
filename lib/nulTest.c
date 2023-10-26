// ================== EXHAUSTIVE NULLIFIABILITY TEST ===================

// Copyright (c) 2023, Jacob Bates
// SPDX-License-Identifier: BSD-2-Clause

// This will be replaced at some point, and I feel like it'll probably
// end up being done in a high-level language like Haskell or something.
// This is just kinda hard to reason about. I do know that many more
// frames exist for the smaller sets than the larger ones, so my idea
// right now is just to work on optimizing for those smaller sets.

#include <stdlib.h>
#include <stdbool.h>

// Test if a set is Nullifiable or Not
// Returns 0 if nullifiable, 1 if innullifiable, -1 on memory error
int nulTest(const unsigned long *set, size_t size)
{
    int recursiveTest(const unsigned long *, size_t);

    // Simple cases to not use recursion on
    if (size == 0) return 1;
    if (size == 1) return set[0] != 0;
    if (size == 2) return set[0] != set[1];
    for (size_t i = 0; i < size; i++) if (set[i] == 0) return 0;

    // Use recursion
    return recursiveTest(set, size);
}

// Test if a Length-3 Set is Nullifiable or Not
// Returns 0 if nullifiable, 1 if innullifiable

// This function is simplified and optimized for length-3 sets. It first
// checks if any two values are equal. Then it checks if applying some
// operation to two values will result in a matching pair. It can do so
// using only six checks and two operators since the pairs of arithmetic
// operations are inverse and all the other possibilities can just be
// rearranged into one of these. Like the recursive function, this only
// takes positive integers.
int nulTestTriplet(const unsigned long set[3])
{
    unsigned long a = set[0], b = set[1], c = set[2];

    // Try simple equality
    if (a == b || b == c || c == a) return 0;

    // Try additive operations
    else if (a + b == c) return 0;
    else if (b + c == a) return 0;
    else if (c + a == b) return 0;

    // Try multiplicative operations
    else if (a * b == c) return 0;
    else if (b * c == a) return 0;
    else if (c * a == b) return 0;

    // If none worked, innullifiable
    return 1;
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
// size-3 or larger, and positive integers only.
int recursiveTest(const unsigned long *set, size_t size)
{
    // Base case
    if (size == 3) return nulTestTriplet(set);

    // Pass over every pair of values, checking for simple equality
    for (size_t pairA = 0; pairA < size; pairA++)
        for (size_t pairB = pairA + 1; pairB < size; pairB++)
            if (set[pairA] == set[pairB]) return 0;

    // If we can't prove nullifiability in this state, we're gonna have
    // to do some arithmetic and change up how the set looks. We'll try
    // every arithmetic operation we can on every pair we can, and pass
    // in that new 'set' to ourselves. If we can show nullifiability at
    // any point, that carries on

    // Allocate Space for New Set
    unsigned long *newSet = calloc(size - 1, sizeof(unsigned long));
    if (newSet == NULL) return -1;

    // Iterate through all the possible pairs of values
    for (size_t pairA = 0; pairA < size; pairA++)
        for (size_t pairB = pairA + 1; pairB < size; pairB++)
    {
        // Fill the new set with all the other values, leave the first
        // position for the operation result
        size_t index = 1;
        for (size_t i = 0; i < size; i++) {
            if (i == pairA || i == pairB) continue;
            else newSet[index++] = set[i];
        }

        // Get the values of that pair
        unsigned int a = set[pairA];
        unsigned int b = set[pairB];

        // List all the possible results obtained from performing
        // arithmetic operations on them, or replacement values
        unsigned long replacements[4] = {0};
        replacements[1] = a + b;
        replacements[3] = a * b;

        // There is one difference (won't generate a zero as we've
        // already scanned for equality)
        if (a > b) replacements[0] = a - b;
        else replacements[0] = b - a;

        // Up to one quotient is possible
        if (a % b == 0) replacements[2] = a / b;
        else if (b % a == 0) replacements[2] = b / a;

        // Iterate through those replacement values
        for (size_t i = 0; i < 4; i++)
        {
            // If empty element, this means nothing
            if (replacements[i] == 0) continue;

            // Place into the new set
            newSet[0] = replacements[i];

            // Recurse on this set
            int res = recursiveTest(newSet, size - 1);

            // If we get an error or if it's been nullified, carry that
            // on
            if (res != 1) {
                free(newSet);
                return res;
            }
        }
    }

    // If we haven't shown nullifiability at any stage, the set is
    // innullifiable
    free(newSet);
    return 1;
}
