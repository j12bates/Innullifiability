// ========================== EQUIVALENT SETS ==========================

// See more info about this program in the source file `eqSets.c'.

#ifndef EQSETS_H
#define EQSETS_H

#include <stdlib.h>

// Reconfigure Equivalent Set Generation
int eqSetsInit(size_t, unsigned long);

// Enumerate Equivalent Sets
int eqSets(const unsigned long *, size_t,
        bool (*)(const unsigned long *, size_t));

#endif
