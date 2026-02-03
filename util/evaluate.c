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

// Furthermore, this program can deal in set lexicographic indices. With
// one command-line option enabled, it'll print indices alongside the
// set representations, and the M-value filter list becomes a list of
// cutoff values for index buckets, into which sets are counted. Bucket
// cutoffs are non-inclusive. The fixed value list is meaningless here.

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

// Whether to Display Sets
bool disp;

// Whether to Filter by Lexicographic Indices (and print if applicable)
bool lexicog;

// Set Selection
char mode;
char mask = 0, bits = BISECT;

// Number of Threads
size_t threads = 1;

// Filtering: arrays for fixed values and high-value filters, and an
// array for counting matches. Alternatively this could be index bucket
// markers.
#define FILTER_CT_MAX 1024
size_t filterCt = 0;
size_t filter[FILTER_CT_MAX] = {0};
size_t filterMatch[FILTER_CT_MAX] = {0};

size_t filterFixedCt = 0;
unsigned long filterFixed[4] = {0};

// Thread Set Counting Vectors
size_t *countv = NULL;
size_t **filterMatchv = NULL;
_Thread_local size_t threadno;

// Usage Format String
const char *usage =
        "Usage: %s [-sl] recSize rec.dat mode "
                "[threads [filters [fixed]]]\n"
        "   -s      Short: No Printing Sets\n"
        "By default, filters can be a space-separated list of M-values"
        " in ascending order, with up to four fixed values above"
        " them.\n"
        "   -l      Lexicographic Indices: Filtering and Display\n"
        "With this option, lexicographic indices are now displayed"
        " alongside printed sets, and filters can be a space-separated"
        " list of index cutoff values in ascending order.\n";

int main(int argc, char **argv)
{
    // ============ Command-Line Arguments

    // Parse arguments, show usage on invalid
    {
        const Param params[7] = {PARAM_SIZE, PARAM_FNAME, PARAM_CHAR,
                PARAM_CT, PARAM_VAL_LIST, PARAM_VAL_LIST, PARAM_END};

        CK_IFACE_FN(argParse(params, 3, usage, argc, argv,
               &size, &fname, &mode, &threads, &filter, &filterFixed));

        CK_IFACE_FN(optHandle("sl", false, usage, argc, argv,
                    &disp, &lexicog));
        lexicog = !lexicog;
    }

    // Interpret Mode Character
    if (mode == 'n') bits |= SUPER;
    else if (mode == 'b') mask = BISECT;
    else if (mode == 'p') mask = BISECT | SUPER;
    else if (mode == 'i') {
        mask = SUPER | BISECT;
        bits = 0;
    }
    else return 0;

    // Validate Thread Count
    if (threads < 1) {
        fprintf(stderr, "Error: Must use at least 1 thread\n");
        return 1;
    } else if (threads > 1 && disp) {
        disp = false;
        fprintf(stderr, "Sets not printed under multithreading\n");
    }

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

    // Count filters until we reach a zero (end of the list)
    for (size_t i = 0; i < FILTER_CT_MAX; i++) {
        if (filter[i]) filterCt = i + 1;
        else break;
        if (i && filter[i - 1] >= filter[i]) {
            fprintf(stderr, "Error: Invalid filter list\n");
            return 1;
        }
    }

    // ============ Import Record
    rec = sr_initialize(size);
    CK_PTR(rec);

    CK_IFACE_FN(openImport(rec, fname));

    // ============ Query Record to Print Sets

    if (disp) printf("\n");

    // Launch Threads to do the Computing
    {
        void *threadOp(void *);

        // Arrays for threads and the count vectors
        pthread_t th[threads];
        countv = calloc(threads, sizeof(size_t));
        CK_PTR(countv);
        filterMatchv = calloc(threads, sizeof(size_t *));
        CK_PTR(countv);
        for (size_t i = 0; i < threads; i++) {
            filterMatchv[i] = calloc(filterCt, sizeof(size_t));
            CK_PTR(filterMatchv[i]);
        }

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
    free(countv);

    // Total Up all the Filter Matches
    for (size_t i = 0; i < threads; i++) {
        for (size_t slot = 0; slot < filterCt; slot++)
            filterMatch[slot] += filterMatchv[i][slot];
        free(filterMatchv[i]);
    }
    free(filterMatchv);

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

// Thread Function for Scanning and Outputting
void *threadOp(void *arg)
{
    void countSet(const unsigned long *, size_t, char);

    // Argument is a Reference for Count Output
    size_t *count = (size_t *) arg;

    // Get Thread Number
    threadno = count - countv;

    // For every set marked the specified way, count it and print as
    // appropriate
    ssize_t res = sr_query_parallel(rec, mask, bits,
            threads, threadno, NULL, &countSet);
    CK_RES(res);

    // Store set count
    *count = res;

    return NULL;
}

// Take a set and do all the counting we need, print if necessary
void countSet(const unsigned long *set, size_t size, char bits)
{
    size_t setToIdx(const unsigned long *, size_t);

    // Both metrics we filter by, index and M-value, increase as we
    // progress down the record, and the Query function works in order.
    // Our filters are in ascending order, so we can keep a shortcut to
    // the one we're "currently on." Then once we've gone through them
    // all, skip all the filtering logic.
    static _Thread_local size_t slot = 0;
    if (slot == filterCt) goto print;

    // Filter by Values
    if (!lexicog)
    {
        // Check if fixed values match
        for (size_t i = 0; i < filterFixedCt; i++) {
            unsigned long fixed = filterFixed[filterFixedCt - i - 1];
            if (!fixed) break;
            if (set[size - i - 1] != fixed) goto print;
        }

        // Retrieve M-value
        unsigned long mValue = set[size - filterFixedCt - 1];

        // Match against M-value filters
        while (mValue > (unsigned long) filter[slot])
            if (++slot == filterCt) goto print;
        if (mValue == filter[slot]) filterMatchv[threadno][slot]++;
    }

    // Filter by Index
    else
    {
        // Compute the lexicographic index
        size_t lexicogIdx = setToIdx(set, size);

        // Match against index bucket cutoffs
        while (lexicogIdx >= filter[slot])
            if (++slot == filterCt) goto print;
        if (lexicogIdx < filter[slot]) filterMatchv[threadno][slot]++;
    }

    // Print to standard output if required
print:
    if (!disp) return;

    for (size_t i = 0; i < size; i++) printf("%4lu", set[i]);
    if (lexicog) printf("%24lu", setToIdx(set, size));
    printf("\n");

    return;
}

// Compute Index from Set
// Returns the index, no error checking
size_t setToIdx(const unsigned long *set, size_t size)
{
    unsigned long long binom(size_t, size_t);

    size_t idx = 0;

    // Go from most significant (highest) to least
    for (size_t vals = size; vals > 0; vals--)
    {
        // Get set value, decrement since we're not using zero, add
        // combinations to index
        size_t m = set[vals - 1] - 1;
        idx += binom(m, vals);
    }

    return idx;
}

// Binomial Coefficient
unsigned long long binom(size_t m, size_t n)
{
    // Zero Case
    if (m < n) return 0;

    // Total Ordered Combinations, Permutations
    unsigned long long total = 1, perms = 1;
    for (size_t i = 0; i < n; i++)
    {
        total *= m - i;
        perms *= i + 1;
    }

    return total / perms;
}
