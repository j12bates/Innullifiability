// ======================== TOP-DOWN PROCESSING ========================

// Copyright (c) 2023-25, Jacob Bates
// SPDX-License-Identifier: BSD-2-Clause

// This program does reductive, top-down work in searching for
// innullifiable sets. It takes in one destination record, and it scans
// across, reductively testing sets whose nullifiability isn't known.

// After performing an ideal expansion, marking off all supersets of
// nullifiable sets, there is no reason to test for bisectable subsets,
// so this program by default tests bisectability only. Usually we don't
// care that we know the bisectability of every set, including the
// supersets, so we only perform the test by default on unmarked sets,
// not a superset and not an already known mutation.

// If the ideal expansion was infeasible though, we can enable subset
// testing. We can do this in a 'strong' or 'weak' way. In the strong
// case, for any remaining set, the program will conclude whether or not
// it has a nullifiable subset, and optionally if it's bisectable. In
// the weak case, the program will mark down whatever it discovers
// first. So if one cares about precarious sets, a strong test would be
// ideal as it would take care of marking off any nullifiable supersets.

// In general, any remaining nullifiable set is guaranteed to have a
// nullifiable first-order contraction. In the case of the ideal
// expansion, we can further say that this guaranteed set will be
// precarious. If we thoroughly mutated all precarious sets in a range
// and marked off the results, we can even further say that the set
// won't be in that expansion range. We can simply configure the test to
// discard first-order contractions that fall within that range.

// Bisectability is marked off as being tested regardless of the result.
// This way, if a test were to be interrupted, it can be resumed without
// redoing all the work to try to bisect a non-bisectable set. There may
// also be some rare indeterminate cases where a full enumeration of
// contractions wasn't possible, so for these sets the 'tested' mark is
// not given. In some case where a mistake was made in configuring the
// expanded range, there is an option to ignore any marking of known
// non-bisectability, and simply re-test any set that isn't marked as
// being bisectable.

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

#include "../lib/iface.h"
#include "../lib/reduce.h"
#include "../lib/setRec.h"
#include "../lib/tests.h"

// Set Record
SR_Base *rec = NULL;
size_t size;
char *fname;
size_t total;

// Initial Reduction M-range
unsigned long minm = 0, maxm = 0;
bool inRange = false;
bool testSubs = false;
size_t progrSize = 4;

// By default, test sets we haven't nullified
char mask = TESTED_BISECT | SUPER;
char bits = 0;

// Number of Threads
size_t threads = 1;

// Progress
volatile size_t *progv = NULL;
char *progFname = NULL;
sigset_t progmask;

// Options
bool subStrong, subWeak;
bool testAllUntested, reTest;

// Progress Options
bool progExport, intProg;

// Usage Format String
const char *usage =
        "Usage: %s [-swurxi] recSize rec.dat [minm maxm threads "
                "[prog.out]]\n"
        "By default, the program tests any totally unmarked sets for "
                "bisectability only,\n"
        "assuming thorough mutative expansion from the given M-range.\n"
        "The following options open up testing for nullifiable "
                "subsets:\n"
        "   -s      Halt Only on Concluding Subset Test (Strong)\n"
        "   -w      Halt on Positive Result for Either Subsets or "
                "Bisectability (Weak)\n"
        "Bisectability Testing Options:\n"
        "   -u      Test All Sets with Unknown Bisectability\n"
        "   -r      Re-Test Any Sets Marked as Non-Bisectable\n"
        "Progress Updates:\n"
        "   -x      Export Current Output Record on Progress Update\n"
        "   -i      Generate Progress Update on Interrupt\n";

int main(int argc, char **argv)
{
    // ============ Command-Line Arguments

    // Parse arguments, show usage on invalid
    {
        const Param params[7] = {PARAM_SIZE, PARAM_FNAME,
                PARAM_VAL, PARAM_VAL, PARAM_CT, PARAM_FNAME, PARAM_END};

        CK_IFACE_FN(argParse(params, 2, usage, argc, argv,
                &size, &fname, &minm, &maxm, &threads, &progFname));

        CK_IFACE_FN(optHandle("swurxi", true, usage, argc, argv,
                &subStrong, &subWeak, &testAllUntested, &reTest,
                &progExport, &intProg));
    }

    // Validate Thread Count
    if (threads < 1) {
        fprintf(stderr, "Error: Must use at least 1 thread\n");
        return 1;
    }

    // If our set size is smaller than the implemented base cases,
    // automatically test subsets of all sizes (if required)
    if (size <= 4) progrSize = size - 1;

    // Set up the set selector bitmask
    if (subStrong && subWeak) {
        fprintf(stderr, "Error: Options -sw are Mutually Exclusive\n");
        return 1;
    }
    testSubs = subStrong || subWeak;
    if (testAllUntested) mask = TESTED_BISECT;
    if (reTest) mask = (mask & ~TESTED_BISECT) | BISECT;

    // Block Progress Signal
    sigemptyset(&progmask);
    sigaddset(&progmask, SIGUSR1);
    sigprocmask(SIG_BLOCK, &progmask, NULL);

    // Set up Handler for Progress
    {
        void progHandler(int);
        struct sigaction act = {0};

        act.sa_handler = &progHandler;
        sigaction(SIGUSR1, &act, NULL);
    }

    // Set up Handler for Interrupt
    {
        void intHandler(int);
        struct sigaction act = {0};

        act.sa_handler = &intHandler;
        sigaction(SIGINT, &act, NULL);
    }

    // ============ Import Record
    rec = sr_initialize(size);
    CK_PTR(rec);

    CK_IFACE_FN(openImport(rec, fname));
    total = sr_getTotal(rec);

    // ============ Iteratively Perform Test

    // Launch Threads to do the Computing
    {
        void *threadOp(void *);
        void *threadHandler(void *);

        // Arrays for Threads and Args
        pthread_t th[threads];
        progv = calloc(threads, sizeof(size_t));
        CK_PTR(progv);

        // Iteratively Create Threads
        for (size_t i = 0; i < threads; i++) {
            errno = pthread_create(th + i, NULL, &threadOp,
                    (void *) (progv + i));
            CK_NO(errno);
        }

        // Create Signal Handler Thread
        pthread_t handler;
        errno = pthread_create(&handler, NULL, &threadHandler, NULL);
        CK_NO(errno);

        // Iteratively Join Threads
        for (size_t i = 0; i < threads; i++) {
            errno = pthread_join(th[i], NULL);
            CK_NO(errno);
        }

        // Cancel Handler Thread
        errno = pthread_cancel(handler);
        CK_NO(errno);

        free((void *) progv);
        progv = NULL;
    }

    // ============ Export and Cleanup
    CK_IFACE_FN(openExport(rec, fname));

    sr_release(rec);

    return 0;
}

