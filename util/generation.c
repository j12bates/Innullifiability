// ============================ GENERATION =============================

// Copyright (c) 2023, Jacob Bates
// SPDX-License-Identifier: BSD-2-Clause

// This program takes in two records, a source and a destination, and it
// performs a generation. Given that all the marked sets on the source
// correspond to nullifiable sets, the program will mark all their
// supersets on the destination, as all supersets of a nullifiable set
// are also nullifiable. Then, it will also expand sets by introducing
// mutations to the values, or replacing each individual value with
// every possible pair that could reduce to it. Each of these expansion
// phases are optional, but using both guarantees that any set that can
// reduce to any nullifiable sets in the source range is marked.

// The superset status of a set is recorded in the second bit, and by
// default, mutations on supersets of nullifiable sets are skipped
// because they're covered already through mutations done earlier on the
// parent set. However this is not completely equivalent to a ranged
// weeding, since those 'earlier mutations' could be in a completely
// different range and not dealt with in the one expansion. So there's
// an option to disable that optimization and do a 'thorough expansion'
// instead, which provides an equivalent effect to a ranged weeding. But
// even with the optimization, if we are sure to operate on every
// M-range (effectively to infinity, practically to M * (M - 1)) through
// either these expansions or ranged weeding, this guarantees those
// 'earlier mutations' are dealt with, and thus a fully swept record.

// This and the Weed program are the two programs which do *proper
// work,* and so they have the ability to have their progress tracked.
// When sent SIGUSR1, these will produce a progress update, sending a
// small amount of data to a file provided in the argument list, ideally
// a named pipe. It provides 64-bit binary representations of the number
// of sets currently elapsed in the scan, as well as the total number of
// sets. For Generation, it'll optionally also provide the number of
// sets remaining unmarked in the output, and for Weed, it'll provide
// the number of sets having passed the test so far.

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

#include "../lib/iface.h"
#include "../lib/setRec.h"
#include "../lib/expand.h"

// Toggles for each Expansion Phase
bool expandSupers;
bool expandMutate;
bool expandThorough;

// Add'l Options
bool omitImportDest;
bool verbose;

// Progress Options
bool progExport;
bool progUnmarked;
bool intProg;

// Set Records
SR_Base *src = NULL;
SR_Base *dest = NULL;
size_t srcSize;
char *srcFname, *destFname;
size_t srcTotal;

// M-range
unsigned long minM;
unsigned long maxM;

// Number of Threads
size_t threads = 1;

// Progress
volatile size_t *progv = NULL;
char *progFname = NULL;
sigset_t progmask;

// Usage Format String
const char *usage =
        "Usage: %s [-cvsmtxui] srcSize src.dat dest.dat "
                "[threads [prog.out]]\n"
        "   -c      Create/Overwrite Destination (M-range and Fixed "
                "Values taken from Source)\n"
        "   -v      Verbose: Display Progress Messages\n"
        "Expansion Phases (both enabled by default):\n"
        "   -s      Supersets\n"
        "   -m      Mutations\n"
        "   -t      Thorough\n"
        "Progress Updates:\n"
        "   -x      Export Current Output Record\n"
        "   -u      Include Count of Remaining Unmarked Sets\n"
        "   -i      Generate Progress Update on Interrupt\n";

