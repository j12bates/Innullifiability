// ============================== EXPAND ===============================

// Copyright (c) 2023, Jacob Bates
// SPDX-License-Identifier: BSD-2-Clause

// This program is for expanding sets, the inverse operation to 'merging
// and reducing,' in order to find all the sets that could reduce
// immediately to whatever set is input, based on the given rules. A set
// is input, as well as some configuration, and the expansions are
// output through a function pointer.

#include <stdlib.h>
#include <stdbool.h>

#include <errno.h>

#include "expand.h"

// Helper Function Declarations
static int supers(const unsigned long *, size_t,
        unsigned long, unsigned long,
        void (*)(const unsigned long *, size_t));
static int mutate(const unsigned long *, size_t,
        unsigned long, unsigned long, bool, bool,
        void (*)(const unsigned long *, size_t));

static void insertEqPair(unsigned long *, size_t, size_t,
        const unsigned long *, unsigned long, unsigned long,
        void (*)(const unsigned long *, size_t));

// Produce All Set Expansions
// Returns 0 on success, -1 on error (check errno)
int expand(const unsigned long *set, size_t size,
        unsigned long max, int mode,
        void (*out)(const unsigned long *, size_t))
{
#ifndef NO_VALIDATE
    // Validate Input Set: values are ascending and in-range
    errno = EINVAL;
    if (set[0] < 1) return -1;
    for (size_t i = 1; i < size; i++)
        if (set[i] <= set[i - 1]) return -1;
    if (set[size - 1] > max) return -1;
    errno = 0;
#endif

    // If no output, skip all this work
    if (out == NULL) return 0;

    // We can't remove values, so if there are two values specifically
    // above the M-range, mutation won't work
    if (size >= 2) if (set[size - 2] > max) return 0;

    // Mutations
    mutate(set, size, 1, max,
            mode & EXPAND_MUT_ADD, mode & EXPAND_MUT_MUL, out);

    // And if there's even one such value, supersets won't work
    if (size >= 1) if (set[size - 1] > max) return 0;

    // Supersets
    if (mode & EXPAND_SUPERS) supers(set, size, 1, max, out);

    return 0;
}

// ============ Helper Functions

// Enumerate Supersets
// Returns 0 on success, -1 on error (check errno)

// Accepts a set that's not above the M-range, and outputs all supersets
// within the M-range.
int supers(const unsigned long *set, size_t size,
        unsigned long minM, unsigned long maxM,
        void (*out)(const unsigned long *, size_t))
{
    // Set Representation
    unsigned long *super = calloc(size + 1, sizeof(unsigned long));
    if (super == NULL) return -1;

    // Check relation to M-range
    bool belowMRange = set[size - 1] < minM;

    // Initialize with the input, leaving a spot for insertion: front if
    // we're inserting everything, back if we're just inserting in-range
    // M-values
    for (size_t i = 0; i < size; i++)
        if (!belowMRange) super[i + 1] = set[i];
        else super[i] = set[i];

    // Iterate over values to insert, keeping track of index
    size_t pos = !belowMRange ? 0 : size;
    for (unsigned long i = !belowMRange ? 1 : minM; i <= maxM; i++)
    {
        // Insert Value
        super[pos] = i;

        // If we've reached the next value, skip and advance
        bool skip = false;
        if (pos < size) if (super[pos + 1] == i) pos++, skip = true;

        // Otherwise, we have a superset to output
        if (!skip) out(super, size + 1);
    }

    free(super);

    return 0;
}

// Enumerate Set Mutations
// Returns 0 on success, -1 on error (check errno)

