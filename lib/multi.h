// ========================== MULTIPLE EXPAND ==========================

// See more info about this program in the source file `multi.c'.

#ifndef MULTI_H
#define MULTI_H

// Produce Expansions of a Set to a Specific Size
int multiExpand(const unsigned long *, size_t,
        unsigned long, unsigned long,
        size_t, const unsigned long *,
        size_t, void (*)(const unsigned long *, size_t));

#endif
