// ============================ SET RECORDS ============================

// See more info about this library in the source file `setRec.c'.

#ifndef SETREC_H
#define SETREC_H

#include <stdio.h>
#include <stdlib.h>

// Set Record Information Structure
typedef struct Base SR_Base;

// Initialize a Set Record
SR_Base *sr_initialize(size_t);

// Allocate a Set Record
int sr_alloc(SR_Base *, unsigned long, unsigned long);

// Release a Set Record
void sr_release(SR_Base *);

// Get Record Properties
size_t sr_getSize(const SR_Base *);
unsigned long sr_getMinM(const SR_Base *);
unsigned long sr_getMaxM(const SR_Base *);

// Mark a Certain Set and Supersets
int sr_mark(const SR_Base *, const unsigned long *, size_t,
        char);

// Output Sets with Particular Mark Status
ssize_t sr_query(const SR_Base *, char, char,
        void (*)(const unsigned long *, size_t));

// Output Sets with Particular Mark Status, for Parallelism
ssize_t sr_query_parallel(const SR_Base *, char, char,
        size_t, size_t, void (*)(const unsigned long *, size_t));

// Import Record from Binary FIle
int sr_import(SR_Base *, FILE *restrict);

// Export Record to Binary File
int sr_export(const SR_Base *, FILE *restrict);

#endif
