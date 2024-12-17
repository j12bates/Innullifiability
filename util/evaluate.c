// ============================= EVALUATE ==============================

// Copyright (c) 2023, Jacob Bates
// SPDX-License-Identifier: BSD-2-Clause

// This program takes in a record and displays the value representations
// of all the unmarked sets.

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>

#include "../lib/iface.h"
#include "../lib/setRec.h"

// Set Record
SR_Base *rec;
size_t size;
char *fname;

// Whether to List Out Sets
bool disp;

// Number of Threads
size_t threads = 1;

// Set Counts (also for thread IDs)
volatile size_t *countv = NULL;

// Mutex for Printing
pthread_mutex_t printLock = PTHREAD_MUTEX_INITIALIZER;

// Usage Format String
const char *usage = "Usage: %s [-s] recSize rec.dat [threads]\n";

int main(int argc, char **argv)
{
    // ============ Command-Line Arguments

    // Parse arguments, show usage on invalid
    {
        const Param params[4] = {PARAM_SIZE, PARAM_FNAME, PARAM_CT,
                PARAM_END};

        CK_IFACE_FN(argParse(params, 2, usage, argc, argv,
               &size, &fname, &threads));

        CK_IFACE_FN(optHandle("s", false, usage, argc, argv, &disp));
    }

    // Validate Thread Count
    if (threads < 1) {
        fprintf(stderr, "Error: Must use at least 1 thread\n");
        return 1;
    }

    // ============ Import Record
    rec = sr_initialize(size);
    CK_PTR(rec);

    CK_IFACE_FN(openImport(rec, fname));

    // Display Infos
    fprintf(stderr, "rec  - Size: %2zu; M: %4lu to %4lu\n",
            size, sr_getMinM(rec), sr_getMaxM(rec));

    // ============ Query Record to Print Sets

    if (disp) printf("\n");

    // Launch Threads to do the Computing
    {
        void *threadOp(void *);

        // Arrays for Threads and Counts
        pthread_t th[threads];
        countv = calloc(threads, sizeof(size_t));
        CK_PTR(countv);

        // Iteratively Create Threads
        for (size_t i = 0; i < threads; i++) {
            errno = pthread_create(th + i, NULL, &threadOp,
                    (void *) (countv + i));
            CK_NO(errno);
        }

        // Iteratively Join Threads
        for (size_t i = 0; i < threads; i++) {
            errno = pthread_join(th[i], NULL);
            CK_NO(errno);
        }
    }

    size_t count = 0;
    for (size_t i = 0; i < threads; i++) {
        count += countv[i];
    }
    free((void *) countv);

    if (disp) printf("\n");
    printf("%ld Total Unmarked Sets\n", count);

    sr_release(rec);

    return 0;
}

// Thread Function for Scanning the Record & Outputting
void *threadOp(void *arg)
{
    void printSet(const unsigned long *, size_t, char);

    // Argument is a Reference for Count Output
    size_t *count = (size_t *) arg;

    // Get Thread Number
    size_t mod = count - countv;

    // For every unmarked set, count it and print
    ssize_t res = sr_query_parallel(rec, NULLIF, 0,
            threads, mod, NULL, disp ? &printSet : NULL);
    CK_RES(res);

    // Store Set Count
    *count = res;

    return NULL;
}

// Print a Set to the Standard Output
void printSet(const unsigned long *set, size_t size, char bits)
{
    pthread_mutex_lock(&printLock);
    for (size_t i = 0; i < size; i++)
        printf("%4lu", set[i]);
    printf("\n");
    pthread_mutex_unlock(&printLock);

    return;
}
