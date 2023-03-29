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

#include "setTree.h"
#include "eqSets.h"
#include "nulTest.h"

// Size of Sets (N)
size_t size;

// Maximum Element Value (M)
unsigned long max;

// Base Pointer to Data Structure for Keeping Track of Sets
Base *sets;

// Supplementary Function Declarations
bool eliminate(const unsigned long *, size_t);
void verify(const unsigned long *, size_t);
void printSet(const unsigned long *, size_t);

int main(int argc, char *argv[])
{
    // ============ Command Line Arguments
    // Here, we're gonna get the command line arguments we need. We'll
    // first check if we have the right usage, then we'll convert them
    // to numeric types, checking for conversion errors.
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <N (set length)> <M (max value)>\n",
                argv[0]);
        return 2;
    }

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

    // ============ Construct Tree
    // Now we're going to construct the tree for keeping track of all
    // our sets.
    sets = treeConstruct(size, max, ALLOC_STATIC);
    if (sets == NULL) {
        fprintf(stderr, "Unable to Allocate Tree\n");
        return 1;
    }
    printf("Tree Constructed with N = %llu and M = %lu\n\n", size, max);

    // ============ Enumerate a Bunch of Nullifiable Sets
    // We know already that in order to be nullifiable, a set must have
    // two ways of getting to the same value. So here we'll use the
    // equivalent pairs program to expand a set containing two of the
    // same number, which we know is definitely nullifiable. This will
    // give us a bunch more nullifiable sets, which we'll mark off in
    // the tree.

    // Initialize the program
    eqSetsInit(size, max);

    printf("Generating Nullifiable Sets...\n");

    // Trivial sets for each allowed value
    for (unsigned long n = 1; n <= max; n++)
    {
        unsigned long minNulSet[2];
        minNulSet[0] = n;
        minNulSet[1] = n;

        printf("%lu...", n);
        fflush(stdout);

        eqSets(minNulSet, 2, &eliminate);
    }

    printf("Done\n\n");

    // Deallocate the dynamic memory
    eqSetsInit(0, 0);

    // ============ Verify and Print Sets
    // The Equivalent Sets program is not completely perfect, and it
    // cannot mark all nullifiable sets. So, we need to traverse the
    // tree and just manually check each set before we can definitively
    // say it's innullifiable.

    printf("Testing Remaining Sets...\n");

    // Check the sets that remain after Equivalent Sets
    long long remaining = treeQuery(sets, QUERY_SETS_UNMARKED, &verify);
    if (remaining == -1) {
        fprintf(stderr, "Memory Error on Querying Tree\n");
        return 1;
    }

    printf("Done\n\n");

    // Now, everything unmarked is Innullifiable
    long long finals = treeQuery(sets, QUERY_SETS_UNMARKED, &printSet);
    if (finals == -1) {
        fprintf(stderr, "Memory Error on Querying Tree\n");
        return 1;
    }

    printf("\n%lld Innullifiable Sets, %lld Passed Equivalent Sets\n",
            finals, remaining);

    // ============ Deallocate Tree
    treeDestruct(sets);

    return 0;
}

// ================ Supplemental Functions

// Supplemental Function for Eliminating a Set/Subsets

// When the equivalent sets program comes up with a nullifiable set, it
// gets sent to this function, which uses a function from the set tree
// library to mark it or its supersets (supersets of a nullifiable set
// are nullifiable as well).
bool eliminate(const unsigned long *pSet, size_t pSetc)
{
    // Mark anything matching this pattern on the data structure
    int res = treeMark(sets, pSet, pSetc);

    // Handle an error, just in case
    if (res == -1) {
        fprintf(stderr, "Memory Error while Marking Sets\n");
        exit(1);
    }
    else if (res == -2) {
        fprintf(stderr, "This really shouldn't be happening.\n");
        exit(16);
    }

    return res == 1;
}

// Supplemental Function for Verifying a Set

// Since equivalent sets can't catch everything, we'll run everything
// through this, which will run the exhaustive nullifiability test on
// every set it receives and then eliminate it if it needs to be.
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
        printf("%4d", set[i]);
    printf("\n");
}