// Accepts a set that's not above the M-range, or which has only one
// value 'poking out', and outputs all mutations within the M-range. It
// can be specified which mutation modes to use (additive,
// multiplicative).
int mutate(const unsigned long *set, size_t size,
        unsigned long minM, unsigned long maxM, bool add, bool mul,
        void (*out)(const unsigned long *, size_t))
{
    // Set Representation for Expanded Set
    unsigned long *eSet = calloc(size + 1, sizeof(unsigned long));
    if (eSet == NULL) return -1;

    // Check relation to M-range
    unsigned long mval = set[size - 1];
    bool belowMRange = mval < minM;
    bool aboveMRange = mval > maxM;
    bool inMRange = !belowMRange && !aboveMRange;

    // We can insert any value, unless we have to get the set back into
    // the M-range, in which case we must insert something in that range
    unsigned long minMajor = 1;
    if (!inMRange) minMajor = minM;

    // Iterate through all the different elements we could mutate
    for (size_t mutPt = 0; mutPt < size; mutPt++)
    {
        // Value we're Mutating
        unsigned long mutVal = set[mutPt];

        // At the M-value, we have to make sure to not erase it without
        // the new M-value being in-range, unless a previous value can
        // be the new M-value
        if (mutPt == size - 1) {
            minMajor = minM;
            if (size >= 2) if (set[size - 2] >= minM) minMajor = 1;
        }

        // Sum and product pairs will break a value up into two smaller
        // ones, so we can use them when the set will remain in range,
        // but also to break up the M-value if it's above the range
        if (inMRange || (aboveMRange && mutPt == size - 1))
        {
            // Sum Equivalent Pairs: iterate over larger addends
            if (add) for (unsigned long major = mutVal - 1;
                    major > mutVal / 2; major--)
            {
                if (major < minMajor) continue;
                unsigned long minor = mutVal - major;
                insertEqPair(eSet, size + 1, mutPt, set,
                        minor, major, out);
            }

            // Product Equivalent Pairs: iterate over smaller factors
            if (mul) for (unsigned long minor = 1;
                    minor < mutVal / minor; minor++)
            {
                if (mutVal % minor != 0) continue;
                unsigned long major = mutVal / minor;
                if (major < minMajor) continue;
                insertEqPair(eSet, size + 1, mutPt, set,
                        minor, major, out);
            }
        }

        // Difference and quotient pairs will always result in an
        // increase in value, so we can insert them so long as there's
        // nothing poking out above the range
        if (inMRange || belowMRange)
        {
            // Difference Equivalent Pairs: iterate over minuends
            if (add) for (unsigned long minuend = mutVal + 1;
                    minuend <= maxM; minuend++)
            {
                if (minuend < minMajor) continue;
                unsigned long subtrahend = minuend - mutVal;
                insertEqPair(eSet, size + 1, mutPt, set,
                        subtrahend, minuend, out);
            }

            // Quotient Equivalent Pairs: iterate over divisors
            if (mul) for (unsigned long divisor = 1;
                    divisor <= maxM / mutVal; divisor++)
            {
                unsigned long dividend = mutVal * divisor;
                if (dividend < minMajor) continue;
                insertEqPair(eSet, size + 1, mutPt, set,
                        divisor, dividend, out);
            }
        }
    }

    free(eSet);

    return 0;
}

// Insert Equivalent Pair into Set, Output
void insertEqPair(unsigned long *eSet, size_t eSize, size_t mutPt,
        const unsigned long *set, unsigned long minor,
        unsigned long major, void (*out)(const unsigned long *, size_t))
{
    // Can't create a double value
    if (minor == major) return;

    // Iterate through output set indices, keeping track of source set
    // index
    size_t eIndex = 0;
    for (size_t index = 0; index < eSize - 1; index++)
    {
        // Insert new value(s) if in order, value of zero means nothing
        // to insert
        while (minor < set[index] && minor != 0) {
            eSet[eIndex++] = minor;
            minor = major;
            major = 0;
        }

        // Copy the next set value unless it's being replaced; exit if
        // that'll create a double value
        if (index != mutPt) {
            if (minor == set[index]) return;
            eSet[eIndex++] = set[index];
        }
    }

    // Insert the new value(s) if we haven't already
    if (minor != 0) eSet[eIndex++] = minor;
    if (major != 0) eSet[eIndex++] = major;

    // Output Set
    out(eSet, eSize);

    return;
}
