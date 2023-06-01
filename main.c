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
// So instead, this program works backwards, generating a bunch of
// nullifiable sets by considering every single way that a particular
// value might have been reached. For example, given that the set
// (1, 3, 4) is nullifiable, we could then generate a bunch of other
// nullifiable sets by expanding the 3 into 5 and 2 (5 - 2 = 3), and all
// the other 'equivalent pairs' of 3. By generating sets instead of
// testing sets, we can eliminate a whole lot more sets faster than
// running a test over every set.

// The program starts with pairs of the same number, which are known to
// be nullifiable. Then it expands those into nullifiable sets of length
// three, and marks off those sets. It can also mark off sets which are
// supersets of those sets. After the generation is done, the program
// then expands those length-three sets into length-four nullifiable
// sets. Then, if a set is already marked as being a superset of a
// smaller nullifiable set, it knows it doesn't need to bother expanding
// it as well, as the parent set has already been expanded as much as it
// can, and it would cover all the same sets anyways. This process
// continues until the final generation of N-length nullifiable sets.

// The program keeps track of the sets by way of a series of Set
// Records, which are just arrays that store some data for every
// possible set. It creates a record for each set length, and uses it to
// store sets from each generation. It can store whether a set has been
// marked nullifiable, as well as whether that was due to being a
// superset, so that the program can detect and avoid redundantly
// expanding it.

// This method of expanding sets doesn't cover everything, and it lets
// some nullifiable sets through. So, after the final generation is
// made, the program runs the remaining sets through an exhaustive test,
// to weed out sets which are nullifiable which couldn't be caught by
// nullifiable set expansion. In particular, this arises because the
// expansion method stays within the range of valid set values, and some
// sets might be only nullifiable through a calculation that goes beyond
// that range.

// The process of generation can be described more generally in two
// steps. First, any sets that are marked at all on the source record
// have their supersets marked on the destination record. Second, any
// sets on the source record that are marked as new nullifiable sets
// (not as a superset) are expanded using equivalent sets and marked in
// the destination normally. Then, if desired, a final sweep may go
// through the record to remove any remaining nullifiable sets.

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <errno.h>
#include <pthread.h>

#include "setRec/setRec.h"
#include "eqSets/eqSets.h"
#include "nulTest/nulTest.h"

// Bitmasks for Set Records
#define NULLIF      1 << 0
#define SUPERSET    NULLIF | 1 << 1
#define MARKED      NULLIF | SUPERSET

// Size of Sets (N)
size_t size;

// Maximum Element Value (M)
unsigned long max;

// Number of Threads
unsigned long threads = 1;

// Set Records for Each Length 1-N
SR_Base **rec;

// Thread Initializer Argument Structure
typedef struct ThreadArg ThreadArg;
struct ThreadArg {
    SR_Base *rec;
    char mask;
    char bits;
    size_t mod;
    void (*out)(const unsigned long *, size_t);
    ssize_t res;
};

// Thread Function Declarations
ssize_t threadedQuery(SR_Base *, char, char,
        void (*)(const unsigned long *, size_t));
void *initThread(void *);

// Supplementary Function Declarations
void eliminate(const unsigned long *, size_t);
void expandNul(const unsigned long *, size_t);
void verify(const unsigned long *, size_t);
void printSet(const unsigned long *, size_t);

ssize_t retrieve(void (*)(const unsigned long *, size_t), bool);
void resCheck(int);

