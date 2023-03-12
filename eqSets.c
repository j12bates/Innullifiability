// ========================== EQUIVALENT SETS ==========================

#include <stdlib.h>
#include <stdbool.h>

// Maximum Set Value (M)
unsigned long max;

// 2-D Array of Equivalent Pairs by Value
long long **eqPairs = NULL;

// Function Declarations
void enumerateEqPairs(int);
bool storeEqPair(int, size_t, int, int);

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
int eqSetsInit(unsigned long maxValue)
{
    // Free everything in case there was something here before
    if (eqPairs != NULL)
        for (unsigned long i = 0; i < max; i++)
            free(eqPairs[i]);

    // Set Maximum Set Value
    max = maxValue;

    // Each value gets an array
    eqPairs = calloc(max, sizeof(long long *));
    if (eqPairs == NULL) return -1;

    // Max equivalent pairs calculation from earlier
    size_t maxEqPairs = max - 2;
    if (max > 5) maxEqPairs = 3 * max / 2 - 5;

    // Allocate Memory and Enumerate for all Values
    for (int i = 1; i <= max; i++) {
        eqPairs[i - 1] = calloc(maxEqPairs, sizeof(long long int));
        if (eqPairs[i - 1] == NULL) return -1;

        enumerateEqPairs(i);
    }
}

// ============ Helper Functions

// Enumerate Equivalent Pairs for a Given Value
void enumerateEqPairs(int value)
{
    size_t index = 0;

    // Sums: iterate over smaller addends
    for (int i = 1; i <= value / 2; i++)
        if (storeEqPair(value, index, i, value - i)) index++;

    // Diffs: iterate over subtrahends
    for (int i = 1; i <= max - value; i++)
        if (storeEqPair(value, index, i, value + i)) index++;

    // Prods: iterate over smaller factors
    for (int i = 2; i <= value / i; i++) {
        if (value % i != 0) continue;               // only if it works
        if (storeEqPair(value, index, i, value / i)) index++;
    }

    // Quots: iterate over divisors
    for (int i = 2; i <= max / value; i++)
        if (storeEqPair(value, index, i, value * i)) index++;
}

// Check if Two Values are a Valid Equivalent Pair and Store if so
bool storeEqPair(int value, size_t index, int a, int b)
{
    // No duplicates rule
    if (a == value || b == value || a == b) return false;

    // Store as a single long long
    eqPairs[value - 1][index]   = (long long int) a
                                | (long long int) b << 32;

    return true;
}
