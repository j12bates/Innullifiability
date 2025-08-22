// ============================== EXPAND ===============================

// See more info about this program in the source file `expand.c'.

#ifndef EXPAND_H
#define EXPAND_H

int supers(const unsigned long *, size_t,
        unsigned long, unsigned long,
        size_t, const unsigned long *,
        size_t, void (*)(const unsigned long *, size_t));
int mutate(const unsigned long *, size_t,
        unsigned long, unsigned long, bool, bool,
        void (*)(const unsigned long *, size_t));

#endif
