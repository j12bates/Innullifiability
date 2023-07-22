// ============================ CREATE NEW =============================

// This program creates a blank record file with a specified Size and
// M-Value Range, for use with the other programs.

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <assert.h>
#include <errno.h>

#include "setRec/setRec.h"

#include "common.h"

// Set Record
SR_Base *rec;
size_t size;
unsigned long minm, maxm;
char *fname;

// Usage Format String
const char *usage = "Usage: %s size minm maxm rec.dat\n";

int main(int argc, char **argv)
{
    // ============ Command-Line Arguments

    // Parse arguments, show usage on invalid
    {
        int argParse(const Param *, int, int, char **, ...);

        const Param params[5] = {PARAM_SIZE, PARAM_VAL, PARAM_VAL,
                PARAM_FNAME, PARAM_END};

        int res = argParse(params, 4, argc, argv,
                &size, &minm, &maxm, &fname);
        if (res) {
            fprintf(stderr, usage, argv[0]);
            return 1;
        }
    }

    // Validate Input
    if (size < 1) {
        fprintf(stderr, "Size Must be Positive");
        return 1;
    }

    if (minm > maxm) {
        fprintf(stderr, "Minimum M-value Must be Less than Maximum\n");
        return 1;
    }

    // ============ Create Record and Export
    fprintf(stderr, "Creating... Size: %2zu; M: %4lu to %4lu\n",
            size, minm, maxm);

    rec = sr_initialize(size);

    {
        int res = sr_alloc(rec, minm, maxm);
        assert(res != -2);
        if (res == -1) {
            perror("Error");
            return 1;
        }
    }

    {
        int openExport(SR_Base *, char *);

        fprintf(stderr, "Exporting Record...");
        if (openExport(rec, fname)) return 1;
    }

    sr_release(rec);

    return 0;
}

// Common Program Function
#include "common.c"
