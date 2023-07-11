// =========================== SET MUTATION ============================

// See more info about this program in the source file `mutate.c'.

#ifndef MUTATE_H
#define MUTATE_H

#include <stdlib.h>

// Configure Maximum Value
int mutateInit(unsigned long);

// Expand Set by One Element with Mutation
int mutate(const unsigned long *, size_t,
        void (*)(const unsigned long *, size_t));

#endif
