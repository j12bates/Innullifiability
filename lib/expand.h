// ============================== EXPAND ===============================

// See more info about this program in the source file `expand.c'.

#ifndef EXPAND_H
#define EXPAND_H

#define EXPAND_SUPERS 1 << 0
#define EXPAND_MUT_ADD 1 << 1
#define EXPAND_MUT_MUL 1 << 2

// Produce All Set Expansions
int expand(const unsigned long *, size_t,
        unsigned long, int,
        void (*)(const unsigned long *, size_t));

#endif
