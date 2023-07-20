// ============================ CREATE NEW =============================

// This program creates a blank record file with a specified Size and
// M-Value Range, for use with the other programs.

#include <stdio.h>
#include <stdlib.h>

#include <assert.h>
#include <errno.h>

#include "setRec/setRec.h"

// Set Record
SR_Base *rec;
size_t size;
unsigned long minm, maxm;
char *fname;

int main(int argc, char **argv)
{
    // ============ Command-Line Arguments

    // Usage Check
    if (argc < 5) {
        fprintf(stderr, "Usage: %s size minm maxm rec.dat\n", argv[0]);
        return 1;
    }

    // Size Argument
    errno = 0;
    size = strtoul(argv[1], NULL, 10);
    if (errno) {
        perror("size argument");
        return 1;
    }

    // MinM Argument
    errno = 0;
    minm = strtoul(argv[2], NULL, 10);
    if (errno) {
        perror("minm argument");
        return 1;
    }

    // MaxM Argument
    errno = 0;
    maxm = strtoul(argv[3], NULL, 10);
    if (errno) {
        perror("maxm argument");
        return 1;
    }

    // Validate Input
    if (minm > maxm) {
        fprintf(stderr, "Minimum M-value Must be Less than Maximum\n");
        return 1;
    }

    // Record Filename
    fname = argv[4];

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
