// ============================= EVALUATE ==============================

// Copyright (c) 2023, Jacob Bates
// SPDX-License-Identifier: BSD-2-Clause

// This program takes in a record and displays the value representations
// of all the unmarked sets.

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

// Usage Format String
const char *usage = "Usage: %s [-s] recSize rec.dat\n";

int main(int argc, char **argv)
{
    // ============ Command-Line Arguments

    // Parse arguments, show usage on invalid
    {
        const Param params[3] = {PARAM_SIZE, PARAM_FNAME, PARAM_END};
        int res;

        res = argParse(params, 2, usage, argc, argv, &size, &fname);
        if (res) return 1;

        res = optHandle("s", false, usage, argc, argv, &disp);
        if (res) return 1;
    }

    // ============ Import Record
    rec = sr_initialize(size);
    CK_PTR(rec);

    if (openImport(rec, fname)) return 1;

    // Display Infos
    fprintf(stderr, "rec  - Size: %2zu; M: %4lu to %4lu\n",
            size, sr_getMinM(rec), sr_getMaxM(rec));

    // ============ Query Record to Print Sets
    {
        void printSet(const unsigned long *, size_t, char);

        if (disp) printf("\n");

        ssize_t res = sr_query(rec, NULLIF, 0, NULL,
                disp ? &printSet : NULL);
        CK_RES(res);

        if (disp) printf("\n");
        printf("%ld Total Unmarked Sets\n", res);
    }

    sr_release(rec);

    return 0;
}

// Print a Set to the Standard Output
void printSet(const unsigned long *set, size_t setc, char bits)
{
    for (size_t i = 0; i < setc; i++)
        printf("%4lu", set[i]);
    printf("\n");

    return;
}
