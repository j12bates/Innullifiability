// ===================== GENERAL MULTIPLE SUPERSET =====================

// Copyright (c) 2025, Jacob Bates
// SPDX-License-Identifier: BSD-2-Clause

// This program is another one for conducting an expansion, except it is
// generalized to any source and destination set size. So if we want to
// expand a record of length-6 sets into one of length-8 sets directly,
// without having to deal with a monstrous load of length-7 riffraff, we
// can do it this way. Only supersets are implemented, not multiple-
// mutations.

// This program does not implement any form of progress tracking as it
// isn't necessary for the Database Script.

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <pthread.h>
#include <unistd.h>

#include "../lib/iface.h"
#include "../lib/multi.h"

// Set Records
SR_Base *src = NULL;
SR_Base *dest = NULL;
size_t srcSize, destSize;
char *srcFname, *destFname;

// Destination Range
unsigned long minM, maxM;
size_t fixedc;
unsigned long *fixedv;

// Number of Threads
size_t threads = 1;

// Thread Index Things
size_t *tidxv = NULL;

// Usage Format String
const char *usage =
        "Usage: %s srcSize src.dat destSize dest.dat [threads]\n";

int main(int argc, char **argv)
{
    // ============ Command Line Arguments

    // Parse Arguments, Show Usage on Invalid
    {
        const Param params[6] = {PARAM_SIZE, PARAM_FNAME,
                PARAM_SIZE, PARAM_FNAME, PARAM_CT, PARAM_END};

        CK_IFACE_FN(argParse(params, 4, usage, argc, argv,
                &srcSize, &srcFname, &destSize, &destFname, &threads));
    }

    // Validate Thread Count
    if (threads < 1) {
        fprintf(stderr, "Error: Must use at least 1 thread\n");
        return 1;
    }

    // ============ Import Records

    // Initialize Records
    src = sr_initialize(srcSize);
    dest = sr_initialize(destSize);
    CK_PTR(src);
    CK_PTR(dest);

    // Import Records from Files
    CK_IFACE_FN(openImport(src, srcFname));
    CK_IFACE_FN(openImport(dest, destFname));

    // Get All Range Information
    minM = sr_getMinM(dest);
    maxM = sr_getMaxM(dest);
    fixedc = sr_getFixedSize(dest);
    fixedv = calloc(fixedc, sizeof(unsigned long));
    CK_PTR(fixedv);
    for (size_t i = 0; i < fixedc; i++)
        fixedv[i] = sr_getFixedValue(dest, i);

    // ============ Perform Expansions in Threads

    // Use threads to do all the computing
    {
        void *threadOp(void *);

        // Arrays for Threads and Args
        pthread_t th[threads];
        tidxv = calloc(threads, sizeof(size_t));
        CK_PTR(tidxv);

        // Iteratively Create Threads
        for (size_t i = 0; i < threads; i++) {
            errno = pthread_create(th + i, NULL, &threadOp,
                    (void *) (tidxv + i));
            CK_NO(errno);
        }

        // Iteratively Join Threads
        for (size_t i = 0; i < threads; i++) {
            errno = pthread_join(th[i], NULL);
            CK_NO(errno);
        }

        free((void *) tidxv); // no, I don't know why cast to void ptr
        tidxv = NULL;
    }

    // ============ Export and Cleanup

    // Export Destination
    CK_IFACE_FN(openExport(dest, destFname));

    // Unlink Records
    sr_release(src);
    sr_release(dest);

    free(fixedv);

    return 0;
}

// Thread Function for Performing Expansion
void *threadOp(void *arg)
{
    void handleExpand(const unsigned long *, size_t, char);

    // Get Thread Number
    size_t mod = (size_t *) arg - tidxv;

    // Perform multiple expansion on every nullifiable set
    ssize_t res = sr_query_parallel(src, NULLIF, NULLIF,
            threads, mod, NULL, &handleExpand);
    CK_RES(res);

    return NULL;
}

// Individual Source Set Expansion Function
void handleExpand(const unsigned long *set, size_t size, char bits)
{
    void elim(const unsigned long *, size_t);

    multiExpand(set, size, minM, maxM, fixedc, fixedv, destSize, &elim);

    return;
}

// Individual Destination Set Elimination Function
void elim(const unsigned long *set, size_t size)
{
    // Mark this set as Nullifiable/Superset
    int res = sr_mark(dest, set, size, NULLIF | ONLY_SUP);
    CK_RES(res);

    return;
}
