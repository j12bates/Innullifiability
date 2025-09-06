// ============================= EVALUATE ==============================

// Copyright (c) 2023-25, Jacob Bates
// SPDX-License-Identifier: BSD-2-Clause

// This program takes in a record and displays the value representations
// of all the unmarked sets.

// Additionally, this program has a feature to count sets according to
// a set of high-value filters. These can be specified in a space-
// separated list of nonzero whole numbers, and the total counts will be
// output in a space-separated list. The filters are on the M-value, or
// a_{n-k} with k fixed values, also set through a space-separated list.

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>

#include <pthread.h>

#include "../lib/iface.h"
#include "../lib/setRec.h"

// Set Record
SR_Base *rec;
size_t size;
char *fname;

// Whether to List Out Sets
bool disp;

// Set Selection
char mode;
char mask = 0, bits = BISECT;

// Number of Threads
size_t threads = 1;

// Set Counts (also for thread IDs)
volatile size_t *countv = NULL;

// Filtering: arrays for fixed values and high-value filters, and a
// thread-safe array for counting matches
volatile _Atomic size_t filterMatch[1024] = {0};

size_t filterCt = 0;
unsigned long filterValue[1024] = {0};

size_t filterFixedCt = 0;
unsigned long filterFixed[4] = {0};

// Usage Format String
const char *usage =
        "Usage: %s [-s] recSize rec.dat mode "
                "[threads [filters [fixed]]]\n"
        "   -s      Short: No Printing Sets\n";
        // TODO: lexicographic indices as an option

int main(int argc, char **argv)
{
    // ============ Command-Line Arguments

    // Parse arguments, show usage on invalid
    {
        const Param params[7] = {PARAM_SIZE, PARAM_FNAME, PARAM_CHAR,
                PARAM_CT, PARAM_VAL_LIST, PARAM_VAL_LIST, PARAM_END};

        CK_IFACE_FN(argParse(params, 3, usage, argc, argv,
               &size, &fname, &mode,
               &threads, &filterValue, &filterFixed));

        CK_IFACE_FN(optHandle("s", false, usage, argc, argv, &disp));
    }

    // Validate Thread Count
    if (threads < 1) {
        fprintf(stderr, "Error: Must use at least 1 thread\n");
        return 1;
    } else if (threads > 1 && disp) {
        fprintf(stderr, "Sets not printed under multithreading\n");
        disp = false;
    }

    // Interpret Mode Character
    if (mode == 'n') bits |= ONLY_SUP;
    else if (mode == 'b') mask = BISECT;
    else if (mode == 'p') mask = BISECT | ONLY_SUP;
    else if (mode == 'i') {
        mask = ONLY_SUP | BISECT;
        bits = 0;
    }
    else return 0;

    // Count Fixed Values
    unsigned long pFixed = 0;
    for (size_t i = 0; i < 4; i++) {
        if (pFixed < filterFixed[i])
            filterFixedCt = i + 1;
        else if (filterFixed[i] == 0) break;
        else {
            fprintf(stderr, "Error: Invalid fixed-value list\n");
            return 1;
        }
        pFixed = filterFixed[i];
    }

    // Count to Last Valid Filter
    for (size_t i = 0; i < 1024; i++)
        if (filterValue[i]) filterCt = i + 1;

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

    // Total Up all the Sets
    size_t count = 0;
    for (size_t i = 0; i < threads; i++) count += countv[i];
    free((void *) countv);

    // Print the Counts: Each Filter (if any), and Total
    if (disp) printf("\n");
    if (filterCt) printf("filters -- ");
    for (size_t i = 0; i < filterCt; i++)
        printf("%zu ", filterMatch[i]);
    if (filterCt) printf("\n");
    printf("total -- %ld\n", count);

    // Unlink Record
    sr_release(rec);

    return 0;
}

// Thread Function for Scanning the Record & Outputting
void *threadOp(void *arg)
{
    void countSet(const unsigned long *, size_t, char);

    // Argument is a Reference for Count Output
    size_t *count = (size_t *) arg;

    // Get Thread Number
    size_t mod = count - countv;

    // For every unmarked set, count it and print
    ssize_t res = sr_query_parallel(rec, mask, bits,
            threads, mod, NULL, &countSet);
    CK_RES(res);

    // Store Set Count
    *count = res;

    return NULL;
}

// Take a set and do all the counting we need, print if necessary
void countSet(const unsigned long *set, size_t size, char bits)
{
    // Check if fixed values match
    for (size_t i = 0; i < filterFixedCt; i++) {
        unsigned long fixed = filterFixed[filterFixedCt - i - 1];
        if (!fixed) break;
        if (set[size - i - 1] != fixed) goto print;
    }

    // Match against the M-value filters
    for (size_t i = 0; i < filterCt; i++)
        if (set[size - filterFixedCt - 1] == filterValue[i])
            atomic_fetch_add(filterMatch + i, 1);

    // Print set to standard output if required
print:
    if (disp) {
        for (size_t i = 0; i < size; i++)
            printf("%4lu", set[i]);
        printf("\n");
    }

    return;
}
