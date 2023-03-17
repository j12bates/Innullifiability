// ========================== EQUIVALENT SETS ==========================

// This program is my approach to finding nullifiable sets quickly and
// efficiently. Rather than enumerating each set and performing some
// exhaustive test on it to see if it's nullifiable, this will instead
// 'dream up' nullifiable sets based on patterns that lead to
// nullifiability.

// This all works based off the notion of 'equivalent pairs'--pairs of
// values that, by some allowed arithmetic operation, equal a particular
// value. For example, the value 2 could have the equivalent pair
// (3, 5), since 5 - 3 = 2, and (4, 8), since 8 / 4 = 2. Since we are
// only dealing with sets without repetition, things like (1, 1) or 
// (2, 4) don't count. Given a set, any value can be substituted with
// any of its equivalent pairs (assuming that wouldn't cause a
// repetition with some other value), and that set can still equal at
// least all the same values by performing arithmetic operations, since
// we know the values of the equivalent pair can definitely be made
// equal to the original value.

// This program computes all equivalent pairs of values that can occur
// in a set, that is, pairs equivalent to values 1-M by means of only
// values 1-M. Then, for any set given, it can find 'equivalent sets' by
// iterating over every value in a set and replacing it with every
// equivalent pair for that value, all the while calling some
// function provided by a function pointer, passing in sets as they are
// generated, and recursing on new sets as they are generated.

// So what does this have to do with nullifiability? Well, the only way
// for a set to be nullifiable is by it having some means of getting a
// value to equal another value, so that they can be subtracted and then
// that zero can be multiplied to any remaining values. Given this, we
// can start from a base set containing two of the same value, and then
// repeatedly expand by equivalent pairs up to as many elements as a set
// can allow. As we keep going, the set will continue to have a route to
// equalling zero. Any sets we calculate along the way, as well as their
// supersets, can be marked as nullifiable.

// It's important to realize though that this cannot cover all possible
// equivalent sets. The equivalent pairs, being 1-M only, cannot cover
// arithmetic results going outside the range of values used within the
// sets, even though those may be necessary for a set being equivalent
// to a particular value. I could fix this by implementing something
// complex which keeps track of the number of arithmetic steps and all
// the possible values at each step and blah blah blah complicated, but
// for what it's worth, this will eliminate a whole lot of sets anyways,
// and quickly. Nevertheless, there will have to be a sort of manual
// check to essentially weed out any stragglers after this passes over.

#include <stdlib.h>
#include <stdbool.h>

// Maximum Set Value (M)
static unsigned long max;

// Final Set Size (N)
static size_t size;

// 2-D Array of Equivalent Pairs by Value
// It's worth noting that this is indexed from zero, but the first
// element points to the equivalent pairs of the value one.
static long long **eqPairs = NULL;
static size_t maxPairs;

// Helper Function Declarations
static void enumerateEqPairs(unsigned long);
static bool storeEqPair(unsigned long, size_t,
        unsigned long, unsigned long);
static bool insertPair(const unsigned long *, size_t, size_t,
        unsigned long *, long long);

// Reconfigure Equivalent Set Generation
// Returns 0 on success, -1 on memory error, -2 on input error

// This function will reset the equivalent set generation program, which
// just involves making space for all the equivalent pairs we need and
// generating them. It takes in the maximum set value (M) and the number
// of set elements (N). To avoid repetition, N cannot be less than M,
// and also it doesn't make any sense for N to be less than 2. But you
// could just call this function with invalid values to deallocate the
// dynamic memory. That'd be much appreciated.

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
int eqSetsInit(unsigned long newMax, size_t newSize)
{
    // Free everything in case there was something here before
    if (eqPairs != NULL)
        for (unsigned long i = 0; i < max; i++)
            free(eqPairs[i]);

    // Exit if these values don't make sense
    eqPairs = NULL;
    if (newSize > newMax || newMax < 2) return -2;

    // Set Variables
    max = newMax;
    size = newSize;

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

    return 0;
}

// Enumerate Equivalent Sets
// Returns 0 on success, -1 on memory error, -2 on input error

// This function iteratively and recursively expands a set using
// equivalent pairs. It takes in a set, which must be an array of
// numbers in ascending order and with no repetitions, except for the
// important case in which the set is of length two, as either way one
// of the values will be replaced with an equivalent pair which cannot
// contain the same value anyways. The sets it generates are guaranteed
// to have no repetitions and be in ascending order as well. These sets
// are passed into the function passed in here the moment they are
// generated.
int eqSets(const unsigned long *set, size_t setc,
        void (*fn)(const unsigned long *, size_t))
{
    // If we've reached the maximum, exit successfully
    if (setc == size) return 0;

    // Exit with input error if we're not in ascending order, or if
    // there are no repetitions, except for the length-two case
    for (size_t i = 1; i < setc; i++)
    {
        if (set[i - 1] > set[i]) return -2;
        if (set[i - 1] == set[i] && setc != 2) return -2;
    }

    // Allocate space for expanded set
    unsigned long *newSet = calloc(setc + 1, sizeof(unsigned long));
    if (newSet == NULL) return -1;

    // Iterate over the values in the set
    for (size_t i = 0; i < setc; i++)
    {
        // Get the value in that position
        unsigned long value = set[i];

        // If this is a repeat, it won't result in anything new
        if (i > 0) if (set[i] == set[i - 1]) continue;

        // Iterate over the equivalent pairs for that value
        for (size_t j = 0; j < maxPairs; j++)
        {
            // The equivalent pair
            long long pair = eqPairs[value - 1][j];

            // Exit this loop if there are no more equivalent pairs
            if (pair == 0) break;

            // Otherwise, try inserting it
            if (insertPair(set, setc, i, newSet, pair) == false)
                continue;

            // If it worked, call function
            fn(newSet, setc + 1);

            // Recurse on this new set
            if (eqSets(newSet, setc + 1, fn) == -1)
            {
                free(newSet);
                return -1;
            }
        }
    }

    // Deallocate memory
    free(newSet);

    return 0;
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

    return;
}

// Store an Equivalent Pair if Valid
// Returns a boolean corresponding to whether it was stored or not, to
// aid in keeping count
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

// Insert Pair into Sorted Array
// Returns a boolean for whether or not the insertion could be made
bool insertPair(const unsigned long *set, size_t setc, size_t replace,
        unsigned long *newSet, long long pair)
{
    // The values in the equivalent pair
    unsigned long pairA = (unsigned long) pair & 0xFFFFFFFF;
    unsigned long pairB = (unsigned long) (pair >> 32) & 0xFFFFFFFF;

    // Insert values one at a time: `pairA` will contain the
    // next new (equivalent pair) value until both values have
    // been inserted, at which point it will be 0
    size_t index = 0;
    for (size_t i = 0; i < setc; i++)
    {
        // Insert the next new value if it would be in order
        while (pairA < set[i] && pairA != 0)
        {
            newSet[index++] = pairA;
            pairA = pairB;
            pairB = 0;
        }

        // Exit if the next new value causes a repetition
        if (pairA == set[i]) return false;

        // Insert the next original value unless it's being
        // replaced
        if (i != replace) newSet[index++] = set[i];
    }

    // Place the new values if we haven't done that yet
    if (pairA != 0) newSet[index++] = pairA;
    if (pairB != 0) newSet[index++] = pairB;

    // If we haven't populated every index, we don't have a set
    return index == setc + 1;
}
