// ============================ GENERATION =============================

// Copyright (c) 2023, Jacob Bates
// SPDX-License-Identifier: BSD-2-Clause

// This program takes in two records, a source and a destination, and it
// performs a generation. Given that all the marked sets on the source
// correspond to nullifiable sets, the program will mark all their
// supersets on the destination, as all supersets of a nullifiable set
// are also nullifiable. Then, it will also expand any of those sets
// that don't have the second bit set by introducing mutations to the
// values. Each of these expansion phases are optional.

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

#include "../lib/iface.h"
#include "../lib/setRec.h"
#include "../lib/mutate.h"
#include "../lib/supers.h"

// Toggles for each Expansion Phase
bool expandSupers;
bool expandMutate;

// Add'l Options
bool omitImportDest;
bool verbose;

// Set Records
SR_Base *src = NULL;
SR_Base *dest = NULL;
size_t srcSize;
char *srcFname, *destFname;
size_t srcTotal;

// Overall Maximum Value
unsigned long max;

// Number of Threads
size_t threads = 1;

// Progress
typedef struct Prog Prog;
struct Prog {
    size_t sup;
    size_t mut;
};
volatile Prog *progv = NULL;
char *progFname = NULL;
sigset_t progmask;

// Usage Format String
const char *usage =
        "Usage: %s [-smcv] srcSize src.dat dest.dat "
                "[threads [prog.out]]\n"
        "   -s      Expansion Phase Toggle -- Supersets\n"
        "   -m      Expansion Phase Toggle -- Mutations\n"
        "   -c      Create/Overwrite Destination (Source M-values)\n"
        "   -v      Verbose: Display Progress Messages\n";

