// ============================= SET TREE =============================

// See more info about this library in the source file `setTree.c'.

#ifndef SETTREE_H
#define SETTREE_H

#include <stdlib.h>

// Set Tree Information Structure
typedef struct Base Base;

// Tree Allocation Modes
typedef enum AllocMode AllocMode;
enum AllocMode {
    ALLOC_STATIC,
    ALLOC_DYNAMIC
};

// Tree Query Modes
typedef enum QueryMode QueryMode;
enum QueryMode {
    QUERY_SETS_UNMARKED,
    QUERY_SETS_MARKED,
    QUERY_SETS_ALL
};

// Construct a Tree
Base *treeConstruct(size_t, unsigned long, AllocMode);

// Destruct a Tree (Deallocate/Free)
void treeDestruct(Base *);

// Mark a Certain Set and Supersets
int treeMark(const Base *, const unsigned long *, size_t);

// Print (Un)Marked Sets
long long treeQuery(const Base *, QueryMode,
        void (*)(const unsigned long *, size_t));

#endif
