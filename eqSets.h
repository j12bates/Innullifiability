// ========================== EQUIVALENT SETS ==========================

// See more info about this program in the source file `eqSets.c'.

#ifndef EQSETS_H
#define EQSETS_H

// Reconfigure Equivalent Set Generation
int eqSetsInit(unsigned long, size_t);

// Enumerate Equivalent Sets
int eqSets(const unsigned long *, size_t,
        void (*)(const unsigned long *, size_t));

#endif
