// ============================== EXPAND ===============================

// See more info about this program in the source file `expand.c'.

#ifndef EXPAND_H
#define EXPAND_H

// Produce All Set Expansions
int expand(const unsigned long *, size_t,
        unsigned long, int,
        void (*)(const unsigned long *, size_t));

#endif
