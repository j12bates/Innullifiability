// ========================= INNULLIFIABILITY =========================
// This program is trying to solve a very specific semi-maths-related
// problem a friend and I encountered. In essence, we're trying to find
// sets of positive integer numbers which, through performing any binary
// arithmetic operations (+, -, *, /) on them (in sequence or with
// bracketing) resulting in positive integer numbers (essentially no
// non-integer ratios allowed), you cannot ever get a result of zero.

// There are 10 such sets when looking at sets of 4 values up to 9, and
// here they are:

//      1, 4, 6, 8          4, 5, 6, 8
//      1, 4, 6, 9          4, 6, 7, 8
//      1, 5, 7, 9          4, 6, 8, 9
//      3, 6, 7, 8          5, 6, 7, 9
//      3, 7, 8, 9          5, 7, 8, 9

// There are only two ways for an arithmetic operation on positive
// integers to result in zero:

//      1. Subtracting a number from itself
//      2. Multiplying any number by zero

// So clearly sets with 0 in them are out, as are sets with the same
// number twice, so we only look at sets starting at 1 without
// repetition. The only way one of these sets can be 'nullifiable' is by
// having a pair or subset of numbers which, by application of the
// arithmetic operations, gives another number in the set.

// The way this program works is simple: using sets of 'equivalent
// pairs', pairs of numbers which can equal a particular value by
// applying an operation, trivial nullifiable sets are expanded, with
// values being replaced with their equivalent pairs, thus giving new
// nullifiable sets which can in turn be expanded upon, and whose
// supersets are nullifiable as well. This continues and should cover
// every possible nullifiable set within the range of sets given. Any
// sets remaining are INNULLIFIABLE, and therefore interesting.

// Yeah, I know, I could've done it in like eight lines of Haskell or
// something. Fun fact, I actually tried it and it was super slow. So
// I'm doing it the only other way I know how. And I think it'll work.
// It might be big, but it'll be fast. I think.

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// Size of Set (N), Maximum Value (M)
int size, max;

// Factorials
size_t *fact;

// This points to an array containing a boolean value for each set. The
// boolean is essentially just there to store whether the set has yet
// been proven nullifiable. The index in this array corresponding to a
// particular set is used for referencing a set.

// The order of the sets is just in standard numeric order. The first
// element starts at 1. The next element starts at whatever the current
// element is plus one. Once the value of the next element cannot be
// increased (has reached max - level), the current element's value is
// increased.
bool *sets;

// This points to arrays containing the indices for the sets containing
// each individual value. This is used for figuring out what sets have
// a particular set of values, and thus which to mark as nullifiable. Be
// careful with indexing! Index 0 corresponds to the value 1.
size_t **occs;
size_t *occsPtrs;

// A Value's Equivalent Pairs are what I like to call pairs of values
// which, when some binary arithmetic operation is applied, equal that
// value. When a value in a set is substituted with one of these, it can
// be made equal to all the same values. Equivalent pairs cannot contain
// the value the pair is meant to be equivalent to (as this is just
// redundant for nullifiability-checking purposes), and the values must
// not be the same as we're only looking at sets without repetition.
// Again, index 0 corresponds to the value 1.
long long int **eqPairs;

// Actually, I should separate functions into different source files.
size_t enumerateOccs(size_t, size_t, int);
void enumerateEqPairs(int);
bool insertEqPair(int, size_t, int, int);

