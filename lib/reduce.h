// ============================== REDUCE ===============================

// See more info about this program in the source file `reduce.c'.

#ifndef REDUCE_H
#define REDUCE_H

#include <stdbool.h>

int subset(const unsigned long *, size_t,
        unsigned long, unsigned long, bool,
        size_t, int (*)(const unsigned long *, size_t));

int contraction(const unsigned long *, size_t,
        unsigned long, unsigned long, bool,
        size_t,
        int (*)(const unsigned long *, size_t, size_t));

#endif
