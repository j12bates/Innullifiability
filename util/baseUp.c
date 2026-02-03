// ======================== BASE-UP PROCESSING =========================

// Copyright (c) 2023-25, Jacob Bates
// SPDX-License-Identifier: BSD-2-Clause

// This program does expansive, base-up work in searching for
// innullifiable sets. It takes in two records, a source and
// destination, and scans across the source, expanding nullifiable sets
// to mark off many nullifiable sets in the destination.

// In our ideal expansion strategy, we will be taking precarious sets of
// all sizes and marking off their supersets. If we're able to do this
// with all precarious sets up to the destination M-range, then we will
// be left with only sets who are themselves either precarious or
// innullifiable. For this reason, this program implements superset
// expansion from any smaller set size, not just N - 1.

// To finish off the search, we test sets by their bisectability. Any
// remaining bisectable sets are precarious, and by definition any
// innullifiable sets aren't. But any precarious sets are guaranteed to
// have a precarious contraction, so we can similarly expand out any
// precarious sets of length N - 1 by mutations, marking off any sets
// that have them as contractions.

// We can't practically gather together all the sets that could possibly
// be the guaranteed contraction of a set in our target space, so
// additional testing must be done. However, if our mutative expansion
// was thorough, we know that contraction can't be in the source
// M-range, so we can have our test ignore contractions in that space.

// In some cases, it may be infeasible to even gather all precarious
// sets up to our target M-range. That process might require excessive
// testing. So in the cases where our ideal strategy makes no
// guarantees, we ought to still be able to mark out as many sets as
// possible. So the program can be configured to generally expand
// bisectable sets rather than just precarious ones, or even all
// nullifiable sets in the source space.

// This and the Top-Down program have the ability to have their progress
// tracked. When sent SIGUSR1, the programs will output a small progress
// update to whatever filename is passed in, ideally a named pipe. Here
// progress is measured by how far through the record we've scanned
// through: in Base-Up, this is the source record of sets we're
// expanding; in Top-Down, this is the destination record we're testing.
// In Base-Up it can optionally also provide the number of sets still
// unmarked in the destination, in case at some point it would be more
// efficient to start the testing stage.

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

#include "../lib/expand.h"
#include "../lib/iface.h"
#include "../lib/setRec.h"

// Set Records
SR_Base *src = NULL;
SR_Base *dest = NULL;
size_t srcSize, destSize;
char *srcFname, *destFname;
size_t srcTotal;

// What Source Sets
bool sup = true, mut = true;
char supMode, mutMode;
char supMask = 0, supBits = BISECT;
char mutMask = 0, mutBits = BISECT;

// Destination Range
unsigned long minM, maxM;
size_t fixedc;
unsigned long *fixedv;

// Number of Threads
size_t threads = 1;

// Progress
volatile size_t *progv = NULL;
char *progFname = NULL;
sigset_t progmask;

// Progress Options
bool progExport;
bool progUnmarked;
bool intProg;

// Usage Format String
const char *usage =
        "Usage: %s [-xui] srcSize src.dat destSize dest.dat "
                "supMode mutMode [threads [prog.out]]\n"
        "supMode and mutMode:\n"
        "   'n'     Expand All Nullifible Sets\n"
        "   'b'       '     '  Bisectable Sets\n"
        "   'p'       '     '  Precarious Sets\n"
        "   '-'     Do not expand by this method\n"
        "Progress Updates:\n"
        "   -x      Export Current Output Record\n"
        "   -u      Include Count of Remaining Unmarked Sets\n"
        "   -i      Generate Progress Update on Interrupt\n";

