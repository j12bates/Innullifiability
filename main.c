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
// running a test over every set. This routine is referred to as
// 'expansion' in the program.

// The program keeps track of the sets by way of Set Records, which are
// just arrays that store some data for every possible set of a certain
// length. At any point, there are two records being used for generating
// sets: one 'destination' where all the nullifiable sets of the current
// length will be held, and one 'source' which was the result of the
// previous generation. Nullifiable sets can be stored by 'marking' them
// on a record.

// The program starts with pairs of the same number, which are known to
// be nullifiable. It can then expand those into nullifiable sets of
// length three, which it will mark in a record. From then on, sets can
// be further expanded, but in addition, supersets can be marked in as
// well. Importantly, a superset of a nullifiable set does not need to
// be expanded, as the parent set is already being expanded, and
// expanding the superset won't give anything new. So, these supersets
// are marked with an additional flag in the record, while 'new' sets
// aren't. Only the new sets need to be expanded in the next generation.

// The process of generation can be described more generally in two
// steps. First, any sets that are marked at all on the source record
// have their supersets marked on the destination record. Second, any
// new nullifiable sets on the source record (not supersets) are
// expanded using equivalent sets and marked in the destination
// normally. The next generation uses this as the source record.

// This method of expanding sets doesn't cover everything, and it lets
// some nullifiable sets through. So, after the final generation is
// made, the program runs the remaining sets through an exhaustive test,
// to weed out sets which are nullifiable which couldn't be caught by
// nullifiable set expansion. In particular, this arises because the
// expansion method stays within the range of valid set values, and some
// sets might be only nullifiable through a calculation that goes beyond
// that range.

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <errno.h>
#include <assert.h>
#include <pthread.h>

#include "setRec/setRec.h"
#include "eqSets/eqSets.h"
#include "nulTest/nulTest.h"

// Bitmasks for Set Records
#define NULLIF      1 << 0
#define SUPERSET    NULLIF | 1 << 1
#define MARKED      NULLIF | SUPERSET

// Target Size of Sets (N)
size_t size;

// Maximum Element Value (M)
unsigned long max;

// Number of Threads
unsigned long threads = 1;

// Input/Output Files
const char *outFname = NULL;
const char *inFname = NULL;
size_t inSize;

// Set Records for Source and Destination of Generation
SR_Base *recSrc = NULL;
SR_Base *recDest = NULL;
size_t sizeSrc = 0;

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
void baseSets(void (*)(const unsigned long *, size_t));

