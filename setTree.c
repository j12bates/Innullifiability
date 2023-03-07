// ============ Set Tree

// This part of the program controls a tree that has nodes representing
// all the sets. A certain set's details can be accessed on this tree
// simply by accessing child nodes corresponding to the set's element
// values, in ascending order. In other words, the root node has
// children corresponding to the set's lowest value, and their children
// correspond to the next highest values in sets.

// Here's an example of what a tree like this would look like, where the
// total number of set elements is 3 and the values are 1-6:

//                                       <root>
//             ┌──────────────────────────┬┴┴───────────────┬──────────┐
//             1                          2                 3          4
//    ┌────────┼──────┬────┐         ┌────┴─┬────┐        ┌─┴──┐       │
//    2        3      4    5         3      4    5        4    5       5
// ┌─┬┴┬─┐   ┌─┼─┐   ┌┴┐   │       ┌─┼─┐   ┌┴┐   │       ┌┴┐   │       │
// 3 4 5 6   4 5 6   5 6   6       4 5 6   5 6   6       5 6   6       6

// Since these sets cannot have duplicates, a higher-level node cannot
// have children corresponding to certain higher values, as that would
// exhaust the number of possible values for further elements of a set.
// For example, if the maximum value is 9, the starting element cannot
// be the value 7, or else there would be a repeat, which isn't allowed.

// The Level (L) of a node is 

#include <stdlib.h>
#include <stdbool.h>

// Typedefs
typedef struct Base Base;
typedef struct Node Node;

// Set Tree Information Structure
struct Base {
    Node *root;                 // Root node
    size_t levels;              // Number of levels of nodes
    unsigned long maxSuperc;    // Max number of possible child nodes
};

// Tree Node Structure
struct Node {
    Node **supers;              // Pointers to child nodes
    bool flag;                  // Flag
};

// Function Declarations
Node *treeAlloc(size_t, unsigned long);
void treeFree(Node *, size_t, unsigned long);
void treeMark(Node *, size_t, unsigned long,
        unsigned long, unsigned long *, size_t);

// Construct Tree
Base *treeConstruct(size_t levels, unsigned long value,
        unsigned long max)
{
    // We can't have more elements than possible values
    if (max < levels) return NULL;

    // Allocate Memory for Information Structure
    Base *base = malloc(sizeof(Base));

    // Populate Information Structure
    base->levels = levels;
    base->maxSuperc = max - levels + 1;

    // Allocate Entire Tree
    base->root = treeAlloc(levels, max - levels + 1);

    return base;
}

// Destruct Tree
void treeDestruct(Base *base)
{
    // Deallocate Entire Tree
    treeFree(base->root, base->levels, base->maxSuperc);

    // Deallocate Information Strucure
    free(base);

    return;
}

// Recursively Allocate Tree Nodes
Node *treeAlloc(size_t levels, unsigned long superc)
{
    // Allocate Memory for Node
    Node *node = malloc(sizeof(Node));

    // Initialize Values to Defaults
    node->supers = NULL;
    node->flag = false;

    // Base case: if there are no more levels, nothing to enumerate
    if (levels == 0) return node;

    // Allocate Array of Children
    node->supers = calloc(superc, sizeof(Node *));

    // Recurse to Allocate all Children
    for (int i = 0; i < superc; i++)
        node->supers[i] = treeAlloc(levels - 1, superc - i);

    return node;
}

// Recursively Deallocate Tree Nodes
void treeFree(Node *node, size_t levels, unsigned long superc)
{
    // If there are Children, Deallocate them First
    if (levels != 0)
        for (int i = 0; i < superc; i++)
            treeFree(node->supers[i], levels - 1, superc - i);

    // Deallocate Node
    free(node);

    return;
}

// Query a Certain Set and Supersets (actually just set the flag)
void treeQuery(Base *base, unsigned long *values, size_t valuec)
{
    // First Relative Value (must be 1 or greater)
    if (values[0] < 1) return;
    unsigned long rel = values[0] - 1;

    // Allocate Memory for Relative Values
    unsigned long *rels = calloc(valuec - 1, sizeof(unsigned long));

    // Translate Values into Relative Values
    for (size_t i = 1; i < valuec; i++)
    {
        // Values must be in ascending order
        if (values[i] <= values[i - 1]) return;

        // Difference of Values, Subtract One because No Repetition
        rels[i - 1] = values[i] - values[i - 1] - 1;
    }

    // Mark Nodes
    treeMark(base->root, base->levels, base->maxSuperc,
            rel, rels, valuec - 1);

    // Deallocate Memory
    free(rels);

    return;
}

// Recursively Mark Nodes
void treeMark(Node *node, size_t levels, unsigned long superc,
        unsigned long rel, unsigned long *rels, size_t relc)
{
    // If we got here, there's nothing we can do
    if (levels == 0) return;

    // If this node is already marked, do nothing
    if (node->flag) return;

    // This Node Points to the Value Specified
    if (rel < superc)
    {
        // The Child Node of this Value
        Node *super = node->supers[rel];

        // If there are no further values to catch, we're done here
        if (relc == 0) super->flag = true;

        // Otherwise, Recurse on that Child Node
        else treeMark(super, levels - 1, superc - rel,
                rels[0], rels + 1, relc - 1);
    }

    // We have Spare Levels, so Enumerate Intermediary Values
    if (relc + 1 < levels)
    {
        for (unsigned long i = 0; i < rel && i < superc; i++)
        {
            // New Node, Adjust Current Relative Value
            treeMark(node->supers[i], levels - 1, superc - i,
                    rel - i - 1, rels, relc);
        }
    }

    return;
}