int main(int argc, char **argv)
{
    // ============ Command-Line Arguments

    // Parse arguments, show usage on invalid
    {
        const Param params[6] = {PARAM_SIZE, PARAM_FNAME, PARAM_FNAME,
                PARAM_CT, PARAM_FNAME, PARAM_END};

        CK_IFACE_FN(argParse(params, 3, usage, argc, argv,
                &srcSize, &srcFname, &destFname, &threads, &progFname));

        CK_IFACE_FN(optHandle("cvsmtxui", true, usage, argc, argv,
                &omitImportDest, &verbose, &expandSupers, &expandMutate,
                &expandThorough, &progExport, &progUnmarked, &intProg));
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

    // Set up Handler for Progress
    {
        void progHandler(int);
        struct sigaction act = {0};

        act.sa_handler = &progHandler;
        sigaction(SIGUSR1, &act, NULL);
    }

    // Set up Handler for Interrupt
    {
        void intHandler(int);
        struct sigaction act = {0};

        act.sa_handler = &intHandler;
        sigaction(SIGINT, &act, NULL);
    }

    // ============ Import Records

    // Initialize Records
    src = sr_initialize(srcSize);
    dest = sr_initialize(srcSize + 1);
    CK_PTR(src);
    CK_PTR(dest);

    // Import Source Record from File
    CK_IFACE_FN(openImport(src, srcFname));
    srcTotal = sr_getTotal(src);

    // Import Destination Record from File
    if (!omitImportDest) CK_IFACE_FN(openImport(dest, destFname));

    // Or Create it from Scratch
    else {
        size_t fixedSize = sr_getFixedSize(src);
        unsigned long *fixed = calloc(fixedSize, sizeof(unsigned long));
        for (size_t i = 0; i < fixedSize; i++)
            fixed[i] = sr_getFixedValue(src, i);

        int res = sr_alloc(dest, srcSize + 1 - fixedSize,
                sr_getMinM(src), sr_getMaxM(src), fixedSize, fixed);
        CK_RES(res);
    }

    // If we have fixed values, the highest one is our M-range
    size_t fixedc = sr_getFixedSize(dest);
    if (fixedc) {
        minM = sr_getFixedValue(dest, fixedc - 1);
        maxM = minM;
    }

    // Otherwise, just the usual M-range
    else {
        minM = sr_getMinM(dest);
        maxM = sr_getMaxM(dest);
    }

    // ============ Perform Expansions in Threads

    // Print Information about Execution
    if (verbose) {
        fprintf(stderr, "src  - Size: %2zu; M: %4lu to %4lu\n",
                sr_getSize(src), sr_getMinM(src), sr_getMaxM(src));
        fprintf(stderr, "dest - Size: %2zu; M: %4lu to %4lu\n",
                sr_getSize(dest), minM, maxM);
        fprintf(stderr, "Performing Generation with %zu Threads\n",
                threads);
        fprintf(stderr, "Expanding by: %s%s\n",
                expandSupers ? "Supersets " : "",
                expandMutate ? "Mutations " : "");
    }

    // Use threads to do all the computing
    {
        void *threadOp(void *);
        void *threadUnblocked(void *);

        // Arrays for Threads and Args
        pthread_t th[threads];
        progv = calloc(threads, sizeof(size_t));
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

    // ============ Export and Cleanup

    // Export Destination
    if (verbose) fprintf(stderr, "Writing Output Record...");
    CK_IFACE_FN(openExport(dest, destFname));
    if (verbose) fprintf(stderr, "Done\n");

    // Unlink Records
    sr_release(src);
    sr_release(dest);

    return 0;
}

// Thread Function for Performing Expansion
void *threadOp(void *arg)
{
    void handleExpand(const unsigned long *, size_t, char);

    // Argument is a Reference for Progress Output
    size_t *prog = (size_t *) arg;

    // Get Thread Number
    size_t mod = prog - progv;

    // Perform expansion phases on every nullifiable set
    ssize_t res = sr_query_parallel(src, NULLIF, NULLIF,
            threads, mod, prog, &handleExpand);
    CK_RES(res);

    return NULL;
}

// Thread Function for Intercepting Signals
void *threadUnblocked(void *arg)
{
    // Unblock the signal and just wait
    pthread_sigmask(SIG_UNBLOCK, &progmask, NULL);
    while (1) pause();

    return NULL;
}

// Progress Signal Handler
void progHandler(int signo)
{
    if (signo != SIGUSR1) return;

    // Sum of Progress
    size_t prog = 0;
    for (size_t i = 0; i < threads; i++) prog += progv[i];

    // Count Unmarked Sets in Output if Specified
    ssize_t remainingOutput = 0;
    if (progUnmarked)
        remainingOutput = sr_query(dest, NULLIF, 0, NULL, NULL);

    // Push Progress Update
    if (progFname != NULL)
        if (pushProg(prog, srcTotal, remainingOutput, progFname))
            FAULT();

    // Export Destination if Specified
    if (progExport) CK_IFACE_FN(openExport(dest, destFname));

    return;
}

// Interrupt Handler
void intHandler(int signo)
{
    if (signo != SIGINT) return;

    // Generate Progress Update if Specified
    if (intProg) progHandler(SIGUSR1);

    // Exit the program
    safeExit();

    return;
}

// Set Expansion Function

void handleExpand(const unsigned long *set, size_t size, char bits)
{
    void elim_onlySup(const unsigned long *, size_t);
    void elim_nul(const unsigned long *, size_t);

    // Either way, a nullifiable set's supersets should be marked;
    // further mutations are accounted for
    if (expandSupers)
        expand(set, size, minM, maxM, EXPAND_SUPERS, &elim_onlySup);

    // Introduce Mutations, but only if not touched by supersets (or if
    // we're doing a thorough expansion); don't rule out further
    // mutations
    if (expandMutate) if (!(bits & ONLY_SUP) || expandThorough)
        expand(set, size, minM, maxM, EXPAND_MUT_ADD | EXPAND_MUT_MUL,
                &elim_nul);

    return;
}

// Individual Set Elimination Functions

void elim_onlySup(const unsigned long *set, size_t size)
{
    // Mark this set as Nullifiable/Superset
    int res = sr_mark(dest, set, size, NULLIF | ONLY_SUP);
    CK_RES(res);

    return;
}

void elim_nul(const unsigned long *set, size_t size)
{
    // Mark this set as Nullifiable only
    int res = sr_mark(dest, set, size, NULLIF);
    CK_RES(res);

    return;
}
