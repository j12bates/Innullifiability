// =============================== WEED ================================

// This program takes in a record, and 'weeds out' all the remaining
// unmarked nullifiable sets. It will iteratively apply the exhaustive
// test and mark any sets that fail.

#include <stdio.h>
#include <stdlib.h>

#include <assert.h>
#include <errno.h>
#include <pthread.h>

#include "setRec/setRec.h"
#include "nulTest/nulTest.h"

#define NULLIF 1 << 0

// Number of Threads
size_t threads = 1;

// Set Record
SR_Base *rec = NULL;
size_t size;
char *fname;

int main(int argc, char **argv)
{
    // ============ Command-Line Arguments

    // Usage Check
    if (argc < 3) {
        fprintf(stderr, "Usage: %s %s %s %s\n",
                argv[0], "recSize", "rec.dat", "[threads]");
        return 1;
    }

    // RecSize Argument
    errno = 0;
    size = strtoul(argv[1], NULL, 10);
    if (errno) {
        perror("recSize argument");
        return 1;
    }

    // Optional Threads Argument
    errno = 0;
    if (argc > 3) {
        threads = strtoul(argv[3], NULL, 10);
        if (errno) {
            perror("threads argument");
            return 1;
        }
    }

    // Record Filename
    fname = argv[2];

    // ============ Import Record
    rec = sr_initialize(size);

    {
        int openImport(SR_Base *, char *);

        fprintf(stderr, "Importing Record...");
        if (openImport(rec, fname)) return 1;
    }

    // ============ Iteratively Perform Test

    // Print Information about Execution
    fprintf(stderr, "rec  - Size: %2zu; M: %4lu to %4lu\n",
            size, sr_getMinM(rec), sr_getMaxM(rec));
    fprintf(stderr, "Testing Unmarked Sets with %zu Threads\n",
            threads);

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

    fprintf(stderr, "Done\n");

    // ============ Export and Cleanup
    {
        int openExport(SR_Base *, char *);

        fprintf(stderr, "Exporting Record...");
        if (openExport(rec, fname)) return 1;
    }

    sr_release(rec);

    return 0;
}

// Common Program Functions
#include "common.c"

// Thread Function for Testing Sets
void *threadOp(void *arg)
{
    void testElim(const unsigned long *, size_t);
    _Noreturn void tryExitFail(void);

    // Get Thread Number
    size_t mod = *(size_t *) arg;

    // For every unmarked set, run exhaustive test
    size_t res = sr_query_parallel(rec, NULLIF, 0,
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
