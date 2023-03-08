// ============ Set Tree

// This part of the program controls a tree that has nodes representing
// all the sets. These sets contain values from 1 to a particular
// maximum value (M), and are without repetition. A certain set's
// details can be accessed on this tree simply by accessing child nodes
// corresponding to the set's element values, in ascending order. In
// other words, the root node has children corresponding to the set's
// lowest value, and their children correspond to the next highest
// values in sets.

// Here's an example of what a tree like this would look like, where the
// total number of set elements (N) is 3 and the maximum value (M) is 6:

//                                       <root>
//             ┌──────────────────────────┬┴┴───────────────┬──────────┐
//             1                          2                 3          4
//    ┌────────┼──────┬────┐         ┌────┴─┬────┐        ┌─┴──┐       │
//    2        3      4    5         3      4    5        4    5       5
// ┌─┬┴┬─┐   ┌─┼─┐   ┌┴┐   │       ┌─┼─┐   ┌┴┐   │       ┌┴┐   │       │
// 3 4 5 6   4 5 6   5 6   6       4 5 6   5 6   6       5 6   6       6

// Note that for each level, the range of values represented by child
// nodes shifts upward by one, from 1-4 to 2-5 to 3-6. This is due to
// the nature of non-repetitive sets: successive values must be at least
// one greater (or else it would be a repetition), and as a result they
// cannot be too great such that later elements cannot take on a valid
// value (if the first value is 5, the second must be 6, so what can the
// third be?).

// The number of children a node has is simple to calculate. The root
// has the maximum number of values, which is calculated by the
// following:
//     M - N + 1
// A node has children which correspond to successive values leading to
// the level maximum, which increases by one for every level. Therefore,
// a node has a number of children equal to its total number of larger
// siblings (values which the children represent) plus one (for the
// increase in level. For example, in the tree above, the level-1 node
// representing 1 has 4 children, the node for 2 has 3, that for 3 has
// 2, and the node for the value 4 has only 1 child. This pattern
// continues across all tree levels.

// Knowing this, we will define a standard method for traversing a tree
// like this: a recursive function will take in a node pointer as well
// as two counters, one for the number of remaining levels and one for
// the number of child nodes; after performing some operation, if the
// level counter is not zero (i.e. there are child nodes), it will
// iterate over the node's children, recursing on them with the level
// counter decremented (as a child node is one level down) once and the
// child counter decremented for each iteration (as for each successive
// child there is one fewer value to represent). It is not important for
// a traversal function to know its position within the table.

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

// This is a recursive function for allocating and constructing a tree.
// It uses the standard method for traversal to allocate nodes and
// arrays for children.
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

// This function works in a similar fashion to the allocation function,
// except that it has to free memory AFTER iterating over children
// rather than before. I think it's fairly obvious why this is.
void treeFree(Node *node, size_t levels, unsigned long superc)
{
    // If this node doesn't exist, exit
    if (node == NULL) return;

    // If there are Children, Deallocate them First
    if (levels != 0)
        for (int i = 0; i < superc; i++)
            treeFree(node->supers[i], levels - 1, superc - i);

    // Deallocate Node
    free(node);

    return;
}

// Query a Certain Set and Supersets
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
        // Values must be below the maximum of the lowest level
        if (values[i] >= base->maxSuperc + base->levels) return;

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

// This function is a recursive function for marking nodes which have
// particular ancestors. It uses the standard method for traversal. In
// addition, it takes in a set of constraining 'relative values,' which
// work like child node indices while representing the node's proper
// value, so that the function need not keep track of its current
// position.

// When a node has a child with the next constraining value (i.e. the
// relative value is less than the child counter), the function
// recurses, removing the constraining value, unless there are no more,
// in which case a satisfactory node has been found and is marked.
// Marking a node means that all its descendant nodes are treated as
// though they are marked already.

// The function also needs to account for paths through the tree that
// contain intermediary values. For example, if the constraining values
// are 2 and 5, the path 2 -> 3 -> 5 is satisfactory, even though 5 is
// not immediately following 2. The way this is dealt with using
// relative values is that the next constraining value is decremented
// (as we are passing to a lower level) and the relative value of the
// intermediary is subtracted as well. We can only do this with
// intermediary values lesser than the next constraining value, and it
// is only worth dealing with intermediary values at all if there are
// more levels remaining in the table than constraining values (as
// otherwise we would never reach the final one).
void treeMark(Node *node, size_t levels, unsigned long superc,
        unsigned long rel, unsigned long *rels, size_t relc)
{
    // If this node doesn't exist, exit
    if (node == NULL) return;

    // If we got to the lowest level, there's nothing to do
    if (levels == 0) return;

    // If this node is already marked, no need to go further
    if (node->flag) return;

    // This node has a child that represents the value we want
    if (rel < superc)
    {
        // The Child Node of this Value
        Node *super = node->supers[rel];

        // If there are no further value constraints, this node is
        // satisfactory
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
