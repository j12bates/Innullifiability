// ============ Set Tree

// This part of the program controls a tree that has nodes representing
// all the sets. A certain set's details can be accessed on this tree
// simply by accessing child nodes corresponding to the set's element
// values, in ascending order. In other words, the root node has
// children corresponding to the set's lowest value, and their children
// correspond to the next highest values in sets.

// Since these sets cannot have duplicates, a higher-level node cannot
// have children corresponding to certain higher values, as that would
// exhaust the number of possible values for further elements of a set.
// For example, if the maximum value is 9, the starting element cannot
// be the value 7, or else there would be a repeat, which isn't allowed.

// The Level (L) of a node is 

#include <stdlib.h>
#include <stdbool.h>

typedef struct Base Base;
typedef struct Node Node;

// Tree Information Structure
struct Base {
    Node *root;             // Root node
    size_t levels;          // Number of levels of nodes
    unsigned long maxNodes; // Max number of nodes a node can point to
};

// Tree Node Structure
struct Node {
    Node **supers;          // Pointers to child nodes
    unsigned long superc;   // Number of child nodes
    bool flag;              // Flag
};

// Function Declarations
Node *treeAlloc(size_t, unsigned long);
void treeFree(Node *, size_t, unsigned long);
void treeMark(Node *, size_t, unsigned long, unsigned long *, size_t);

// Construct Tree
Base *treeConstruct(size_t levels, unsigned long value, unsigned long max)
{
    // We can't have more elements than possible values
    if (max < levels) return NULL;

    // Allocate Memory for Information Structure
    Base *base = malloc(sizeof(Base));

    // Populate Information Structure
    base->levels = levels;
    base->maxNodes = max - levels + 1;

    // Allocate Entire Tree
    base->root = treeAlloc(levels, maxNodes);

    return base;
}

// Destruct Tree
void treeDestruct(Base *base)
{
    // Deallocate Entire Tree
    treeFree(base->root, base->levels, base->maxNodes);

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
    node->superc = 0;
    node->flag = false;

    // Base case: if there are no more levels, nothing to enumerate
    if (levels == 0) return node;

    // Allocate Array of Children
    node->superc = superc;
    node->supers = calloc(superc, sizeof(Node *));

    // Recurse to Allocate all Children
    for (int i = 0; i < supers; i++)
        node->super[i] = treeAlloc(levels - 1, supers - i);

    return node;
}

// Recursively Deallocate Tree Nodes
void treeFree(Node *node, size_t levels, unsigned long supers)
{
    // If there are Children, Deallocate them First
    if (levels != 0)
        for (int i = 0; i < supers; i++)
            treeFree(node->super[i], levels - 1, supers - i);

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
    treeMark(base->root, base->levels, rel, rels, valuec - 1);

    // Deallocate Memory
    free(rels);

    return;
}

// Recursively Mark Nodes
void treeMark(Node *node, size_t levels,
        unsigned long rel, unsigned long *rels, size_t relc)
{
    // If we got here, there's nothing we can do
    if (levels == 0) return;

    // This Node Points to the Value Specified
    if (rel < node->superc)
    {
        // The Child Node of this Value
        Node super = node->supers[rel];

        // If there are no further values to catch, we're done here
        if (relc == 0) super->flag = true;

        // Otherwise, Recurse on that Child Node
        else treeMark(super, levels - 1, rels[0], rels + 1, relc - 1);
    }

    // We have Spare Levels, so Enumerate Intermediary Values
    if (relc + 1 < levels)
    {
        for (unsigned long i = 0; i < rel; i++)
        {
            // New Node, Adjust Current Relative Value
            treeMark(node->supers[i], levels - 1,
                    rel - i - 1, rels, relc);
        }
    }

    return;
}