int main(int argc, char **argv)
{
    // ============ Command Line Arguments

    // Parse Arguments, Show Usage on Invalid
    {
        const Param params[9] = {PARAM_SIZE, PARAM_FNAME,
                PARAM_SIZE, PARAM_FNAME, PARAM_CHAR, PARAM_CHAR,
                PARAM_CT, PARAM_FNAME, PARAM_END};

        CK_IFACE_FN(argParse(params, 6, usage, argc, argv,
                &srcSize, &srcFname, &destSize, &destFname,
                &supMode, &mutMode, &threads, &progFname));

        CK_IFACE_FN(optHandle("xui", true, usage, argc, argv,
                &progExport, &progUnmarked, &intProg));
    }

    // Interpret Mode Characters
    if (supMode == 'n') supBits |= SUPER;
    else if (supMode == 'b') supMask = BISECT;
    else if (supMode == 'p') supMask = BISECT | SUPER;
    else sup = false;

    if (srcSize + 1 != destSize) mut = false;
    else if (mutMode == 'n') mutBits |= SUPER;
    else if (mutMode == 'b') mutMask = BISECT;
    else if (mutMode == 'p') mutMask = BISECT | SUPER;
    else mut = false;

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
    dest = sr_initialize(destSize);
    CK_PTR(src);
    CK_PTR(dest);

    // Import Records from Files
    CK_IFACE_FN(openImport(src, srcFname));
    CK_IFACE_FN(openImport(dest, destFname));
    srcTotal = sr_getTotal(src);

    // Get All Range Information
    minM = sr_getMinM(dest);
    maxM = sr_getMaxM(dest);
    fixedc = sr_getFixedSize(dest);
    fixedv = calloc(fixedc, sizeof(unsigned long));
    CK_PTR(fixedv);
    for (size_t i = 0; i < fixedc; i++)
        fixedv[i] = sr_getFixedValue(dest, i);

    // ============ Perform Expansions in Threads

    // Use threads to do all the computing
    {
        void *threadOp(void *);
        void *threadHandler(void *);

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
        errno = pthread_create(&handler, NULL, &threadHandler, NULL);
        CK_NO(errno);

        // Iteratively Join Threads
        for (size_t i = 0; i < threads; i++) {
            errno = pthread_join(th[i], NULL);
            CK_NO(errno);
        }

        // Cancel Handler Thread
        errno = pthread_cancel(handler);
        CK_NO(errno);

        free((void *) progv); // no, I don't know why cast to void ptr
        progv = NULL;
    }

    // ============ Export and Cleanup

    // Export Destination
    CK_IFACE_FN(openExport(dest, destFname));

    // Unlink Records
    sr_release(src);
    sr_release(dest);

    free(fixedv);

    return 0;
}

// Thread Function for Performing Expansion
void *threadOp(void *arg)
{
    void handleSup(const unsigned long *, size_t, char);
    void handleMut(const unsigned long *, size_t, char);
    ssize_t res;

    // Argument is a Reference for Progress Output
    size_t *prog = (size_t *) arg;

    // Get Thread Number
    size_t mod = prog - progv;

    // TODO: rework everything so that this can be done in one query, so
    // there aren't two different cycles of progress tracking.

    // Query the Record to Perform Superset Expansion
    if (sup) {
        res = sr_query_parallel(src, supMask, supBits,
                threads, mod, prog, &handleSup);
        CK_RES(res);
    }

    // Query the Record to Perform Mutation Expansion
    if (mut) {
        res = sr_query_parallel(src, mutMask, mutBits,
                threads, mod, prog, &handleMut);
        CK_RES(res);
    }

    return NULL;
}

void elimSup(const unsigned long *, size_t);
void elimBisect(const unsigned long *, size_t);
void elimSupBisect(const unsigned long *, size_t);
void (*elim[3])(const unsigned long *, size_t)
    = {&elimSup, &elimBisect, &elimSupBisect};

// Individual Source Set Expansion Functions

void handleMut(const unsigned long *set, size_t size, char bits)
{
    // This library requires absolute set maximum
    unsigned long noFixedSeg_minM = minM;
    unsigned long noFixedSeg_maxM = maxM;
    if (fixedc) noFixedSeg_minM = noFixedSeg_maxM = fixedv[fixedc - 1];

    // Mutation preserves bisectability and the property of being a
    // superset, so we will mark appropriately
    int setMarkIdx = !!(bits & SUPER) + 2 * !!(bits & BISECT) - 1;
    mutate(set, size, noFixedSeg_minM, noFixedSeg_maxM, true, true,
            elim[setMarkIdx]);

    return;
}

void handleSup(const unsigned long *set, size_t size, char bits)
{
    supers(set, size, minM, maxM, fixedc, fixedv,
            destSize, &elimSup);

    return;
}

// Individual Destination Set Elimination (Marking) Functions

void elimSup(const unsigned long *set, size_t size)
{
    int res = sr_mark(dest, set, size, NULLIF | SUPER);
    CK_RES(res);

    return;
}

void elimBisect(const unsigned long *set, size_t size)
{
    int res = sr_mark(dest, set, size, NULLIF | BISECT | TESTED_BISECT);
    CK_RES(res);

    return;
}

void elimSupBisect(const unsigned long *set, size_t size)
{
    int res = sr_mark(dest, set, size, NULLIF | SUPER
            | BISECT | TESTED_BISECT);
    CK_RES(res);

    return;
}

// ============ PROGRESS/SIGNALS

// Thread Function for Intercepting Signals
void *threadHandler(void *arg)
{
    // Unblock the signal and just wait
    pthread_sigmask(SIG_UNBLOCK, &progmask, NULL);
    while (true) pause();

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
    if (progUnmarked) {
        remainingOutput = sr_query(dest, NULLIF, 0, NULL, NULL);
        CK_RES(remainingOutput);
    }

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
