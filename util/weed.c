// =============================== WEED ================================

// Copyright (c) 2023, Jacob Bates
// SPDX-License-Identifier: BSD-2-Clause

// This program takes in a record, and 'weeds out' all the remaining
// unmarked nullifiable sets. It will iteratively apply the exhaustive
// test and mark any sets that fail.

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <assert.h>
#include <errno.h>
#include <pthread.h>

#include "../lib/iface.h"
#include "../lib/setRec.h"
#include "../lib/nulTest.h"

// Number of Threads
size_t threads = 1;

// Set Record
SR_Base *rec = NULL;
size_t size;
char *fname;

// Options
bool verbose;

// Usage Format String
const char *usage = "Usage: %s [-v] recSize rec.dat [threads]\n";

int main(int argc, char **argv)
{
    // ============ Command-Line Arguments

    // Parse arguments, show usage on invalid
    {
        const Param params[4] = {PARAM_SIZE, PARAM_FNAME, PARAM_CT,
                PARAM_END};
        int res;

        res = argParse(params, 2, argc, argv,
                &size, &fname, &threads);
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

    // ============ Import Record
    rec = sr_initialize(size);

    if (openImport(rec, fname)) return 1;

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

        // VLAs for Threads and Args
        pthread_t th[threads];
        size_t num[threads];

        // Iteratively Create Threads with Number
        for (size_t i = 0; i < threads; i++) {
            num[i] = i;
            errno = pthread_create(th + i, NULL, &threadOp,
                    (void *) (num + i));
            if (errno) {
                perror("Thread Creation");
                return 1;
            }
        }

        // Iteratively Join Threads
        for (size_t i = 0; i < threads; i++) {
            errno = pthread_join(th[i], NULL);
            if (errno) {
                perror("Thread Joining");
                return 1;
            }
        }
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

    // Get Thread Number
    size_t mod = *(size_t *) arg;

    // For every unmarked set, run exhaustive test
    ssize_t res = sr_query_parallel(rec, NULLIF, 0,
            threads, mod, &testElim);
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

// Exit with a Fail Code
_Noreturn void tryExitFail(void)
{
    // exit() depends on global var(s), not thread-safe
    static pthread_mutex_t exitLock = PTHREAD_MUTEX_INITIALIZER;

    pthread_mutex_lock(&exitLock);
    exit(1);
}
