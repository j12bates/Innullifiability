// =============================== WEED ================================

// This program takes in a record, and 'weeds out' all the remaining
// unmarked nullifiable sets. It will iteratively apply the exhaustive
// test and mark any sets that fail.

#include <stdio.h>
#include <stdlib.h>

#include <assert.h>
#include <errno.h>
#include <pthread.h>

#include "nulTest/nulTest.h"

#include "common.c"

#define NULLIF 1 << 0

// Number of Threads
size_t threads = 1;

// Set Record
SR_Base *rec = NULL;
size_t size;
char *fname;

int main(int argc, char **argv)
{
    return 0;
}