int main(int argc, char **argv)
{
    // ============ Command-Line Arguments

    // Parse arguments, show usage on invalid
    {
        const Param params[6] = {PARAM_SIZE, PARAM_FNAME, PARAM_FNAME,
                PARAM_CT, PARAM_FNAME, PARAM_END};
        int res;

        res = argParse(params, 3, argc, argv,
                &srcSize, &srcFname, &destFname, &threads, &progFname);
        if (res) {
            fprintf(stderr, usage, argv[0]);
            return 1;
        }

        res = optHandle("smcv", true, argc, argv,
                &expandSupers, &expandMutate, &omitImportDest,
                &verbose);
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

    // Validate Thread Count
    if (threads < 1) {
        fprintf(stderr, "Error: Must use at least 1 thread\n");
        return 1;
    }

    // Block Progress Signal
    sigemptyset(&progmask);
    sigaddset(&progmask, SIGUSR1);
    sigprocmask(SIG_BLOCK, &progmask, NULL);

    // ============ Import Records

    // Initialize Records
    src = sr_initialize(srcSize);
    dest = sr_initialize(srcSize + 1);
    CK_PTR(src);
    CK_PTR(dest);

    // Import Source Record from File
    if (openImport(src, srcFname)) {
        fprintf(stderr, "Caused by Source\n");
        return 1;
    }
    srcTotal = sr_getTotal(src);

    // Import Destination Record from File
    if (!omitImportDest) {
        if (openImport(dest, destFname)) {
            fprintf(stderr, "Caused by Destination\n");
            return 1;
        }
    }

    // Or Create it from Scratch
    else {
        int res = sr_alloc(dest, sr_getMinM(src), sr_getMaxM(src));
        CK_RES(res);
    }

    // ============ Perform Expansions in Threads

    // Print Information about Execution
    {
        // Max M-Values
        unsigned long srcMaxM = sr_getMaxM(src);
        unsigned long destMaxM = sr_getMaxM(dest);
        max = srcMaxM > destMaxM ? srcMaxM : destMaxM;

        // Print Infos
        if (verbose)
        {
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
    }

    // Set Up Expansion Programs
    supersInit(max);
    mutateInit(max);

    // Use threads to do all the computing
    {
        void *threadOp(void *);
        void *threadUnblocked(void *);

        // Arrays for Threads and Args
        pthread_t th[threads];
        progv = calloc(threads, sizeof(Prog));
        CK_PTR(progv);

        // Iteratively Create Threads
        for (size_t i = 0; i < threads; i++) {
            errno = pthread_create(th + i, NULL, &threadOp,
                    (void *) (progv + i));
            CK_NO(errno);
        }

        // Create Signal Handler Thread
        pthread_t handler;
        errno = pthread_create(&handler, NULL, &threadUnblocked, NULL);
        CK_NO(errno);

        // Iteratively Join Threads
        for (size_t i = 0; i < threads; i++) {
            errno = pthread_join(th[i], NULL);
            CK_NO(errno);
        }

        // Cancel Handler Thread
        errno = pthread_cancel(handler);
        CK_NO(errno);

        free((void *) progv);
        progv = NULL;
    }

    // Clean Up
    mutateInit(0);

    // ============ Export and Cleanup

    // Export Destination
    if (openExport(dest, destFname)) return 1;

    // Unlink Records
    sr_release(src);
    sr_release(dest);

    return 0;
}

// Thread Function for Performing Expansion
void *threadOp(void *arg)
{
    void expand_sup(const unsigned long *, size_t, char);
    void expand_mut(const unsigned long *, size_t, char);

    ssize_t res = 0;

    // Argument is a Reference for Progress Output
    Prog *prog = (Prog *) arg;

    // Get Thread Number
    size_t mod = prog - progv;

    // For every nullifiable set, expand to supersets
    if (expandSupers) res = sr_query_parallel(src, NULLIF, NULLIF,
            threads, mod, &prog->sup, &expand_sup);
    CK_RES(res);

    // For the new nullifiable sets, introduce mutations
    if (expandMutate) res = sr_query_parallel(src, MARKED, NULLIF,
            threads, mod, &prog->mut, &expand_mut);
    CK_RES(res);

    return NULL;
}

// Thread Function for Unblocking Progress Signal
void *threadUnblocked(void *arg)
{
    void progHandler(int);

    // Set up handler and unblock signal
    struct sigaction act = {0};
    act.sa_handler = &progHandler;
    sigaction(SIGUSR1, &act, NULL);

    pthread_sigmask(SIG_UNBLOCK, &progmask, NULL);

    // Just wait
    while (1) pause();

    return NULL;
}

// Progress Signal Handler
void progHandler(int signo)
{
    if (signo != SIGUSR1) return;
    if (progFname == NULL) return;

    // Open Progress File
    int fd = open(progFname, O_WRONLY | O_TRUNC);
    CK_RES(fd);

    // Sum of Progress
    size_t progSup = 0, progMut = 0;
    for (size_t i = 0; i < threads; i++) {
        progSup += progv[i].sup;
        progMut += progv[i].mut;
    }

    // Raw Number Buffers, Fixed Size
    uint64_t buf[3] = {progSup, progMut, srcTotal};

    // Output Progress
    CK_RES(write(fd, buf, sizeof(buf)));

    // Close Progress File
    CK_RES(close(fd));

    return;
}

// Individual Set Expansion Functions

void expand_sup(const unsigned long *set, size_t setc, char bits)
{
    void elim_onlySup(const unsigned long *, size_t);

    // Expand set to supersets; don't mutate further
    supers(set, setc, &elim_onlySup);

    return;
}

void expand_mut(const unsigned long *set, size_t setc, char bits)
{
    void elim_nul(const unsigned long *, size_t);

    // Introduce mutations; set might need further mutation
    mutate(set, setc, &elim_nul);

    return;
}

// Individual Set Elimination Functions

void elim_onlySup(const unsigned long *set, size_t setc)
{
    // Mark this set as Nullifiable/Superset
    int res = sr_mark(dest, set, setc, NULLIF | ONLY_SUP);
    CK_RES(res);

    return;
}

void elim_nul(const unsigned long *set, size_t setc)
{
    // Mark this set as Nullifiable only
    int res = sr_mark(dest, set, setc, NULLIF);
    CK_RES(res);

    return;
}
