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

// TODO explain the different libraries and the basic gist of how the
// program works

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
void eliminate(const unsigned long *, size_t);
void printSet(const unsigned long *, size_t);

int main(int argc, char *argv[])
{
    // ============ Command Line Arguments
    // Here, we're gonna get the command line arguments we need. We'll
    // first check if we have the right usage, then we'll convert them
    // to numeric types, checking for conversion errors.
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <N-val> <M-val>\n", argv[0]);
        return 2;
    }

    errno = 0;
    size = strtoull(argv[1], NULL, 10);
    if (errno != 0) {
        fprintf(stderr, "N-val argument: %s\n", strerror(errno));
        return 2;
    }

    max = strtoul(argv[2], NULL, 10);
    if (errno != 0) {
        fprintf(stderr, "M-val argument: %s\n", strerror(errno));
        return 2;
    }

    // ============ Set Tree Construction
    // Now we're going to construct the tree for keeping track of all
    // our sets.
    sets = treeConstruct(size, max);
    if (sets == NULL) {
        fprintf(stderr, "Unable to Allocate Tree\n");
        return 1;
    }
    printf("Tree Constructed with N = %u and M = %u\n", size, max);

    // ============ Enumerate Equivalent Sets
    eqSetsInit(size, max);

    unsigned long minNulSet[2];
    for (unsigned long n = 1; n <= max; n++)
    {
        minNulSet[0] = n;
        minNulSet[1] = n;
        eqSets(minNulSet, 2, &eliminate);
    }

    // ============ What Sets are Left?
    long long remaining = treeQuery(sets, QUERY_SETS_UNMARKED, &printSet);
    printf("%lld sets remain\n", remaining);

    return 0;
}

// Function for Eliminating a Set/Subsets

// When the equivalent sets program comes up with a nullifiable set, it
// gets sent to this function, which uses a function from the set tree
// library to mark it or its supersets (supersets of a nullifiable set
// are nullifiable as well).
void eliminate(const unsigned long *pSet, size_t pSetc)
{
    // Mark anything matching this pattern on the data structure
    int ret = treeMark(sets, pSet, pSetc);

    // Handle an error, just in case
    if (ret == -1) {
        fprintf(stderr, "Memory Error while Marking Sets\n");
        exit(1);
    }
    else if (ret == -2) {
        fprintf(stderr, "This really shouldn't be happening.\n");
        exit(16);
    }

    return;
}

// Function for Printing a Set to the Standard Output
void printSet(const unsigned long *set, size_t setc)
{
    for (size_t i = 0; i < setc; i++)
        printf("%4d", set[i]);
    printf("\n");
}
