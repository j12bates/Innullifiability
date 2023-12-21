// ============================ CREATE NEW =============================

// Copyright (c) 2023, Jacob Bates
// SPDX-License-Identifier: BSD-2-Clause

// This program creates a blank record file with a specified Variable
// Segment Size, M-range, and Fixed Segment, for use with other
// programs.

#include <stdio.h>
#include <stdlib.h>


#include "../lib/iface.h"
#include "../lib/setRec.h"

// Set Record
SR_Base *rec;
size_t varSize;
unsigned long minm, maxm;
size_t fixedSize;
char *fixedStr;
unsigned long *fixed;
char *fname;

// Usage Format String
const char *usage =
        "Usage: %s size minm maxm fixedSize \"fixedVals\" rec.dat\n";

int main(int argc, char **argv)
{
    // ============ Command-Line Arguments

    // Parse arguments, show usage on invalid
    {
        const Param params[7] = {PARAM_SIZE, PARAM_VAL, PARAM_VAL,
                PARAM_SIZE, PARAM_STR, PARAM_FNAME, PARAM_END};

        CK_IFACE_FN(argParse(params, 6, usage, argc, argv,
                &varSize, &minm, &maxm, &fixedSize, &fixedStr, &fname));
    }

    // Validate Input
    if (varSize < 1) {
        fprintf(stderr, "Size Must be Positive\n");
        return 1;
    }

    if (minm > maxm) {
        fprintf(stderr, "Min M-value Cannot be Greater than Max\n");
        return 1;
    }

    if (fixedSize > 4) {
        fprintf(stderr, "No more than 4 Fixed Values\n");
        return 1;
    }

    // Interpret Fixed Values
    fixed = calloc(fixedSize, sizeof(unsigned long));
    for (size_t i = 0; i < fixedSize; i++) {
        char *next;
        fixed[i] = strtoul(fixedStr, &next, 0);
        fixedStr = next;

        if (errno) {
            perror("Reading Fixed Values");
            return 1;
        }
        if (*next != '\0' && *next != ' ') {
            fprintf(stderr, "Could not Read Fixed Values\n");
            return 1;
        }
    }

    // Validate Fixed Values
    if (fixedSize > 0) {
        if (fixed[0] <= maxm) {
            fprintf(stderr, "Fixed Values must be Above Max M-value\n");
            return 1;
        }
        for (size_t i = 1; i < fixedSize; i++)
            if (fixed[i] <= fixed[i - 1]) {
                fprintf(stderr, "Fixed Values must be Ascending\n");
                return 1;
            }
    }

    // ============ Create Record and Export
    fprintf(stderr, "Creating... Size: %2zu; M: %4lu to %4lu\n",
            varSize, minm, maxm);

    rec = sr_initialize(varSize + fixedSize);
    CK_PTR(rec);

    {
        int res = sr_alloc(rec, varSize, minm, maxm, fixedSize, fixed);
        CK_RES(res);
    }

    CK_IFACE_FN(openExport(rec, fname));

    sr_release(rec);
    free(fixed);

    return 0;
}
