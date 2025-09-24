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
// cutoff values for index buckets, into which sets are counted. Fixed
// values ignored.

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "../lib/iface.h"
#include "../lib/setRec.h"

// Set Record
SR_Base *rec;
size_t size;
char *fname;

// Whether to List Out Sets
bool disp;

// Whether to Deal in Lexicographic Indices
bool lexicog;

// Set Selection
char mode;
char mask = 0, bits = BISECT;

// Set Counts (also for thread IDs)
size_t *countv = NULL;

// Filtering: arrays for fixed values and high-value filters, and an
// array for counting matches. Alternatively this could be index bucket
// markers.
size_t filterMatch[1024] = {0};

size_t filterCt = 0;
unsigned long filterValue[1024] = {0};

size_t filterFixedCt = 0;
unsigned long filterFixed[4] = {0};

// Usage Format String
const char *usage =
        "Usage: %s [-sl] recSize rec.dat mode [filters [fixed]]\n"
        "   -s      Short: No Printing Sets\n"
        "By default, filters can be a space-separated list of M-values,"
        " with up to four fixed values above them.\n"
        "   -l      Lexicographic Indices: Filtering and Display\n"
        "With this option, lexicographic indices are now displayed"
        " alongside printed sets, and filters can be a space-separated"
        " list of index cutoff values in ascending order.\n";

int main(int argc, char **argv)
{
    // ============ Command-Line Arguments

    // Parse arguments, show usage on invalid
    {
        const Param params[6] = {PARAM_SIZE, PARAM_FNAME, PARAM_CHAR,
                PARAM_VAL_LIST, PARAM_VAL_LIST, PARAM_END};

        CK_IFACE_FN(argParse(params, 3, usage, argc, argv,
               &size, &fname, &mode, &filterValue, &filterFixed));

        CK_IFACE_FN(optHandle("sl", false, usage, argc, argv,
                    &disp, &lexicog));
        lexicog = !lexicog;
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
    for (size_t i = 0; i < 1024; i++) {
        if (filterValue[i]) filterCt = i + 1;
        else continue;
        if (lexicog && i && filterValue[i - 1] >= filterValue[i]) {
            fprintf(stderr, "Error: Invalid index bucket list\n");
            return 1;
        }
    }

    // ============ Import Record
    rec = sr_initialize(size);
    CK_PTR(rec);

    CK_IFACE_FN(openImport(rec, fname));

    // ============ Query Record to Print Sets

    if (disp) printf("\n");

    // For every unmarked set, count it and print
    size_t count = 0;
    {
        void countSet(const unsigned long *, size_t, char);

        ssize_t res = sr_query(rec, mask, bits, NULL, &countSet);
        CK_RES(res);
        count = (size_t) res;
    }

    if (disp) printf("\n");

    // Print the Counts: Each Filter (if any), and Total
    if (filterCt) printf("filters -- ");
    for (size_t i = 0; i < filterCt; i++)
        printf("%zu ", filterMatch[i]);
    if (filterCt) printf("\n");
    printf("total -- %ld\n", count);

    // Unlink Record
    sr_release(rec);

    return 0;
}

// Take a set and do all the counting we need, print if necessary
void countSet(const unsigned long *set, size_t size, char bits)
{
    size_t setToIdx(const unsigned long *, size_t);

    // Filter by Values
    if (!lexicog)
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
                filterMatch[i]++;
    }

    // Filter by Index
    else
    {
        // Compute the lexicographic index
        size_t lexicogIdx = setToIdx(set, size);

        // Match against index bucket cutoffs
        for (size_t i = 0; i < filterCt; i++)
            if (lexicogIdx <= (size_t) filterValue[i]) {
                filterMatch[i]++;
                break;
            }
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
