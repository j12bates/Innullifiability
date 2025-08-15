// ======================== TOP-DOWN PROCESSING ========================

// Copyright (c) 2023-25, Jacob Bates
// SPDX-License-Identifier: BSD-2-Clause

// This program takes in a record, and 'weeds out' all the remaining
// unmarked nullifiable sets. It will iteratively apply the exhaustive
// test and mark any sets that fail. It can also weed on a specific
// range for the initial reduction, meaning it'll only proceed to work
// on sets it's reduced to that M-range on the first go. This can be
// likened to performing a thorough expansion on a weeded record with a
// specific M-range, and in fact it has the same effect: every set that
// can be reduced to anything nullifiable in that range gets marked.

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
bool inRange = true;
bool testSubs = true;
size_t progrSize = 4;

// Number of Threads
size_t threads = 1;

// Progress
volatile size_t *progv = NULL;
char *progFname = NULL;
sigset_t progmask;

// Options
bool verbose;
bool progExport;
bool intProg;

// Usage Format String
const char *usage =
        "Usage: %s [-vxi] recSize rec.dat [minm maxm threads "
                "[prog.out]]\n"
        "   -v      Verbose: Display Progress Messages\n"
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

        CK_IFACE_FN(optHandle("vxi", true, usage, argc, argv,
                &verbose, &progExport, &intProg));
    }

    // Validate Thread Count
    if (threads < 1) {
        fprintf(stderr, "Error: Must use at least 1 thread\n");
        return 1;
    }

    // If our set size is smaller than the implemented base cases,
    // automatically test subsets of all sizes (if required)
    if (size <= 4) progrSize = size - 1;

    // Interpret a 'maxm' of 0 to mean no upper limit, so outside the
    // complementary 0-to-'minm' range
    if (maxm == 0) {
        if (minm > 0) maxm = minm - 1;
        minm = 0;
        inRange = false;
    }

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

    // Print Information about Execution
    if (verbose)
    {
        fprintf(stderr, "rec  - Size: %2zu; M: %4lu to %4lu\n",
                size, sr_getMinM(rec), sr_getMaxM(rec));
        fprintf(stderr, "Testing Unmarked Sets with %zu Threads\n",
                threads);
    }

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
    if (verbose) fprintf(stderr, "Writing Output Record...");
    CK_IFACE_FN(openExport(rec, fname));
    if (verbose) fprintf(stderr, "Done\n");

    sr_release(rec);

    return 0;
}

// Individual Set Nullifiability Testing/Marking
void testElim(const unsigned long *set, size_t size, char bits)
{
    // The code 'c': bit 1 is bisectability, bit 2 is superset
    int res = 1, c = 0;
    int progrAndBase(const unsigned long *, size_t, size_t);

    // Perform Preliminary Subset Tests
    if (testSubs) for (size_t i = 1; i <= progrSize; i++)
        c |= 2 * bisectSubs(set, size, i, size);

    // If we're at the base-case, simply do the test. The subsets will
    // have been automatically tested
    if (size == 3) c |= bisect(set[0], set[1], set[2], 0, 3);
    else if (size == 4) c |= bisect(set[0], set[1], set[2], set[3], 4);

    // Exhaustively run the test on recursively-generated contractions
    // for larger sizes
    else if (!c) {
        res = contraction(set, size, minm, maxm, inRange, 4,
                &progrAndBase, &c);
        CK_RES(res);
    }

    // Mark the appropriate bits in the record
    int mark    = ((!res && !c) || size <= 4) * (TESTED_BISECT)
                | !!(c & 1) * (TESTED_BISECT | BISECT | NULLIF)
                | !!(c & 2) * (ONLY_SUP | NULLIF);
    res = sr_mark(rec, set, size, mark);
    CK_RES(res);

    return;
}

// Progressive and Base-Case Tests
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
    ssize_t res = sr_query_parallel(rec, NULLIF, 0,
            threads, mod, prog, &testElim);
    CK_RES(res);

    return NULL;
}

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