// Supplementary Function Declarations
void expand(const unsigned long *, size_t);
void eliminate(const unsigned long *, size_t);
void super(const unsigned long *, size_t);
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
    if (argc < 4) {
        fprintf(stderr, "Usage: %s %s %s %s %s\n",
                argv[0], "N", "M", "T", "[outfile [infile inN]]");
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
    threads = strtoul(argv[3], NULL, 10);
    if (errno != 0) {
        perror("T Argument [3]");
        return 2;
    }

    // Get Output Filename
    if (argc > 4) outFname = argv[4];

    // Get Input Filename and Size
    errno = 0;
    if (argc > 6) {
        inFname = argv[5];
        inSize = strtoul(argv[6], NULL, 10);
        if (errno != 0) {
            perror("inN Argument [6]");
            return 2;
        }
    }

    // General Input Errors
    if (max < size) {
        fprintf(stderr, "Input: M cannot be less than N\n");
        return 2;
    }

    if (inSize > size) {
        fprintf(stderr, "Input: inN cannot be greater than N\n");
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

    printf("N = %lu, M = %lu\n\n", size, max);

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

    // Import Input if Supplied
    if (inFname != NULL)
    {
        // Allocate Record as Destination (will be moved over to source)
        sizeSrc = inSize;
        recDest = sr_initialize(sizeSrc, max);

        // Open Input File
        FILE *f = fopen(inFname, "rb");
        if (f == NULL) perror("Error while Importing");

        // If successful, try importing
        else {
            int res = sr_import(recDest, f);

            // Handle various errors
            if (res == -1) perror("Error while Importing");
            else if (res == -2) {
                fprintf(stderr, "Input Record is Wrong Size\n");
                exit(2);
            }
            else if (res == -3) {
                fprintf(stderr, "Invalid Input Record File\n");
                exit(2);
            }

            fclose(f);
        }
    }

    // If no input, start from the base
    else sizeSrc = 2;

    // Produce Iterative Generations
    for (; sizeSrc < size; sizeSrc++)
    {
        printf("Expanding Size %lu...", sizeSrc);
        fflush(stdout);

        // Previous Destination is new Source
        recSrc = recDest;

        // Allocate new Destination
        recDest = sr_initialize(sizeSrc + 1, max);

        // If we're just starting out, base from the trivial sets,
        // expanding them
        if (sizeSrc == 2) baseSets(&expand);

        // Otherwise, perform a normal generation with source and
        // destination
        else if (sizeSrc > 2)
        {
            // Get all the sets that are marked nullifiable and mark their
            // supersets
            threadedQuery(recSrc, NULLIF, NULLIF, &super);

            // Get all the new nullifiable sets and expand them
            threadedQuery(recSrc, MARKED, NULLIF, &expand);

            // Deallocate Source
            sr_release(recSrc);
        }

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

    sizeSrc = size - 1;

    // Test the sets that remain after Equivalent Sets
    printf("Testing Remaining Sets...\n");
    retrieve(&verify, true);
    printf("Done\n\n");

    // Now, everything unmarked is Innullifiable
    ssize_t finals = retrieve(&printSet, false);

    printf("\n%ld Innullifiable Sets, %ld Passed Equivalent Sets\n",
            finals, remaining);

    // Export Record to Binary File
    if (outFname != NULL)
    {
        printf("\nExporting to file %s...\n", outFname);

        // Open Output File
        FILE *f = fopen(outFname, "wb");
        if (f == NULL) perror("Error while Exporting");

        // If successful, try exporting
        else {
            if (sr_export(recDest, f) == -1)
                perror("Error while Exporting");
            else printf("Success\n");
            fclose(f);
        }
    }

    // Deallocate Final Set Record
    sr_release(recDest);

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

// Produce Trivial Nullifiable Sets (Simulated Query)
void baseSets(void (*out)(const unsigned long *, size_t))
{
    // Trivial nullifiable sets for each allowed value
    for (unsigned long n = 1; n <= max; n++)
    {
        // A Pair is Nullifiable
        unsigned long minNulSet[2];
        minNulSet[0] = n;
        minNulSet[1] = n;

        // Expand this Set
        out(minNulSet, 2);
    }
}

// ================ Supplemental Functions

// Supplemental Function: Expand a New Nullifiable Set into More
void expand(const unsigned long *set, size_t setc)
{
    // Expand Set and Eliminate it
    eqSets(set, setc, &eliminate);

    return;
}

// Supplemental Function: Eliminate a New Nullifiable Set in the
// Destination
void eliminate(const unsigned long *set, size_t setc)
{
    // Set must be size of destination
    assert(setc == sizeSrc + 1);

    // Mark this Particular Set as Nullifiable
    int res = sr_mark(recDest, set, setc, NULLIF);
    resCheck(res);

    return;
}

// Supplemental Function: Mark an Old Nullifiable Set's Supersets in the
// Destination
void super(const unsigned long *set, size_t setc)
{
    // Set must be shorter than size of destination
    assert(setc <= sizeSrc);

    // Mark Supersets
    int res = sr_mark(recDest, set, setc, SUPERSET);
    resCheck(res);

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
        res = threadedQuery(recDest, MARKED, 0, out);
    else
        res = sr_query(recDest, MARKED, 0, out);

    return res;
}

// Check Result of Mark Function
void resCheck(int res)
{
    // Handle a Memory Error
    if (res == -1) {
        fprintf(stderr, "Memory Error while Marking Sets\n");
        exit(1);
    }

    // Invalid input shouldn't be possible
    assert(res != -2);

    return;
}
