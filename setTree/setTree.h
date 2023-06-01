// ============================= SET TREE =============================

// See more info about this library in the source file `setTree.c'.

#ifndef SETTREE_H
#define SETTREE_H

#include <stdlib.h>

// Set Tree Information Structure
typedef struct Base TreeBase;

// Tree Allocation Modes
typedef enum TreeAllocMode TreeAllocMode;
enum TreeAllocMode {
    ALLOC_STATIC,
    ALLOC_DYNAMIC
};

// Tree Query Modes
typedef enum TreeQueryMode TreeQueryMode;
enum TreeQueryMode {
    QUERY_SETS_UNMARKED,
    QUERY_SETS_MARKED,
    QUERY_SETS_ALL
};

// Initialize a Tree
TreeBase *treeInitialize(size_t, unsigned long, TreeAllocMode);

// Release a Tree (Deallocate/Free)
void treeRelease(TreeBase *);

// Mark a Certain Set and Supersets
int treeMark(const TreeBase *, const unsigned long *, size_t);

// Print (Un)Marked Sets
long long treeQuery(const TreeBase *, TreeQueryMode,
        void (*)(const unsigned long *, size_t));

#endif
