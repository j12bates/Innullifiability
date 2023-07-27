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

#include "iface.h"

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

// Usage Format String
const char *usage = "Usage: %s [-sm] srcSize src.dat dest.dat "
                    "[threads]\n";

int main(int argc, char **argv)
{
    // ============ Command-Line Arguments

    // Parse arguments, show usage on invalid
    {
        const Param params[5] = {PARAM_SIZE, PARAM_FNAME, PARAM_FNAME,
                PARAM_CT, PARAM_END};
        int res;

        res = argParse(params, 3, argc, argv,
                &srcSize, &srcFname, &destFname, &threads);
        if (res) {
            fprintf(stderr, usage, argv[0]);
            return 1;
        }

        res = optHandle("sm", true, argc, argv,
                &expandSupers, &expandMutate);
        if (res) {
            fprintf(stderr, usage, argv[0]);
            return 1;
        }
    }

    // Default to all expansion phases
    if (!expandSupers && !expandMutate) {
        expandSupers = true;
        expandMutate = true;
    }

    // ============ Import Records

    // Initialize Records
    src = sr_initialize(srcSize);
    dest = sr_initialize(srcSize + 1);

    // Import Records from Files
    fprintf(stderr, "Importing Source Record...");
    if (openImport(src, srcFname)) return 1;

    fprintf(stderr, "Importing Destination Record...");
    if (openImport(dest, destFname)) return 1;

    // ============ Perform Expansions in Threads

    // Print Information about Execution
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
        fprintf(stderr, "Performing Generation with %zu Threads\n",
                threads);
        fprintf(stderr, "Expanding by: %s%s\n",
                expandSupers ? "Supersets " : "",
                expandMutate ? "Mutations " : "");
    }

    // Set Up Expansion Programs
    supersInit(max);
    mutateInit(max);

    // Use threads to do all the computing
    {
        void *threadOp(void *);

        // Arrays for Threads and Args
        pthread_t th[threads];
        size_t num[threads];

        // Iteratively Create Threads with Number
        for (size_t i = 0; i < threads; i++) {
            num[i] = i;
            errno = pthread_create(th + i, NULL, &threadOp,
                    (void *) (num + i));
            if (errno) {
                perror("Thread Creation");
                return 1;
            }
        }

        // Iteratively Join Threads
        for (size_t i = 0; i < threads; i++) {
            errno = pthread_join(th[i], NULL);
            if (errno) {
                perror("Thread Joining");
                return 1;
            }
        }
    }

    // Clean Up
    mutateInit(0);

    fprintf(stderr, "Done\n");

    // ============ Export and Cleanup

    // Export Destination
    fprintf(stderr, "Exporting Destination Record...");
    if (openExport(dest, destFname)) return 1;

    // Unlink Records
    sr_release(src);
    sr_release(dest);

    return 0;
}

// Thread Function for Performing Expansion
void *threadOp(void *arg)
{
    void expand_sup(const unsigned long *, size_t);
    void expand_mut(const unsigned long *, size_t);

    ssize_t res;

    // Mutex Lock in case we call exit()
    static pthread_mutex_t exitLock = PTHREAD_MUTEX_INITIALIZER;

    // Get Thread Number
    size_t mod = *(size_t *) arg;

    // For every nullifiable set, expand to supersets
    if (expandSupers) res = sr_query_parallel(src, NULLIF, NULLIF,
            threads, mod, &expand_sup);
    if (res < 0) goto errCk;

    // For the new nullifiable sets, introduce mutations
    if (expandMutate) res = sr_query_parallel(src, MARKED, NULLIF,
            threads, mod, &expand_mut);

    // Check for Errors
errCk:
    assert(res != -2);
    if (res == -1) {
        perror("Error");
        pthread_mutex_lock(&exitLock);
        exit(1);
    }

    return NULL;
}

// Individual Set Expansion Functions

void expand_sup(const unsigned long *set, size_t setc)
{
    void elim_onlySup(const unsigned long *, size_t);

    // Set must be size of source
    assert(setc == srcSize);

    // Expand set to supersets; don't mutate further
    supers(set, setc, &elim_onlySup);

    return;
}

void expand_mut(const unsigned long *set, size_t setc)
{
    void elim_nul(const unsigned long *, size_t);

    // Set must be size of source
    assert(setc == srcSize);

    // Introduce mutations; set might need further mutation
    mutate(set, setc, &elim_nul);

    return;
}

// Individual Set Elimination Functions

void elim_onlySup(const unsigned long *set, size_t setc)
{
    // Mark this set as Nullifiable/Superset
    int res = sr_mark(dest, set, setc, NULLIF | ONLY_SUP);
    assert(res != -2);
}

void elim_nul(const unsigned long *set, size_t setc)
{
    // Mark this set as Nullifiable only
    int res = sr_mark(dest, set, setc, NULLIF);
    assert(res != -2);
}