int main(int argc, char *argv[])
{
    // ============ Command Line Arguments
    // Here, we're gonna get the command line arguments we need. We'll
    // first check if we have the right usage, then we'll convert them
    // to numeric types, checking for conversion errors.

    // Arguments Check, Usage Message
    if (argc < 3) {
        fprintf(stderr, "Usage: %s %s %s %s\n",
                argv[0], "N", "M", "[T]");
        fprintf(stderr, "  %s\t%s\n", "N ", "integer length of sets");
        fprintf(stderr, "  %s\t%s\n", "M ", "integer maximum value");
        fprintf(stderr, "  %s\t%s\n", "T ", "number of threads");
        return 2;
    }

    // Get Numeric Arguments
    errno = 0;
    size = strtoul(argv[1], NULL, 10);
    if (errno != 0) {
        perror("N Argument [1]");
        return 2;
    }

    errno = 0;
    max = strtoul(argv[2], NULL, 10);
    if (errno != 0) {
        perror("M Argument [2]");
        return 2;
    }

    errno = 0;
    if (argc > 3) threads = strtoul(argv[3], NULL, 10);
    if (errno != 0) {
        perror("T Argument [3]");
        return 2;
    }

    // General Input Errors
    if (max < size) {
        fprintf(stderr, "Input: M cannot be less than N\n");
        return 2;
    }

    if (size == 0) {
        fprintf(stderr, "Input: N = 0 makes no sense\n");
        return 2;
    }

    if (threads == 0) {
        fprintf(stderr, "Input: Must have at least 1 Thread\n");
        return 2;
    }

    // ============ Initialize Set Records
    // Now we're going to initialize the data structure for keeping
    // track of sets. We're going to create a set record for each length
    // of set from 1 to N.

    // Allocate Array Space
    rec = calloc(size, sizeof(SR_Base *));
    if (rec == NULL) {
        perror("Unable to Allocate Data Structure");
        return 1;
    }

    // Create all Set Records
    for (size_t i = 0; i < size; i++)
    {
        rec[i] = sr_initialize(i + 1, max);

        if (rec[i] == NULL) {
            perror("Unable to Allocate Data Structure");
            return 1;
        }
    }

    printf("Instantiated with N = %lu and M = %lu\n\n",
            size, max);

    // ============ Enumerate a Bunch of Nullifiable Sets
    // We know that a set is nullifiable if and only if it has two ways
    // of getting to the same value. So here we'll use the equivalent
    // pairs program to expand a set containing two of the same number,
    // which we know is definitely nullifiable. This will give us a
    // bunch more nullifiable sets, which we'll mark off in the data
    // structure. From there, we'll expand those expansions, until we've
    // expanded all the way up to the final set length.

    // Since supersets of nullifiable sets are also nullifiable, we can
    // also mark supersets in the data structure. We also can ignore
    // those supersets later on when we expand all the sets of that
    // size, as we know that the expansion of those will have already
    // been covered earlier.

    // Configure the program
    eqSetsInit(max);

    printf("Generating Nullifiable Sets...\n");

    ssize_t remaining;
    remaining = retrieve(NULL, true);
    printf("At starting...     ");
    printf("%16ld Sets Remain\n", remaining);

    // Trivial sets for each allowed value
    printf("Expanding Size %lu...", 2ul);
    fflush(stdout);
    if (size > 2) for (unsigned long n = 1; n <= max; n++)
    {
        // A Pair is Nullifiable
        unsigned long minNulSet[2];
        minNulSet[0] = n;
        minNulSet[1] = n;

        // Expand
        expandNul(minNulSet, 2);
    }
    remaining = retrieve(NULL, true);
    printf("%16ld Sets Remain\n", remaining);

    // Iteratively Expand Nullifiable Sets by One Element
    for (size_t setSize = 3; setSize < size; setSize++)
    {
        printf("Expanding Size %lu...", setSize);
        fflush(stdout);

        // Make sure we only get the sets that are marked nullifiable
        // and also not a superset, start the chain of functions
        threadedQuery(rec[setSize - 1], MARKED, NULLIF, &expandNul);

        // Get Number of Sets Remaining
        remaining = retrieve(NULL, true);
        printf("%16ld Sets Remain\n", remaining);
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

    remaining = retrieve(NULL, true);

    // Test the sets that remain after Equivalent Sets
    retrieve(&verify, true);
    printf("Done\n\n");

    // Now, everything unmarked is Innullifiable
    ssize_t finals = retrieve(&printSet, false);

    printf("\n%ld Innullifiable Sets, %ld Passed Equivalent Sets\n",
            finals, remaining);

    // ============ Release Set Records
    for (size_t i = 0; i < size; i++) sr_release(rec[i]);
    free(rec);

    return 0;
}

// Perform a Threaded Query to a Set Record
ssize_t threadedQuery(SR_Base *rec, char mask, char bits,
        void (*out)(const unsigned long *, size_t))
{
    // Arrays for Threads and Thread Initializer Arguments
    pthread_t *th = calloc(threads - 1, sizeof(pthread_t));
    ThreadArg *args = calloc(threads - 1, sizeof(ThreadArg));
    if (th == NULL || args == NULL) {
        perror("Memory Error while Expanding Sets");
        exit(1);
    }

    // Iteratively Create T - 1 Threads (don't count the current one)
    for (unsigned long i = 0; i < threads - 1; i++)
    {
        // Provide Arguments
        args[i].rec = rec;
        args[i].mask = mask;
        args[i].bits = bits;
        args[i].mod = i + 1;
        args[i].out = out;

        // Create thread, enter from init function
        int res = pthread_create(th + i, NULL, &initThread,
                (void *) (args + i));

        // Catch any Error
        if (res) {
            perror("Failed to Create Thread");
            exit(1);
        }
    }

    // Use this current thread for the zero-offset
    ThreadArg arg = {rec, mask, bits, 0, out, 0};
    initThread((void *) &arg);

    // For storing the end result
    ssize_t reses = arg.res;

    // Iteratively Join Threads, Keeping Results
    for (unsigned long i = 0; i < threads - 1; i++)
    {
        // Join a Thread
        int res = pthread_join(th[i], NULL);

        // Catch any Error
        if (res) {
            perror("Failed to Join Thread");
            exit(1);
        }

        // Deal with Result: Propogate Error or Add Counters
        if (args[i].res == -1 || reses == -1) reses = -1;
        else reses += args[i].res;
    }

    // Free Memory
    free(th);
    free(args);

    return reses;
}

// Initialize a Thread for a Query
void *initThread(void *argument)
{
    // Get Mod Value from Argument
    ThreadArg *arg = (ThreadArg *) argument;

    // Query our own sets, expand them, and eliminate them
    ssize_t res = sr_query_parallel(arg->rec, arg->mask, arg->bits,
            threads, arg->mod, arg->out);

    // Set Result
    arg->res = res;

    return NULL;
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
    int res = sr_mark(rec[setc - 1], set, setc, NULLIF);
    resCheck(res);

    // If we haven't seen this already, mark all its supersets
    if (res == 1) for (size_t i = setc; i < size; i++)
    {
        res = sr_mark(rec[i], set, setc, SUPERSET);
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
ssize_t retrieve(void (*out)(const unsigned long *, size_t),
        bool threaded)
{
    // Query for Sets with Neither Mark, Output
    ssize_t res;
    if (threaded)
        res = threadedQuery(rec[size - 1], MARKED, 0, out);
    else
        res = sr_query(rec[size - 1], MARKED, 0, out);

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
