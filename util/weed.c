// =============================== WEED ================================

// Copyright (c) 2023, Jacob Bates
// SPDX-License-Identifier: BSD-2-Clause

// This program takes in a record, and 'weeds out' all the remaining
// unmarked nullifiable sets. It will iteratively apply the exhaustive
// test and mark any sets that fail.

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
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
const char *usage = "Usage: %s [-v] recSize rec.dat [threads]\n";

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

    // ============ Import Record
    rec = sr_initialize(size);
    if (rec == NULL) {
        perror("Record Initialization Error");
        return 1;
    }

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
        if (progv == NULL) {
            perror("Progress Variables");
            return 1;
        }

        // Iteratively Create Threads
        for (size_t i = 0; i < threads; i++) {
            errno = pthread_create(th + i, NULL, &threadOp,
                    (void *) (progv + i));
            if (errno) {
                perror("Thread Creation");
                return 1;
            }
        }

        // Create Signal Handler Thread
        pthread_t handler;
        errno = pthread_create(&handler, NULL, &threadHandler, NULL);
        if (errno) {
            perror("Thread Creation");
            return 1;
        }

        // Iteratively Join Threads
        for (size_t i = 0; i < threads; i++) {
            errno = pthread_join(th[i], NULL);
            if (errno) {
                perror("Thread Joining");
                return 1;
            }
        }

        // Cancel Handler Thread
        errno = pthread_cancel(handler);
        if (errno) {
            perror("Thread Cancellation");
            return 1;
        }

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
    void testElim(const unsigned long *, size_t);
    _Noreturn void tryExitFail(void);

    // Argument is a Reference for Progress Output
    size_t *prog = (size_t *) arg;

    // Get Thread Number
    size_t mod = prog - progv;

    // For every unmarked set, run exhaustive test
    ssize_t res = sr_query_parallel(rec, NULLIF, 0,
            threads, mod, prog, &testElim);
    assert(res != -2);
    if (res == -1) {
        perror("Error");
        tryExitFail();
    }

    return NULL;
}

// Individual Set Testing/Elimination
void testElim(const unsigned long *set, size_t setc)
{
    _Noreturn void tryExitFail(void);

    int res;
    assert(setc == size);

    // Run the Test
    res = nulTest(set, setc);
    if (res == -1) {
        perror("Test Error");
        tryExitFail();
    }

    // Eliminate if Nullifiable
    if (res == 0) {
        res = sr_mark(rec, set, setc, NULLIF);
        assert(res != -2);
    }

    return;
}

// Thread Function for Intercepting Signals
void *threadHandler(void *arg)
{
    void progHandler(int);

    // Set up Handler and Unblock Signal
    struct sigaction act = {0};
    act.sa_handler = &progHandler;
    sigaction(SIGUSR1, &act, NULL);

    pthread_sigmask(SIG_UNBLOCK, &progmask, NULL);

    // Just wait
    while (1) pause();

    return NULL;
}

// Progress Signal Handler
void progHandler(int signo)
{
    if (signo != SIGUSR1) return;
    if (progFname == NULL) return;

    // Open Progress File
    int fd = open(progFname, O_WRONLY | O_TRUNC);
    if (fd == -1) {
        perror("Progress File");
        exit(1);
    }

    // Sum of Progress
    size_t prog = 0;
    for (size_t i = 0; i < threads; i++) prog += progv[i];

    // Raw Number Buffers, Fixed Size
    uint64_t buf[2] = {prog, total};

    // Output Progress
    write(fd, buf, sizeof(buf));

    // Close Progress File
    if (close(fd)) {
        perror("Progress File");
        exit(1);
    }

    return;
}

// Exit with a Fail Code
_Noreturn void tryExitFail(void)
{
    // exit() depends on global var(s), not thread-safe
    static pthread_mutex_t exitLock = PTHREAD_MUTEX_INITIALIZER;

    pthread_mutex_lock(&exitLock);
    exit(1);
}
