// =============================== WEED ================================

// Copyright (c) 2023, Jacob Bates
// SPDX-License-Identifier: BSD-2-Clause

// This program takes in a record, and 'weeds out' all the remaining
// unmarked nullifiable sets. It will iteratively apply the exhaustive
// test and mark any sets that fail.

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

#include "../lib/iface.h"
#include "../lib/setRec.h"
#include "../lib/nulTest.h"

// Set Record
SR_Base *rec = NULL;
size_t size;
char *fname;
size_t total;

// Number of Threads
size_t threads = 1;

// Progress
volatile size_t *progv = NULL;
char *progFname = NULL;
sigset_t progmask;

// Options
bool verbose;

// Usage Format String
const char *usage =
        "Usage: %s [-v] recSize rec.dat [threads [prog.out]]\n";

int main(int argc, char **argv)
{
    // ============ Command-Line Arguments

    // Parse arguments, show usage on invalid
    {
        const Param params[5] = {PARAM_SIZE, PARAM_FNAME, PARAM_CT,
                PARAM_FNAME, PARAM_END};
        int res;

        res = argParse(params, 2, argc, argv,
                &size, &fname, &threads, &progFname);
        if (res) {
            fprintf(stderr, usage, argv[0]);
            return 1;
        }

        res = optHandle("v", true, argc, argv, &verbose);
        if (res) {
            fprintf(stderr, usage, argv[0]);
            return 1;
        }
    }

    // Validate Thread Count
    if (threads < 1) {
        fprintf(stderr, "Error: Must use at least 1 thread\n");
        return 1;
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

    // ============ Import Record
    rec = sr_initialize(size);
    CK_PTR(rec);

    if (openImport(rec, fname)) return 1;
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
    if (openExport(rec, fname)) return 1;

    sr_release(rec);

    return 0;
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

// Individual Set Testing/Elimination
void testElim(const unsigned long *set, size_t setc, char bits)
{
    int res;

    // Run the Test
    res = nulTest(set, setc);
    CK_RES(res);

    // Eliminate if Nullifiable
    if (res == 0) {
        res = sr_mark(rec, set, setc, NULLIF);
        CK_RES(res);
    }

    return;
}

// Thread Function for Intercepting Signals
void *threadHandler(void *arg)
{
    // Unblock the signal and just wait
    pthread_sigmask(SIG_UNBLOCK, &progmask, NULL);
    while (1) pause();

    return NULL;
}

// Progress Signal Handler
void progHandler(int signo)
{
    if (signo != SIGUSR1) return;
    if (progFname == NULL) return;

    // Sum of Progress
    size_t prog = 0;
    for (size_t i = 0; i < threads; i++) prog += progv[i];

    // Push Progress Update
    if (pushProg(prog, total, progFname)) FAULT();

    return;
}
