// ============================= SET TREE =============================

// See more info about this library in the source file `setTree.c'.

#ifndef SETTREE_H
#define SETTREE_H

#include <stdlib.h>

// Set Tree Information Structure
typedef struct Base Base;

// Set Print Modes
typedef enum PrintMode PrintMode;
enum PrintMode {
    PRINT_SETS_UNMARKED,
    PRINT_SETS_MARKED,
    PRINT_SETS_ALL
};

// Construct a Tree
Base *treeConstruct(size_t, unsigned long);

// Destruct a Tree (Deallocate/Free)
void treeDestruct(Base *);

// Mark a Certain Set and Supersets
int treeMark(const Base *, const unsigned long *, size_t);

// Print (Un)Marked Sets
long long treeQuery(const Base *, PrintMode, FILE *);

#endif
