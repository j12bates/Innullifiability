// ============================= EVALUATE ==============================

// This program takes in a record and displays the value representations
// of all the unmarked sets.

#include <stdio.h>
#include <stdlib.h>

#include <assert.h>
#include <errno.h>

#include "setRec/setRec.h"

#define NULLIF 1 << 0

// Set Record
SR_Base *rec;
size_t size;
char *fname;

int main(int argc, char **argv)
{
    // ============ Command-Line Arguments

    // Usage Check
    if (argc < 3) {
        fprintf(stderr, "Usage: %s recSize rec.dat\n", argv[0]);
        return 1;
    }

    // RecSize Argument
    errno = 0;
    size = strtoul(argv[1], NULL, 10);
    if (errno) {
        perror("recSize argument");
        return 1;
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

    // ============ Export Record
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
