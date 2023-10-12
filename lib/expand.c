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

// Helper Function Declarations
int supers(const unsigned long *, size_t, unsigned long,
        void (*)(const unsigned long *, size_t));
int mutateAdd(const unsigned long *, size_t, unsigned long,
        void (*)(const unsigned long *, size_t));
void mutateOpSum(unsigned long *, size_t, size_t, unsigned long,
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
        if (pos < size) if (super[pos + 1] == i) {
            pos++;
            continue;
        }

        // Otherwise, we have a superset to output
        if (out != NULL) out(super, size + 1);
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

        // do something similar for difference
    }

    return 0;
}

// This takes in a set with an unused spot at the beginning, as well as
// an index in that set whose value to mutate, and it inserts all the
// equivalent pairs it can using addition, outputting all of these
// expanded sets.

// This works kinda like the superset expansion function, except it has
// two insertions, a main and a follower. The main value is always
// ahead, starting out at the value we're actually mutating, and the
// main plus the follower is always that value. And similarly the
// indices of insertion will shift whenever the values collide.
void mutateOpSum(unsigned long *eSet, size_t eSize,
        size_t mutPt, unsigned long max,
        void (*out)(const unsigned long *, size_t))
{
    // Value we're mutating
    unsigned long mutVal = eSet[mutPt];

    // Iterate through all the main values we could insert, keeping
    // track of the indices we're inserting at
    size_t insPtMain = mutPt, insPtFollower = 0;
    for (unsigned long insValMain = mutVal - 1;
            insValMain > mutVal / 2; insValMain--)
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
        if (eSet[insPtMain - 1] == insValMain)
            insPtMain--, skip = true;
        if (eSet[insPtFollower + 1] == insValFollower)
            insPtFollower++, skip = true;

        // Otherwise, we have an expanded set to output
        if (out != NULL && !skip) out(eSet, eSize);
    }

    return;
}
