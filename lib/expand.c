// ============================== EXPAND ===============================

// Copyright (c) 2023-25, Jacob Bates
// SPDX-License-Identifier: BSD-2-Clause

// This library is for expanding sets, the inverse operation to 'merging
// and reducing,' in order to find all the sets that could reduce
// immediately to whatever set is input, based on the given rules. A set
// is input, as well as some configuration, and the expansions are
// output through a function pointer. The input set could be anything,
// of any size or M-value, so long as it's valid and in ascending order,
// and the library guarantees that the output will be of the specified
// size (always +1 for mutations) and in the specified M-range, and that
// the whole of the output will cover every possible such set that could
// be reduced to the input.

// There are two ways a set could be expanded: Supersets, and Mutations.
// Supersets are pretty simple: since adding an arbitrary value to a
// nullifiable set won't change anything (the extra value can simply be
// multiplied away after reaching zero), we can insert whatever
// arbitrary values will get/keep the output set in the M-range.

// Mutations are the more interesting bit. In essence, they try to
// 'un-merge' a value, or replace it with two others that give the
// original back when operated on, keeping the set nullifiable. For
// example, an 'equivalent pair' of 2 is (3, 5), since 5 - 3 = 2, and
// (4, 8), since 8 / 4 = 2. If we had a set like (2, 3, 5), which we
// know is nullifiable, we can substitute the 2 with (4, 8) from before,
// to get the set (3, 4, 5, 8), which we know must also be nullifiable
// since that 4 and 8 can divide to get 2--and the original set--back.

// For nullifiability purposes, we'll be expanding every precarious set
// we know (in and below our target M-range) of every size up to the
// target size. Superset expansion is thus implemented to handle any
// source/destination size. It also allows specification of the fixed
// values, as it is easy to optimize the superset generation that way,
// and we'll be working below the target M-range too. Mutations will
// only be done on precarious sets of one fewer element, and likely
// within the target M-range since the number of expansions decline
// outside. So fixed values aren't as important here, and they're also
// harder to implement, so it's easier to just generate them for nothing
// anyways.

#include <stdlib.h>
#include <stdbool.h>

#include <errno.h>

#include "expand.h"

// Helper Function Declarations
static void insert(unsigned long *, size_t, int,
        size_t, unsigned long,
        void (*)(const unsigned long *, size_t));
static void insertEqPair(unsigned long *, size_t, size_t,
        const unsigned long *, unsigned long, unsigned long,
        void (*)(const unsigned long *, size_t));

// Produce Supersets of a given Larger Size

// Accepts any proper set and outputs any supersets that lie in the
// given M-range with the given fixed values.
int supers(const unsigned long *set, size_t srcSize,
        unsigned long minM, unsigned long maxM,
        size_t fixedc, const unsigned long *fixedv,
        size_t destSize, void (*out)(const unsigned long *, size_t))
{
#ifndef NO_VALIDATE
    // Validate Input Set: values are positive and ascending
    errno = EINVAL;
    if (set[0] < 1) return -1;
    for (size_t i = 1; i < srcSize; i++)
        if (set[i] <= set[i - 1]) return -1;

    // Validate Sizes, Fixed Values
    if (destSize < srcSize) return -1;
    if (fixedc > 0 && fixedv[0] <= minM) return -1;
    for (size_t i = 1; i < fixedc; i++)
        if (fixedv[i] <= fixedv[i - 1]) return -1;
    errno = 0;
#endif

    // If no output, skip all this work
    if (out == NULL) return 0;

    // Total Values to Insert
    int inserts = destSize - srcSize;

    // First count until we reach a value in the Fixed Range
    size_t i = 0;
    for (; i < srcSize; i++) if (set[i] > maxM) break;

    // Countinue counting and verify that those fixed values are allowed
    size_t segIdx = i;
    for (size_t fixedIdx = 0; fixedIdx < fixedc; fixedIdx++) {
        if (i == srcSize) break;
        if (fixedv[fixedIdx] == set[i]) i++;
        else if (set[i] < fixedv[fixedIdx]) return 0;
    }
    if (i != srcSize) return 0;

    // Calculate how many fixed value insertions we're doing
    int insertsFixed = fixedc - (srcSize - segIdx);

    // Create the array for the superset and copy in the values for the
    // variable segment
    unsigned long *super = calloc(destSize, sizeof(unsigned long));
    if (super == NULL) return -1;
    for (size_t i = 0; i < segIdx; i++) super[i] = set[i];

    // If our max value in the variable segment is already in the
    // M-range, we can just do supersets of this alone
    if (segIdx > 0 && set[segIdx - 1] >= minM)
    {
        // Copy the values for the fixed segment
        for (size_t i = 0; i < fixedc; i++)
            super[segIdx + i] = fixedv[i];

        // Perform Insertions
        insert(super, destSize, inserts - insertsFixed, 0, maxM, out);
    }

    // Otherwise, we must manually insert M-values
    else
    {
        // Copy the values for the fixed segment, leaving one spot
        for (size_t i = 0; i < fixedc; i++)
            super[segIdx + i + 1] = fixedv[i];

        // Iterate through the values in the M-range
        for (unsigned long valM = minM; valM <= maxM; valM++)
        {
            // Manually Insert, Perform Insertions
            super[segIdx] = valM;
            insert(super, destSize, inserts - insertsFixed - 1,
                    0, valM - 1, out);
        }
    }

    free(super);
    return 0;
}

