// ============================= SUPERSETS =============================

// See more info about this program in the source file `supers.c'.

#ifndef SUPERS_H
#define SUPERS_H

// Reconfigure Superset Generation
int supersInit(unsigned long);

// Enumerate Supersets
int supers(const unsigned long *, size_t,
        void (*)(const unsigned long *, size_t));

#endif
