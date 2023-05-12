// ============================ SET RECORDS ============================

// See more info about this library in the source file `setRec.c'.

#ifndef SETREC_H
#define SETREC_H

#include <stdlib.h>

// Set Record Information Structure
typedef struct Base SR_Base;

// Initialize a Set Record
SR_Base *sr_initialize(size_t, unsigned long);

// Release a Set Record
void sr_release(SR_Base *);

// Mark a Certain Set and Supersets
int sr_mark(const SR_Base *, const unsigned long *, size_t,
        char);

// Output Sets with Particular Mark Status
long long sr_query(const SR_Base *, char, char,
        void (*)(const unsigned long *, size_t));

// Output Sets with Particular Mark Status, for Parallelism
long long sr_query_parallel(const SR_Base *, char, char,
        size_t, size_t, void (*)(const unsigned long *, size_t));


#endif