// Produce First-Order Mutations
// Returns 0 on success, -1 on error (check errno)

// Accepts any proper set and outputs any mutations of the set in the
// given M-range. Note that this range doesn't distinguish between fixed
// and variable segments---it is simply the largest value allowed in the
// set.
int mutate(const unsigned long *set, size_t size,
        unsigned long minM, unsigned long maxM, bool add, bool mul,
        void (*out)(const unsigned long *, size_t))
{
#ifndef NO_VALIDATE
    // Validate Input Set: values are positive and ascending
    errno = EINVAL;
    if (set[0] < 1) return -1;
    for (size_t i = 1; i < size; i++)
        if (set[i] <= set[i - 1]) return -1;
    errno = 0;
#endif

    // If no output, skip all this work
    if (out == NULL) return 0;

    // No mutations of null set. No mutation can remove two values above
    // the M-range.
    if (size < 1) return 0;
    if (size >= 2) if (set[size - 2] > maxM) return 0;

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

// ============ Helper Functions

// Insert Values into a Set

// This function will insert values into a set to generate a range of
// supersets of a certain order. We start at a given index in the set
// and end on a particular value. So we start by shifting the tail end
// of the set rightwards, so we can insert values. We iterate over all
// the values we could insert, advancing our index on collision. At the
// end we shift any remaining tail back to where it started, so we have
// the original set back. This process is done recursively for higher-
// order supersets.
void insert(unsigned long *super, size_t size, int inserts,
        size_t idxStart, unsigned long valEnd,
        void (*out)(const unsigned long *, size_t))
{
    // If no more insertions, output complete set
    if (inserts == 0) out(super, size);
    if (inserts <= 0) return;

    // Shift our tail to the right so we have a spot to insert
    for (size_t i = size - inserts; i > idxStart; i--)
        super[i] = super[i - 1];

    // Insert values starting from the successor to what's on our left
    size_t idx = idxStart;
    unsigned long val = 1;
    if (idx > 0) val = super[idx - 1] + 1;

    // Try to insert every value up until our range ends
    for (; val <= valEnd - inserts + 1; val++)
    {
        // Insert Value
        super[idx] = val;

        // If we've reached the next index, skip and advance
        if (idx < size - 1 && super[idx + 1] == val) {
            idx++;
            continue;
        }

        // Recurse for another insertion, starting from the next value
        insert(super, size, inserts - 1, idx + 1, valEnd, out);
    }

    // Shift the remaining tail back to get rid of our insertion spot
    for (size_t i = idx; i < size - inserts; i++)
        super[i] = super[i + 1];

    return;
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
