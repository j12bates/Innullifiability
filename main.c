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

// So clearly sets containing zero itself are out, as are sets with the
// same number twice, so we only look at sets starting at 1 without
// repetition. The only way one of these sets can be 'nullifiable' is by
// having a pair or subset of numbers which, by application of the
// arithmetic operations, gives another number in the set.

// In the most general sense, the program works by finding all the
// nullifiable sets and then returning whatever sets remain.

// Of course, one could simply create a recursive function to
// exhaustively test every operation on a set to try and make two values
// the same. However, doing this for every set would get quite costly.
// So instead, this program works backwards, enumerating a bunch of
// nullifiable sets by considering every single way that a particular
// value might have been reached. For example, given that the set
// (1, 3, 4) is nullifiable, we could then generate a bunch of other
// nullifiable sets by expanding the 3 into 5 and 2 (5 - 2 = 3), and all
// the other 'equivalent pairs' of 3. Also, all supersets of nullifiable
// sets are also nullifiable, as zero times anything is zero still. This
// method is described in a lot better detail in the Equivalent Sets
// program. By generating sets instead of testing sets, we can eliminate
// a whole lot more sets faster than running a test over every set.

// We keep track of the sets by way of boolean values in a tree. This
// tree has a node for each set, and the library provides functions for
// 'marking' sets and supersets to ignore them when traversing for any
// remaining innullifiable sets at the end.

// Yeah, I know, I could've done it in like eight lines of Haskell or
// something. Fun fact, I actually tried it and it was super slow. So
// I'm doing it the only other way I know how. And I think it'll work.
// It might be big, but it'll be fast. I think.

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include "setRec.h"
#include "eqSets.h"
#include "nulTest.h"

// Bitmasks for Set Records
#define NULLIF      1 << 1
#define SUPERSET    NULLIF | 1 << 2
#define MARKED      NULLIF | SUPERSET

// Size of Sets (N)
size_t size;

// Maximum Element Value (M)
unsigned long max;

// Set Records for Each Length 1-N
SR_Base **rec;

// Supplementary Function Declarations
void eliminate(const unsigned long *, size_t);
void expandNul(const unsigned long *, size_t);
void verify(const unsigned long *, size_t);
void printSet(const unsigned long *, size_t);

long long retrieve(void (*)(const unsigned long *, size_t));
void resCheck(int);

