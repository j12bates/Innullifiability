// ========================== EQUIVALENT SETS ==========================

#include <stdlib.h>
#include <stdbool.h>

// Maximum Set Value (M)
unsigned long max;

// Final Set Size (N)
size_t size;

// 2-D Array of Equivalent Pairs by Value
long long **eqPairs = NULL;
size_t maxPairs;

// Function Declarations
void enumerateEqPairs(unsigned long);
bool storeEqPair(unsigned long, size_t, unsigned long, unsigned long);

// Reset Equivalent Set Generation
// Returns 0 on success, -1 on memory error

// This function will reset the equivalent set generation program, which
// just involves making space for all the equivalent pairs we need and
// generating them.

// In order to calculate how much space we need to allocate, we'd like
// to know the maximum number of equivalent pairs a value could have.

// Number of diff pairs can be calculated for a value V by the
// following:
//     M - V - 1    for V <= M / 2
//     M - V        otherwise
// The -1 in the first case is because we have to ignore the case where
// the subtrahend is equal to V.

// Number of quot pairs can be calculated for a value V by the
// following:
//     M / V - 2    for V > 1
// The -2 is also to ignore cases where the dividend and divisor are
// equal to V. It's also worth noting that this uses integer division
// and that the minimum number of pairs is zero.

// Sum pairs only increase for every other value, and prod pairs are
// just kinda nowhere.

// For very small values of M, the value of 1 has the most equivalent
// pairs, due to all the diff pairs you get with low values:
//     M - 2

// When M > 5, the value of 2 has more as 2 gets a lot of quot pairs as
// well:
//     M - 3 + M / 2 - 2
// or:
//     3M / 2 - 5
int eqSetsInit(unsigned long maxValue, size_t setSize)
{
    // Free everything in case there was something here before
    if (eqPairs != NULL)
        for (unsigned long i = 0; i < max; i++)
            free(eqPairs[i]);

    // Set Variables
    max = maxValue;
    size = setSize;

    // Each value gets an array
    eqPairs = calloc(max, sizeof(long long *));
    if (eqPairs == NULL) return -1;

    // Max equivalent pairs calculation from earlier
    maxPairs = max - 2;
    if (max > 5) maxPairs = 3 * max / 2 - 5;

    // Allocate Memory and Enumerate for all Values
    for (unsigned long i = 1; i <= max; i++) {
        eqPairs[i - 1] = calloc(maxPairs, sizeof(long long));
        if (eqPairs[i - 1] == NULL) return -1;

        enumerateEqPairs(i);
    }
}

// Recursively Expand Nullifiable Set
// Returns 0 on success, -1 on memory error
int eqSets(const unsigned long *set, size_t setc,
        void (*fn)(const unsigned long *, size_t))
{
    // If we've reached the maximum, exit successfully
    if (setc == size) return 0;

    // Iterate over the values in the set
    for (size_t i = 0; i < setc; i++)
    {
        // Iterate over the equivalent pairs for that value
        for (size_t j = 0; j < maxPairs; j++)
        {
            // Exit this loop if there are no more equivalent pairs
            if (eqPairs[i][j] == 0) break;

            // The values in that equivalent pair
            unsigned long pairA = (unsigned long) eqPairs[i][j];
            unsigned long pairB = (unsigned long) (eqPairs[i][j] >> 32);

            // Allocate space for expanded set
            unsigned long *newSet =
                calloc(setc + 1, sizeof(unsigned long));
            if (newSet == NULL) return -1;

            // Insert values one at a time: `pairA` will contain the
            // next new value until both values have been inserted, at
            // which point it will be 0
            size_t index = 0;
            for (size_t k = 0; k < setc; k++)
            {
                // We can't have a repetition
                if (set[k] == pairA) break;

                // Insert the next new value if needed
                if (set[k] > pairA && pairA != 0)
                {
                    newSet[index++] = pairA;
                    pairA = pairB;
                    pairB = 0;
                }

                // Insert the next value unless it's being replaced
                if (k != i) newSet[index++] = set[k];
            }

            // If we haven't populated every index, we don't have a set
            if (index < setc + 1) continue;

            // Otherwise, call function
            fn(newSet, setc + 1);

            // Recurse on this new set
            if (eqSets(newSet, setc + 1, fn) == -1) return -1;
        }
    }
}

// ============ Helper Functions

// Enumerate Equivalent Pairs for a Given Value
void enumerateEqPairs(unsigned long value)
{
    size_t index = 0;

    // Sums: iterate over smaller addends
    for (unsigned long i = 1; i <= value / 2; i++)
        if (storeEqPair(value, index, i, value - i)) index++;

    // Diffs: iterate over subtrahends
    for (unsigned long i = 1; i <= max - value; i++)
        if (storeEqPair(value, index, i, value + i)) index++;

    // Prods: iterate over smaller factors
    for (unsigned long i = 2; i <= value / i; i++) {
        if (value % i != 0) continue;               // only if it works
        if (storeEqPair(value, index, i, value / i)) index++;
    }

    // Quots: iterate over divisors
    for (unsigned long i = 2; i <= max / value; i++)
        if (storeEqPair(value, index, i, value * i)) index++;
}

// Check if Two Values are a Valid Equivalent Pair and Store if so
bool storeEqPair(unsigned long value, size_t index,
        unsigned long a, unsigned long b)
{
    // No duplicates rule
    if (a == value || b == value || a == b) return false;

    // Smaller value rule
    if (a > b) return false;

    // Store as a single long long
    eqPairs[value - 1][index]   = (long long) a
                                | (long long) b << 32;

    return true;
}
