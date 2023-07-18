// =============================== WEED ================================

// This program takes in a record, and 'weeds out' all the remaining
// unmarked nullifiable sets. It will iteratively apply the exhaustive
// test and mark any sets that fail.

#include <stdio.h>
#include <stdlib.h>

#include <assert.h>
#include <errno.h>
#include <pthread.h>

#include "setRec/setRec.h"
#include "nulTest/nulTest.h"

#include "common.c"

#define NULLIF 1 << 0

// Number of Threads
size_t threads = 1;

// Set Record
SR_Base *rec = NULL;
size_t size;
char *fname;

int main(int argc, char **argv)
{
    // ============ Command-Line Arguments

    // Usage Check
    if (argc < 3) {
        fprintf(stderr, "Usage: %s %s %s %s\n",
                argv[0], "recSize", "rec.dat", "[threads]");
        return 1;
    }

    // RecSize Argument
    errno = 0;
    size = strtoul(argv[1], NULL, 10);
    if (errno) {
        perror("recSize Argument");
        return 1;
    }

    // Optional Threads Argument
    errno = 0;
    if (argc > 4) {
        threads = strtoul(argv[3], NULL, 10);
        if (errno) {
            perror("threads argument");
            return 1;
        }
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

    // ============ Export and Cleanup
    {
        int openExport(SR_Base *, char *);

        fprintf(stderr, "Exporting Record...");
        if (openExport(rec, fname)) return 1;
    }

    sr_release(rec);

    return 0;
}