int main(int argc, char *argv[])
{
    // ============ Command Line Arguments
    // Here, we're gonna get the command line arguments we need. We'll
    // first check if we have the right usage, then we'll convert them
    // to numeric types, checking for conversion errors.

    // Arguments Check, Usage Message
    if (argc != 3) {
        fprintf(stderr, "Usage: %s %s %s\n",
                argv[0], "N", "M");
        fprintf(stderr, "  %s\t%s\n", "N ", "integer length of sets");
        fprintf(stderr, "  %s\t%s\n", "M ", "integer maximum value");
        return 2;
    }

    // Get Numeric Arguments
    errno = 0;
    size = strtoull(argv[1], NULL, 10);
    if (errno != 0) {
        fprintf(stderr, "N Argument [1]: %s\n", strerror(errno));
        return 2;
    }

    errno = 0;
    max = strtoul(argv[2], NULL, 10);
    if (errno != 0) {
        fprintf(stderr, "M Argument [2]: %s\n", strerror(errno));
        return 2;
    }

    // General Input Errors
    if (max < size) {
        fprintf(stderr, "M cannot be less than N\n");
        return 2;
    }

    if (size == 0) {
        fprintf(stderr, "N = 0 makes no sense\n");
        return 2;
    }

    // ============ Initialize Set Records
    // Now we're going to initialize the data structure for keeping
    // track of sets. We're going to create a set record for each length
    // of set from 1 to N.

    // Allocate Array Space
    rec = calloc(size, sizeof(SR_Base *));

    // Create all Set Records
    for (size_t i = 0; i < size; i++)
    {
        rec[i] = sr_initialize(i + 1, max);

        if (rec[i] == NULL) {
            fprintf(stderr, "Unable to Allocate Data Structure\n");
            return 1;
        }
    }

    printf("Instantiated with N = %lu and M = %lu\n\n",
            size, max);

    // ============ Enumerate a Bunch of Nullifiable Sets
    // We know already that in order to be nullifiable, a set must have
    // two ways of getting to the same value. So here we'll use the
    // equivalent pairs program to expand a set containing two of the
    // same number, which we know is definitely nullifiable. This will
    // give us a bunch more nullifiable sets, which we'll mark off in
    // the data structure. From there, we'll expand those expansions,
    // until we've expanded all the way up to the final set length.

    // Since supersets of nullifiable sets are also nullifiable, we can
    // also mark supersets in the data structure. We also can ignore
    // those supersets later on when we expand all the sets of that
    // size, as we know that the expansion of those will have already
    // been covered earlier.

    // Configure the program
    eqSetsInit(max);

    printf("Generating Nullifiable Sets...\n");

    // Trivial sets for each allowed value
    if (size > 2) for (unsigned long n = 1; n <= max; n++)
    {
        unsigned long minNulSet[2];
        minNulSet[0] = n;
        minNulSet[1] = n;

        expandNul(minNulSet, 2);
    }

    // Iteratively Expand Nullifiable Sets by One Element
    for (size_t setSize = 3; setSize < size; setSize++)
    {
        printf("Expanding Size %lu\n", setSize);

        // Make sure we only get the sets that are marked nullifiable
        // and also not a superset of a smaller nullifiable set
        sr_query(rec[setSize - 1], MARKED, NULLIF, &expandNul);
    }

    printf("Done\n\n");

    // Deallocate the dynamic memory
    eqSetsInit(0);

    // ============ Verify and Print Sets
    // The Equivalent Sets program is not completely perfect, and it
    // cannot mark all nullifiable sets. So, we need to manually and
    // exhaustively check each set before we can definitively say it's
    // innullifiable.

    printf("Testing Remaining Sets...\n");

    // Test the sets that remain after Equivalent Sets
    long long remaining = retrieve(&verify);
    printf("Done\n\n");

    // Now, everything unmarked is Innullifiable
    long long finals = retrieve(&printSet);

    printf("\n%lld Innullifiable Sets, %lld Passed Equivalent Sets\n",
            finals, remaining);

    // ============ Release Set Records
    for (size_t i = 0; i < size; i++) sr_release(rec[i]);
    free(rec);

    return 0;
}

// ================ Supplemental Functions

// Supplemental Function for Expanding a Nullifiable Set
void expandNul(const unsigned long *set, size_t setc)
{
    // Expand Set and Eliminate it
    eqSets(set, setc, &eliminate);

    return;
}

// Supplemental Function for Eliminating a Set/Subsets
void eliminate(const unsigned long *set, size_t setc)
{
    // Check Set Size
    if (setc > size)
    {
        fprintf(stderr, "This really shouldn't be happening.\n");
        exit(16);
    }

    // Mark this Particular Set as Nullifiable
    int res = sr_mark(rec[setc - 1], set, setc, NULLIF, NULLIF);
    resCheck(res);

    // If we haven't seen this already, mark all its supersets
    if (res == 1) for (size_t i = setc; i < size; i++)
    {
        res = sr_mark(rec[i], set, setc, SUPERSET, SUPERSET);
        resCheck(res);
    }

    return;
}

// Supplemental Function for Verifying a Set
void verify(const unsigned long *set, size_t setc)
{
    // Run the test
    int res = nulTest(set, setc);

    // Handle an error, just in case
    if (res == -1) {
        fprintf(stderr, "Memory Error while Verifying Sets\n");
        exit(1);
    }

    // Eliminate if nullifiable
    else if (res == 0) eliminate(set, setc);

    return;
}

// Supplemental Function for Printing a Set to the Standard Output
void printSet(const unsigned long *set, size_t setc)
{
    for (size_t i = 0; i < setc; i++)
        printf("%4lu", set[i]);
    printf("\n");
}

// ================ Helper Functions

// Retrieve all Completely Unmarked Sets
long long retrieve(void (*out)(const unsigned long *, size_t))
{
    // Query for Sets with Neither Mark, Output
    long long res = sr_query(rec[size - 1], MARKED, 0, out);

    return res;
}

// Check Result of Mark Function
void resCheck(int res)
{
    // Handle an error, just in case
    if (res == -1) {
        fprintf(stderr, "Memory Error while Marking Sets\n");
        exit(1);
    }
    else if (res == -2) {
        fprintf(stderr, "This really shouldn't be happening.\n");
        exit(16);
    }

    return;
}