int main(int argc, char *argv[])
{
    // Get Command Line Arguments for Set Size and Max Value
    if (argc < 3) {
        fprintf(stderr, "Missing Command Line Arguments\n");
        return 1;
    }

    size = atoi(argv[1]);       // I'll call this N, for Number of
                                // Elements

    max = atoi(argv[2]);        // And this is M, the Maximum Value

    // To avoid repetition, sets cannot have more elements than the
    // total number of values. Also, fewer than two values? Sheesh.
    if (size > max || max < 2) {
        fprintf(stderr, "Invalid Parameters\n");
        return 1;
    }


    // ============ Factorials

    // So basically, the next couple of calculations need factorials. So
    // we're gonna make an array as long as we need and store them in
    // there. Easy.

    // Allocate Memory
    fact = calloc(max + 1, sizeof(size_t));
    if (fact == NULL) return 1;

    // Calculate all Factorials Needed... surely you can understand this
    fact[0] = 1;
    for (int i = 1; i < max + 1; i++)
        fact[i] = fact[i - 1] * i;


    // ============ Set Statuses

    // We're counting sets without repetition. There are M values (1 to
    // M) and we're choosing N separate values. We calculate the total
    // number of distinct choices like this:
    //     M * (M - 1) * (M - 2) * ... * (M - (N - 1))
    // or this:
    //     M! / (M - N)!
    // then we divide by the number of permutations, N!, as we don't
    // care about order. This gives the total number of sets:
    //     M! / (M - N)! / N!
    size_t totalSets = fact[max] / fact[max - size] / fact[size];

    // Allocate Memory
    sets = calloc(totalSets, sizeof(bool));
    if (sets == NULL) return 1;


    // ============ Value Occurrences

    // Each value gets an array
    occs = calloc(max, sizeof(size_t *));
    if (occs == NULL) return 1;

    // Total sets times N gives total number of values in all sets, and
    // values are equally distributed. This gives the total number of
    // occurrences of each value:
    //     totalSets * N / M
    size_t totalOccs = totalSets * size / max;
    for (int i = 0; i < max; i++) {
        occs[i] = calloc(totalOccs, sizeof(size_t));
        if (occs[i] == NULL) return 1;
    }

    // This is just for keeping track of where we're storing the next
    // index
    occsPtrs = calloc(max, sizeof(size_t));
    if (occsPtrs == NULL) return 1;

    // Enumerate Value Occurrences
    enumerateOccs(size, 0, 1);

    printf("thingie\n");

    // ============ Equivalent Pairs

    // Each value gets an array
    eqPairs = calloc(max, sizeof(long long int *));
    if (eqPairs == NULL) return 1;

    // Now we need to calculate the maximum number of equivalent pairs a
    // value could have, as I don't want to mess around with a function
    // returning stuff it allocated itself.

    // Number of diff pairs can be calculated for a value V by the
    // following:
    //     M - V - 1    for V <= M / 2
    //     M - V        otherwise
    // The -1 in the first case is because we have to ignore the case
    // where the subtrahend is equal to V.

    // Number of quot pairs can be calculated for a value V by the
    // following:
    //     M / V - 2    for V > 1
    // The -2 is also to ignore cases where the dividend and divisor
    // are equal to V. It's also worth noting that this uses integer
    // division and that the minimum number of pairs is zero.

    // Sum pairs only increase for every other value, and prod pairs
    // are just kinda nowhere.

    // For very small values of M, the value of 1 has the most
    // equivalent pairs, due to all the diff pairs you get with low
    // values:
    //     M - 2
    size_t maxEqPairs = max - 2;

    // When M > 5, the value of 2 has more as 2 gets a lot of quot pairs
    // as well:
    //     M - 3 + M / 2 - 2
    // or:
    //     3M / 2 - 5
    if (max > 5) maxEqPairs = 3 * max / 2 - 5;

    // Allocate Memory and Enumerate for all Values
    for (int i = 0; i < max; i++) {
        eqPairs[i] = calloc(maxEqPairs, sizeof(long long int));
        if (eqPairs[i] == NULL) return 1;

        enumerateEqPairs(i + 1);
    }

    // TODO remove -- debug for checking enumerations
    printf("\n\nOccurrences:\n");
    for (int i = 0; i < max; i++) {
        printf("%2d:    ", i + 1);
        for (int j = 0; j < totalOccs; j++) {
            printf("%4d", occs[i][j]);
        }
        printf("\n\n");
    }
    printf("\n\nEquivalent Pairs:\n");
    for (int i = 0; i < max; i++) {
        printf("%2d:    ", i + 1);
        for (int j = 0; j < maxEqPairs; j++) {
            printf("\t(%d,%d) ",
                    (int) eqPairs[i][j], (int) (eqPairs[i][j] >> 32)
            );
        }
        printf("\n\n");
    }

    return 0;
}

// Recursive Function for Enumerating Value Occurrences

// This function is used for enumerating value occurrences. Calling it
// with the number of levels as N and the index set to 0 and value set
// to 1 will perform the full enumeration.

// This function is recursive, and it's important to note that it
// returns the total number of sets marked or otherwise operated on, so
// that one level above, this same function can count those sets as also
// containing the value it's currently working on. Apart from that, it
// just iterates over all the possible values for that level.
size_t enumerateOccs(size_t level, size_t index, int value)
{
    // Base case: if there are no more levels, parent should mark only
    // one set
    if (level == 0) return 1;

    size_t totalOps = 0;

    // Iterate over all the values for which you can still have a set
    // without repetition
    for (; value <= max - level + 1; value++) {

        // Recurse to mark sets under this level appropriately and to
        // get the total number of them
        size_t ops = enumerateOccs(level - 1, index, value + 1);
        size_t nextIndex = index + ops;

        // Mark these sets as occurrences of this value
        for (; index < nextIndex; index++) {
            occs[value - 1][occsPtrs[value - 1]] = index;
            occsPtrs[value - 1]++;
        }

        // Keep count of total sets operated on
        totalOps += ops;

    }

    return totalOps;
}

// Enumerate Equivalent Pairs
void enumerateEqPairs(int value)
{
    size_t index = 0;

    // Sums: iterate over smaller addends
    for (int i = 1; i <= value / 2; i++)
        if (insertEqPair(value, index, i, value - i)) index++;

    // Diffs: iterate over subtrahends
    for (int i = 1; i <= max - value; i++)
        if (insertEqPair(value, index, i, value + i)) index++;

    // Prods: iterate over smaller factors
    for (int i = 2; i <= value / i; i++) {
        if (value % i != 0) continue;               // only if it works
        if (insertEqPair(value, index, i, value / i)) index++;
    }

    // Quots: iterate over divisors
    for (int i = 2; i <= max / value; i++)
        if (insertEqPair(value, index, i, value * i)) index++;
}

// Check if Two Values are a Valid Equivalent Pair and Store it if so
bool insertEqPair(int value, size_t index, int a, int b)
{
    // No duplicates rule
    if (a == value || b == value || a == b) return false;

    // Store as a single long long
    eqPairs[value - 1][index]   = (long long int) a
                                | (long long int) b << 32;

    return true;
}
