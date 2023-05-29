// ========================== EQUIVALENT SETS ==========================

// This program is my approach to finding nullifiable sets quickly and
// efficiently. Rather than enumerating each set and performing some
// exhaustive test on it to see if it's nullifiable, this will instead
// 'dream up' nullifiable sets based on patterns that lead to
// nullifiability.

// This all works based off the notion of 'equivalent pairs'--pairs of
// values that, when an allowed arithmetic operation is applied, equal a
// particular value. That is to say that the equivalent pair can compute
// its value. For example, the value 2 could have the equivalent pair
// (3, 5), since 5 - 3 = 2, and (4, 8), since 8 / 4 = 2. Since we are
// only dealing with sets without repetition, things like (1, 1) don't
// count, and since a superset is of no use, things like (2, 4) don't
// count either. Given a set, any value can be substituted with any of
// its equivalent pairs (assuming that wouldn't cause a repetition with
// some other value), and that set can still compute at least all the
// same values.

// This program computes all equivalent pairs of values that can occur
// in a set, that is, pairs equivalent to values 1-M by means of only
// values 1-M. Then, for any set given, it can find 'equivalent sets' by
// iterating over every value in a set and replacing it with every
// equivalent pair for that value, all the while calling some function
// and passing in the new sets as a means of output.

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
// arithmetic results going outside that range of values, even though
// those may be necessary for a set to compute zero. I could fix this by
// implementing something complex which keeps track of the number of
// arithmetic steps and all the possible values at each step and blah
// blah blah complicated, but for what it's worth, this will eliminate a
// whole lot of sets anyways, and quickly. Nevertheless, there will have
// to be a sort of manual check to essentially weed out any stragglers
// after this passes over.

#include <stdlib.h>
#include <stdbool.h>

// 2-D Array of Equivalent Pairs by Value
// It's worth noting that this is indexed from zero, but the first
// element points to the equivalent pairs of the value one.
static long long **eqPairs = NULL;
static size_t maxPairs = 0;
static unsigned long maxValue = 0;

// Helper Function Declarations
static void genEqPairsValue(unsigned long);

static bool storeEqPair(unsigned long, size_t,
        unsigned long, unsigned long);
static bool insertPair(const unsigned long *, size_t, size_t,
        unsigned long *, long long);

// Configure Equivalent Set Maximum Value
// Returns 0 on success, -1 on memory error, -2 on input error

// This function will configure the equivalent sets program, generating
// the needed equivalent pairs up to the max value specified. It can be
// also used to deallocate the dynamic memory by calling it with an
// input value of 1 or 0.

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
int eqSetsInit(unsigned long max)
{
    // Free everything in case there was something here before
    if (eqPairs != NULL)
        for (unsigned long i = 0; i < maxValue; i++)
            free(eqPairs[i]);

    // Exit if max value doesn't make sense
    eqPairs = NULL;
    if (max < 2) return -2;

    // Each value gets an array
    eqPairs = calloc(max, sizeof(long long *));
    if (eqPairs == NULL) return -1;

    // Max equivalent pairs calculation from earlier
    maxValue = max;
    maxPairs = max - 2;
    if (max > 5) maxPairs = 3 * max / 2 - 5;

    // Allocate Memory and Enumerate for all Values
    for (unsigned long i = 1; i <= max; i++) {
        eqPairs[i - 1] = calloc(maxPairs, sizeof(long long));
        if (eqPairs[i - 1] == NULL) return -1;

        genEqPairsValue(i);
    }

    return 0;
}

// Expand Set into Equivalents with One Added Element
// Returns 0 on success, -1 on memory error, -2 on input error

// This function iteratively expands a set by one element using
// equivalent pairs. It takes in a set, which must be an array of
// numbers in ascending order and with no repetitions, except for the
// important case in which the set is of length two, as either way one
// of the values will be replaced with an equivalent pair which cannot
// contain the same value anyways. The sets it generates are guaranteed
// to have no repetitions and be in ascending order as well. These sets
// are passed into the output function as they are generated.
int eqSets(const unsigned long *set, size_t setc,
        void (*out)(const unsigned long *, size_t))
{
    // Validate Input Set Values
    for (size_t i = 0; i < setc; i++)
    {
        // Exit if value out of range
        if (set[i] > maxValue) return -2;

        if (i > 0)
        {
            // Exit if not in ascending order
            if (set[i - 1] > set[i]) return -2;

            // Exit if there's a repetition and this isn't length-two
            if (set[i - 1] == set[i] && setc != 2) return -2;
        }
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
            if (out != NULL) out(newSet, setc + 1);
        }
    }

    // Deallocate memory
    free(newSet);

    return 0;
}

// ============ Helper Functions

// Generate Equivalent Pairs for a Given Value
void genEqPairsValue(unsigned long value)
{
    size_t index = 0;

    // Sums: iterate over smaller addends
    for (unsigned long i = 1; i <= value / 2; i++)
        if (storeEqPair(value, index, i, value - i)) index++;

    // Diffs: iterate over subtrahends
    for (unsigned long i = 1; i <= maxValue - value; i++)
        if (storeEqPair(value, index, i, value + i)) index++;

    // Prods: iterate over smaller factors
    for (unsigned long i = 2; i <= value / i; i++) {
        if (value % i != 0) continue;               // only if it works
        if (storeEqPair(value, index, i, value / i)) index++;
    }

    // Quots: iterate over divisors
    for (unsigned long i = 2; i <= maxValue / value; i++)
        if (storeEqPair(value, index, i, value * i)) index++;

    return;
}

// Store an Equivalent Pair if Valid
// Returns a boolean corresponding to whether it was stored or not, to
// aid in keeping count
bool storeEqPair(unsigned long value, size_t index,
        unsigned long a, unsigned long b)
{
    // No duplicates/supersets rule
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

    // Insert values one at a time: `pairA` will contain the next new
    // (equivalent pair) value until both values have been inserted, at
    // which point it will be 0
    size_t index = 0;
    for (size_t i = 0; i < setc; i++)
    {
        // Insert the next new value if it would be in order: this is a
        // loop because we might do both at once
        while (pairA < set[i] && pairA != 0)
        {
            newSet[index++] = pairA;
            pairA = pairB;
            pairB = 0;
        }

        // Exit if the next new value causes a repetition
        if (pairA == set[i]) return false;

        // Insert the next original value unless it's being replaced
        if (i != replace) newSet[index++] = set[i];
    }

    // Place the new values if we haven't done that yet
    if (pairA != 0) newSet[index++] = pairA;
    if (pairB != 0) newSet[index++] = pairB;

    // If we haven't populated every index, we don't have a set
    return index == setc + 1;
}