// Individual Set Nullifiability Testing/Marking

// Little global variable to keep track of bisectability before
// finishing the subset test (strong test)
_Thread_local bool g_bisectableMemory = false;

// Set comes here direct from the record.
void testElim(const unsigned long *set, size_t size, char bits)
{
    // The code 'c': bit 1 is bisectability, bit 2 is superset
    int res = 1, c = 0;
    int progrAndBase(const unsigned long *, size_t, size_t);
    int testElimSub(const unsigned long *, size_t);

    // Perform Preliminary Subset Tests. We can always terminate on
    // finding a nullifiable subset.
    if (testSubs) for (size_t i = 3; i <= progrSize && i < size; i++)
        c |= 2 * bisectSubs(set, size, i, size);
    if (c) goto mark;

    // If we're at the base-case, simply do the test. The subsets will
    // have been automatically tested (progrSize = 4)
    res = 0;
    if (size == 3) c |= bisect(set[0], set[1], set[2], 0, 3);
    else if (size == 4) c |= bisect(set[0], set[1], set[2], set[3], 4);

    // Larger sizes require reduction
    else
    {
        // Exhaustively run the test on recursively-generated
        // contractions
        g_bisectableMemory = false;
        res = contraction(set, size, minm, maxm, inRange, 4,
                &progrAndBase, &c);
        CK_RES(res);
        c |= g_bisectableMemory;
    }

mark:
    // Mark the appropriate bits in the record. If 'res' is zero, that
    // must mean the bisectability test was executed and wasn't faulty
    bool negative = !res && !c; // a definite negative test result
    char mark   = negative * (TESTED_BISECT)
                | !!(c & 1) * (TESTED_BISECT | BISECT | NULLIF)
                | !!(c & 2) * (SUPER | NULLIF);
    res = sr_mark(rec, set, size, mark);
    CK_RES(res);

    return;
}

// Progressive and Base-Case Tests

// This is where recursive contraction outputs all the sets to be
// tested.
int progrAndBase(const unsigned long *set, size_t size, size_t newIdx)
{
    bool bisectable = false, superset = false;

    // Perform Progressive New Subset Tests
    if (testSubs && progrSize < size)
        superset = bisectSubs(set, size, progrSize, newIdx);

    // Base-Case Test and Subset Tests
    if (size == 4) {
        bisectable = bisect(set[0], set[1], set[2], set[3], 4);
        if (testSubs) for (size_t i = progrSize + 1; i < 4; i++)
            superset |= bisectSubs(set, size, i, size);
    }

    // For a strong test, keep a bisectable result without interrupting
    // the contractions. Then it'll only return when a subset is found
    if (subStrong) {
        g_bisectableMemory |= bisectable;
        bisectable = 0;
    }

    return bisectable + 2 * superset;
}

// Thread Function for Testing Sets
void *threadOp(void *arg)
{
    void testElim(const unsigned long *, size_t, char);

    // Argument is a Reference for Progress Output
    size_t *prog = (size_t *) arg;

    // Get Thread Number
    size_t mod = prog - progv;

    // For every unmarked set, run exhaustive test
    ssize_t res = sr_query_parallel(rec, mask, bits,
            threads, mod, prog, &testElim);
    CK_RES(res);

    return NULL;
}

// ============ PROGRESS/SIGNALS

// Thread Function for Intercepting Signals
void *threadHandler(void *arg)
{
    // Unblock the signal and just wait
    pthread_sigmask(SIG_UNBLOCK, &progmask, NULL);
    while (true) pause();

    return NULL;
}

// Progress Signal Handler
void progHandler(int signo)
{
    if (signo != SIGUSR1) return;

    // Sum of Progress
    size_t prog = 0;
    for (size_t i = 0; i < threads; i++) prog += progv[i];

    // Push Progress Update
    if (progFname != NULL)
        if (pushProg(prog, total, 0, progFname))
            FAULT();

    // Export Record if Specified
    if (progExport) CK_IFACE_FN(openExport(rec, fname));

    return;
}

// Interrupt Handler
void intHandler(int signo)
{
    if (signo != SIGINT) return;

    // Generate Progress Update if Specified
    if (intProg) progHandler(SIGUSR1);

    // Exit the program
    safeExit();

    return;
}
