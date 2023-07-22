// ============================= EVALUATE ==============================

// This program takes in a record and displays the value representations
// of all the unmarked sets.

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
char *fname;

// Usage Format String
const char *usage = "Usage: %s recSize rec.dat\n";

int main(int argc, char **argv)
{
    // ============ Command-Line Arguments

    // Parse arguments, show usage on invalid
    {
        int argParse(const Param *, int, int, char **, ...);

        const Param params[3] = {PARAM_SIZE, PARAM_FNAME, PARAM_END};

        int res = argParse(params, 2, argc, argv, &size, &fname);
        if (res) {
            fprintf(stderr, usage, argv[0]);
            return 1;
        }
    }

    // ============ Import Record
    rec = sr_initialize(size);

    {
        int openImport(SR_Base *, char *);

        fprintf(stderr, "Importing Record...");
        if (openImport(rec, fname)) return 1;
    }

    // ============ Query Record to Print Sets
    {
        void printSet(const unsigned long *, size_t);

        printf("\n");

        ssize_t res = sr_query(rec, NULLIF, 0, &printSet);
        assert(res != -2);
        if (res == -1) {
            perror("Error");
            return 1;
        }

        printf("\n");
        printf("%ld Total Unmarked Sets\n", res);
    }

    sr_release(rec);

    return 0;
}

// Common Program Functions
#include "common.c"

// Print a Set to the Standard Output
void printSet(const unsigned long *set, size_t setc)
{
    for (size_t i = 0; i < setc; i++)
        printf("%4lu", set[i]);
    printf("\n");

    return;
}
