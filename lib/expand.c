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
int supers(const unsigned long *, size_t, unsigned long,
        void (*)(const unsigned long *, size_t));
int mutateAdd(const unsigned long *, size_t, unsigned long,
        void (*)(const unsigned long *, size_t));
int mutateMul(const unsigned long *, size_t, unsigned long,
        void (*)(const unsigned long *, size_t));

void mutateOpSum(unsigned long *, size_t, size_t, unsigned long,
        void (*)(const unsigned long *, size_t));
void mutateOpDiff(unsigned long *, size_t, size_t, unsigned long,
        void (*)(const unsigned long *, size_t));
void insertEqPair(unsigned long *, size_t, size_t,
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

    // Supersets
    if (mode & EXPAND_SUPERS) supers(set, size, max, out);

    // Additive Mutations
    if (mode & EXPAND_MUT_ADD) mutateAdd(set, size, max, out);

    // Multiplicative Mutations
    if (mode & EXPAND_MUT_MUL) mutateMul(set, size, max, out);

    return 0;
}

// ============ Helper Functions

// Enumerate Supersets
// Returns 0 on success, -1 on error (check errno)
int supers(const unsigned long *set, size_t size, unsigned long max,
        void (*out)(const unsigned long *, size_t))
{
    // Set Representation
    unsigned long *super = calloc(size + 1, sizeof(unsigned long));
    if (super == NULL) return -1;

    // Initialize with the input, leaving a spot for insertion
    for (size_t i = 0; i < size; i++) super[i + 1] = set[i];

    // Iterate over values to insert, keeping track of index
    size_t pos = 0;
    for (unsigned long i = 1; i <= max; i++)
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

// Enumerate Additively Mutated Sets
// Returns 0 on success, -1 on error (check errno)
int mutateAdd(const unsigned long *set, size_t size, unsigned long max,
        void (*out)(const unsigned long *, size_t))
{
    // Set Representation for Expanded Set
    unsigned long *eSet = calloc(size + 1, sizeof(unsigned long));
    if (eSet == NULL) return -1;

    // Iterate through all the different elements we could mutate
    for (size_t mutPt = 0; mutPt < size; mutPt++)
    {
        // Initialize output and call helper function (Sum)
        for (size_t i = 0; i < size; i++) eSet[i + 1] = set[i];
        mutateOpSum(eSet, size + 1, mutPt, max, out);

        // Same (Difference)
        for (size_t i = 0; i < size; i++) eSet[i + 1] = set[i];
        mutateOpDiff(eSet, size + 1, mutPt, max, out);
    }

    free(eSet);

    return 0;
}

// Enumerate Multiplicatively Mutated Sets
// Returns 0 on success, -1 on error (check errno)
int mutateMul(const unsigned long *set, size_t size, unsigned long max,
        void (*out)(const unsigned long *, size_t))
{
    // Set Representation for Expanded Set
    unsigned long *eSet = calloc(size + 1, sizeof(unsigned long));
    if (eSet == NULL) return -1;

    // Iterate through all the different elements we could mutate
    for (size_t mutPt = 0; mutPt < size; mutPt++)
    {
        // Value we're mutating
        unsigned long mutVal = eSet[mutPt];

        // Product Equivalent Pairs: iterate over smaller (minor) factors
        for (unsigned long minor = 2; minor < mutVal / minor; minor++)
        {
            if (mutVal % minor != 0) continue;
            unsigned long major = mutVal / minor;
            insertEqPair(eSet, size + 1, mutPt, set, minor, major, out);
        }

        // Quotient Equivalent Pairs: iterate over divisors
        for (unsigned long divisor = 2; divisor <= max / mutVal;
                divisor++)
        {
            unsigned long dividend = mutVal * divisor;
            insertEqPair(eSet, size + 1, mutPt, set,
                    divisor, dividend, out);
        }
    }

    return 0;
}

// These take in a set with an unused spot at the beginning, as well as
// an index in that set whose value to mutate, and it inserts all the
// equivalent pairs it can using additive operations, outputting all of
// these expanded sets.

// They work kinda like the superset expansion function, except with two
// insertions, a main and a follower, where the operation will produce
// the value being mutated, creating an 'Equivalent Pair.' The main
// value is always smaller, starting out at 1 and incrementing each
// time. The follower value is then the balance, so for addition it'll
// decrement each time until meeting up with the main, and for
// subtraction it'll increment until reaching the max value. And to keep
// the sets in order, the insertion indices will shift whenever values
// collide.

// Addition
void mutateOpSum(unsigned long *eSet, size_t eSize,
        size_t mutPt, unsigned long max,
        void (*out)(const unsigned long *, size_t))
{
    // Value we're mutating
    unsigned long mutVal = eSet[mutPt];

    // Iterate through all the main values we could insert, keeping
    // track of the indices we're inserting at
    size_t insPtMain = 0, insPtFollower = mutPt;
    for (unsigned long insValMain = 1;
            insValMain <= (mutVal - 1) / 2; insValMain++)
    {
        // Follower insertion value
        unsigned long insValFollower = mutVal - insValMain;

        // Insert Values
        eSet[insPtMain] = insValMain;
        eSet[insPtFollower] = insValFollower;

        // If either of these insertions have reached the value ahead of
        // it, advance its index, and skip to the next equivalent pair
        // since this has a double value
        bool skip = false;
        if (eSet[insPtMain + 1] == insValMain)
            insPtMain++, skip = true;
        if (eSet[insPtFollower - 1] == insValFollower)
            insPtFollower--, skip = true;

        // Otherwise, we have an expanded set to output
        if (!skip) out(eSet, eSize);
    }

    return;
}

// Subtraction
void mutateOpDiff(unsigned long *eSet, size_t eSize,
        size_t mutPt, unsigned long max,
        void (*out)(const unsigned long *, size_t))
{
    unsigned long mutVal = eSet[mutPt];

    // Iterate through main insertion values
    size_t insPtMain = 0, insPtFollower = mutPt;
    for (unsigned long insValMain = 1;
            insValMain <= max - mutVal; insValMain++)
    {
        unsigned long insValFollower = mutVal + insValMain;

        // Insert
        eSet[insPtMain] = insValMain;
        eSet[insPtFollower] = insValFollower;

        // Y'know, advance and skip
        bool skip = false;
        if (eSet[insPtMain + 1] == insValMain)
            insPtMain++, skip = true;
        if (eSet[insPtFollower + 1] == insValFollower)
            insPtFollower++, skip = true;

        // Output
        if (!skip) out(eSet, eSize);
    }

    return;
}

// Insert Equivalent Pair into Set, Output
void insertEqPair(unsigned long *eSet, size_t eSize, size_t mutPt,
        const unsigned long *set, unsigned long minor,
        unsigned long major, void (*out)(const unsigned long *, size_t))
{
    // Iterate through output set indices, keeping track of source set
    // index
    size_t index = 0;
    for (size_t eIndex = 0; eIndex < eSize; eIndex++)
    {
        // If the next new value fits here, insert it, and change up the
        // variables for next time
        if (minor < set[index] && minor != 0) {
            eSet[eIndex] = minor;
            minor = major;
            major = 0;
        }

        // Otherwise, copy the next set value over
        else eSet[eIndex] = set[index++];
    }

    // Output Set
    out(eSet, eSize);

    return;
}
