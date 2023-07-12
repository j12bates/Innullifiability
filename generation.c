// ============================ GENERATION =============================

// This program takes in two records, a source and a destination, and it
// performs a generation. Given that all the marked sets on the source
// correspond to nullifiable sets, the program will mark all their
// supersets on the destination, as all supersets of a nullifiable set
// are also nullifiable. Then, it will also expand any of those sets
// that don't have the second bit set by introducing mutations to the
// values. Each of these expansion phases are optional.

#include <stdio.h>
#include <stdlib.h>

#include "setRec/setRec.h"
#include "mutate/mutate.h"
#include "supers/supers.h"

#define NULLIF 1 << 0
#define ONLY_SUP 1 << 1
#define MARKED NULLIF | ONLY_SUP

// Number of Threads
unsigned int threads = 1;

// Set Records
SR_Base *src, *dest;
size_t srcSize;

int main(int argc, char **argv)
{
    return 0;
}
