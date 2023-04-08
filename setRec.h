// ============================ SET RECORDS ============================

// See more info about this library in the source file `setRec.c'.

#ifndef SETREC_H
#define SETREC_H

#include <stdlib.h>

// Set Record Information Structure
typedef struct Base SR_Base;

// Set Record Query Modes
typedef enum SR_QueryMode SR_QueryMode;
enum SR_QueryMode {
    SR_QUERY_SETS_UNMARKED,
    SR_QUERY_SETS_MARKED,
    SR_QUERY_SETS_ALL
};

// Initialize a Set Record
SR_Base *sr_initialize(size_t, unsigned long);

// Release a Set Record
void sr_release(SR_Base *);

// Mark a Certain Set and Supersets
int sr_mark(const SR_Base *, const unsigned long *, size_t);

// Output Sets with Particular Mark Status
long long sr_query(const SR_Base *, SR_QueryMode,
        void (*)(const unsigned long *, size_t));

#endif
