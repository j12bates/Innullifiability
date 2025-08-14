// =============================== TESTS ===============================

// See more info about this function in the source file `tests.c'.

#ifndef TESTS_H
#define TESTS_H

#include <stdlib.h>

int bisectSubs(const unsigned long *, size_t, size_t, size_t);

int bisect(unsigned long, unsigned long,
        unsigned long, unsigned long, size_t);

int success(unsigned long, unsigned long, unsigned long, size_t);

#endif
