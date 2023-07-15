// ============================ GENERATION =============================

// This program takes in two records, a source and a destination, and it
// performs a generation. Given that all the marked sets on the source
// correspond to nullifiable sets, the program will mark all their
// supersets on the destination, as all supersets of a nullifiable set
// are also nullifiable. Then, it will also expand any of those sets
// that don't have the second bit set by introducing mutations to the
// values. Each of these expansion phases are optional.

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <assert.h>
#include <errno.h>
#include <pthread.h>

#include "setRec/setRec.h"
#include "mutate/mutate.h"
#include "supers/supers.h"

#define NULLIF 1 << 0
#define ONLY_SUP 1 << 1
#define MARKED NULLIF | ONLY_SUP

// Number of Threads
size_t threads = 1;

// Toggles for each Expansion Phase
bool expandSupers = true;
bool expandMutate = true;

// Set Records
SR_Base *src = NULL;
SR_Base *dest = NULL;
size_t srcSize;
char *srcFname, *destFname;

// Overall Maximum Value
unsigned long max;

int main(int argc, char **argv)
{
    // ============ Command-Line Arguments

    // Usage Check
    if (argc < 4) {
        fprintf(stderr, "Usage: %s %s %s %s %s\n",
                argv[0], "srcSize", "src.dat", "dest.dat", "[threads]");
        return 1;
    }

    // SrcSize is Numeric
    errno = 0;
    srcSize = strtoul(argv[1], NULL, 10);
    if (errno) {
        perror("srcSize argument");
        return 1;
    }

    // Optional Threads Argument
    errno = 0;
    if (argc > 4) {
        threads = strtoul(argv[4], NULL, 10);
        if (errno) {
            perror("threads argument");
            return 1;
        }
    }

    // Record Filenames
    srcFname = argv[2];
    destFname = argv[3];

    // ============ Import Records

    // Initialize Records
    src = sr_initialize(srcSize);
    dest = sr_initialize(srcSize + 1);

    // Import Records from Files
    {
        int openImport(SR_Base *, char *);

        fprintf(stderr, "Importing Source Record...");
        fflush(stderr);
        if (openImport(src, srcFname)) return 1;

        fprintf(stderr, "Importing Destination Record...");
        fflush(stderr);
        if (openImport(dest, destFname)) return 1;
    }

    // Get some Information about the Record
    {
        // Max M-Values
        unsigned long srcMaxM = sr_getMaxM(src);
        unsigned long destMaxM = sr_getMaxM(dest);
        max = srcMaxM > destMaxM ? srcMaxM : destMaxM;

        // Print Infos
        fprintf(stderr, "src  - Size: %2zu; M: %4lu to %4lu\n",
                sr_getSize(src), sr_getMinM(src), srcMaxM);
        fprintf(stderr, "dest - Size: %2zu; M: %4lu to %4lu\n",
                sr_getSize(dest), sr_getMinM(dest), destMaxM);
    }

    // ============ Perform Expansions in Threads

    // Set Up Expansion Programs
    supersInit(max);
    mutateInit(max);

    // Alloc array of threads, instantiate threads with threadOp(), join
    // them, dealloc

    // Clean Up
    mutateInit(0);

    return 0;
}

// Open Record File and Import
int openImport(SR_Base *rec, char *fname)
{
    // Open File
    FILE *f = fopen(fname, "rb");
    if (f == NULL) {
        perror("File Error");
        return 1;
    }

    // Import Record
    int res = sr_import(rec, f);
    if (res) {
        if (res == -1) perror("Import Error");
        else if (res == -2) fprintf(stderr, "Wrong Size\n");
        else if (res == -3) fprintf(stderr, "Invalid Record File\n");
        return 1;
    }

    // Close File
    fclose(f);
    fprintf(stderr, "Success\n");

    return 0;
}

// Thread Function for Performing Expansion
void *threadOp(void *arg)
{
    size_t res;

    // Mutex Lock in case we call exit()
    static pthread_mutex_t exitLock = PTHREAD_MUTEX_INITIALIZER;

    // Get Thread Number
    size_t mod = *(size_t *) arg;

    // For every nullifiable set, expand to supersets
    if (expandSupers) res = sr_query_parallel(src, NULLIF, NULLIF,
            threads, mod, &expand_sup);

    // For the new nullifiable sets, introduce mutations
    if (expandMutate && res >= 0) res = sr_query_parallel(src,
            MARKED, NULLIF, threads, mod, &expand_mut);

    // Check for Errors
    assert(res != -2);
    if (res == -1) {
        perror("Error");
        pthread_mutex_lock(&exitLock);
        exit(1);
    }

    return NULL;
}
