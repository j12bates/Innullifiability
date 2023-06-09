// ============================= SET TREE =============================

// This library controls a tree that has nodes representing sets. These
// sets are without repetition (i.e. do not contain the same value
// twice), and contain a certain number of elements (N) ranging in value
// from 1 to a particular maximum value (M).

// The purpose of this library with regards to the main Innullifiability
// program is to keep a record of which sets have been shown to be
// nullifiable. Sets that have been identified as nullifiable can be
// marked on the tree, as well as their supersets, as a superset of a
// nullifiable set is also nullifiable. Once all nullifiable sets have
// been marked, the tree can be traversed to extract innullifiable sets.

// Each node on the tree corresponds to a specific successive value in a
// set, and the lowest-level node represents the complete set. Values
// are in ascending order. In other words, the root node has children
// corresponding to the set's lowest value, and their children
// correspond to the next highest values in sets. Each node has a flag,
// and if the flag is set, it means that any sets that descend from that
// node are considered 'marked'.

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

// The 'relative value' of a node is essentially its index as a child,
// and is computed by decrementing the difference of the node's value
// and its parent's value. Relative values are used for accessing a node
// representing a particular set, as well as keeping track of what
// numbers a node represents when doing a traversal.

// The number of children a node has is simple to calculate. The root
// has the maximum number of values, which is calculated by the
// following:
//     M - N + 1
// A node has children which correspond to successive values leading to
// the level maximum, which increases by one for every level. Therefore,
// a node has a number of children equal to its total number of larger
// siblings (values which the children represent) plus one (for the
// increase in level). For example, in the tree above, the level-1 node
// representing 1 has 4 children, the node for 2 has 3, that for 3 has
// 2, and the node for the value 4 has only 1 child. This pattern
// continues across all tree levels.

// Knowing this, we will define a standard method for traversing a tree
// like this: a recursive function will take in a node pointer as well
// as two counters, one for the number of remaining levels and one for
// the number of child nodes; after performing some operation, if the
// level counter is not zero (i.e. there are child nodes), it will
// iterate over the node's children, recursing on them with the level
// counter decremented once (as a child node is one level down) and the
// child counter decremented for each iteration (as for each successive
// child there is one fewer value to represent). It is not important for
// a traversal function to know its position within the tree.

#include <stdlib.h>
#include <stdbool.h>

#include "setTree.h"

// Tree Node Structure
typedef struct Node Node;
struct Node {
    bool flag;                  // Flag
    Node *supers;               // Pointer to array of child nodes
};

// Set Tree Information Structure
typedef struct Base Base;
struct Base {
    Node *root;                 // Root node
    size_t levels;              // Number of levels of nodes
    unsigned long superc;       // Max number of possible child nodes
    bool dynamic;               // Allocation mode
};

// Other Typedefs
typedef enum TreeAllocMode AllocMode;
typedef enum TreeQueryMode QueryMode;

// Helper Function Declarations
static int nodeFlag(Node *, size_t, unsigned long,
        unsigned long, const unsigned long *, size_t,
        bool);
static long long nodeQuery(const Node *, size_t, unsigned long,
        unsigned long *, size_t, QueryMode,
        void (*)(const unsigned long *, size_t));
static void setPass(const unsigned long *, size_t,
        void (*)(const unsigned long *, size_t));

// Allocation Function Declarations
static int nodeAllocChildren(Node *, size_t, unsigned long);
static int nodeAllocDescs(Node *, size_t, unsigned long);
static void nodeFreeDescs(Node *, size_t, unsigned long);

// ============ User-Level Functions

// These functions are for the main program to interact with, and they
// make reference to proper values and the base information structure,
// abstracting nodes away from the user level. They make use of the
// helper functions, which are defined later on.

// Initialize a Tree
// Returns NULL on input or memory error
Base *treeInitialize(size_t levels, unsigned long max,
        AllocMode allocMode)
{
    // We can't have more elements than possible values
    if (max < levels) return NULL;

    // Allocate Memory for Information Structure
    Base *base = malloc(sizeof(Base));
    if (base == NULL) return NULL;

    // Allocation Mode
    bool dynamic = allocMode == ALLOC_DYNAMIC;

    // Populate Information Structure
    base->levels = levels;
    base->superc = max - levels + 1;
    base->dynamic = dynamic;

    // Allocate Memory for Root Node
    Node *root = malloc(sizeof(Node));
    if (root == NULL) return NULL;
    base->root = root;

    // Initialize Root Node Values to Defaults
    root->supers = NULL;
    root->flag = false;

    // If we're using static allocation, allocate the entire tree
    if (!dynamic)
    {
        int res = nodeAllocDescs(root, levels, max - levels + 1);
        if (res == -1) {
            treeRelease(base);
            return NULL;
        }
    }

    return base;
}

// Release a Tree (Deallocate/Free)
void treeRelease(Base *base)
{
    // Deallocate Entire Tree
    nodeFreeDescs(base->root, base->levels, base->superc);

    // Deallocate Root Node
    free(base->root);

    // Deallocate Information Strucure
    free(base);

    return;
}

// Mark a Certain Set and Supersets
// Returns 0 on success, or 1 if at least 1 newly marked set, -1 on
// memory error, -2 on input error
int treeMark(const Base *base,
        const unsigned long *values, size_t valuec)
{
    // If tree doesn't exist, exit
    if (base == NULL) return -1;

    // First Relative Value (must be 1 or greater)
    if (values[0] < 1) return -2;
    unsigned long rel = values[0] - 1;

    // Allocate Memory for Relative Values
    unsigned long *rels = calloc(valuec - 1, sizeof(unsigned long));
    if (rels == NULL) return -1;

    // Translate Values into Relative Values, or Child Indices
    for (size_t i = 1; i < valuec; i++)
    {
        // Values must be below the maximum of the lowest level, and
        // must be in ascending order without repetition
        if (values[i] >= base->superc + base->levels
                || values[i] <= values[i - 1])
        {
            free(rels);
            return -2;
        }

        // Difference of Values, Subtract One because No Repetition
        rels[i - 1] = values[i] - values[i - 1] - 1;
    }

    // Flag Nodes Appropriately
    int res = nodeFlag(base->root, base->levels, base->superc,
            rel, rels, valuec - 1,
            base->dynamic);

    // Deallocate Memory
    free(rels);

    return res;
}

// Query (Un)Marked Sets
// Returns number of sets on success, -1 on memory error
long long treeQuery(const Base *base, QueryMode mode,
        void (*out)(const unsigned long *, size_t))
{
    // If tree doesn't exist, exit
    if (base == NULL) return -1;

    // Allocate Memory to Store Relative Values
    unsigned long *rels = calloc(base->levels, sizeof(unsigned long));
    if (rels == NULL) return -1;

    // Print Nodes
    long long n = nodeQuery(base->root, base->levels, base->superc,
            rels, base->levels, mode, out);

    // Deallocate Memory
    free(rels);

    return n;
}

// ============ Helper Functions

// These functions are helper functions for the main user-level
// functions. They're defined separately so that they can have a cleaner
// recursive definition, independent of the nodes' proper corresponding
// values, which can be viewed as a user-level abstraction. Due to the
// consistent pattern, we can simply ignore them here.

// Recursively Flag Tree Nodes
// Returns 0 on success, or 1 if at least 1 newly flagged node, -1 on
// memory error

// This function is a recursive function for flagging nodes which have
// particular ancestors. It uses the standard method for traversal. In
// addition, it takes in a set of constraining 'relative values,' which
// work like child node indices while representing the node's proper
// value, so that the function need not keep track of its current
// position. For example, if the proper values are 1 and 4, the relative
// values would be 0 and 2.

// When a node has a child with the next constraining value (i.e. the
// relative value is less than the child counter), the function recurses
// on it, unless there are no more constraining values, in which case a
// satisfactory node has been found and is flagged. Flagging a node
// means that all its descendant nodes are treated as though they are
// flagged already.

// The function also needs to account for paths through the tree that
// contain intermediary values. For example, if the constraining values
// are 2 and 5, the path 2 -> 3 -> 5 is satisfactory, even though 5 is
// not immediately following 2. It is only worth dealing with
// intermediary values at all if there are more levels remaining in the
// tree than constraining values (as otherwise we would never reach the
// final one).

// Due to the nature of the main program, it's useful to know whether
// the nodes we try to flag were already flagged before we got here. So
// we keep a boolean variable that's set to false upon coming across a
// node that's already flagged, and return 1 if it's still true after
// looking at everything.
int nodeFlag(Node *node, size_t levels, unsigned long superc,
        unsigned long rel, const unsigned long *rels, size_t relc,
        bool dynamic)
{
    // If this node doesn't exist, exit
    if (node == NULL) return -1;

    // If we got to the lowest level, there's nothing to do
    if (levels == 0) return 0;

    // If this node is already flagged, no need to go further
    if (node->flag) return 0;

    // Make sure child nodes are allocated, as we're definitely going to
    // be flagging a descendant
    if (dynamic) nodeAllocChildren(node, levels, superc);

    // Keep track of any newly flagged nodes
    bool newlyFlagged = false;

    // This node has a child that represents the value we want
    if (rel < superc)
    {
        // The Child Node of this Value
        Node *super = node->supers + rel;

        // If there are no further value constraints, this node is
        // satisfactory
        if (relc == 0)
        {
            // Keep track if this node is newly flagged
            newlyFlagged = newlyFlagged || !super->flag;
            super->flag = true;

            // The node's children don't matter anymore
            if (dynamic) nodeFreeDescs(super, levels - 1, superc - rel);
        }

        // Otherwise, recurse on that child node, shifting the set of
        // constraints up
        else {
            int res = nodeFlag(super, levels - 1, superc - rel,
                rels[0], rels + 1, relc - 1,
                dynamic);

            // Pass along an error and otherwise keep track
            if (res == -1) return -1;
            newlyFlagged = newlyFlagged || res == 1;
        }
    }

    // We have spare levels, so enumerate intermediary values
    if (relc + 1 < levels)
        for (unsigned long i = 0; i < rel && i < superc; i++)
        {
            // Child Node
            Node *super = node->supers + i;

            // Recurse with the new node; adjust the relative value
            // similarly to child count, but decrement as we're passing
            // to a lower level
            int res = nodeFlag(super, levels - 1, superc - i,
                    rel - i - 1, rels, relc,
                    dynamic);

            // Pass along an error and otherwise keep track
            if (res == -1) return -1;
            newlyFlagged = newlyFlagged || res == 1;
        }

    return newlyFlagged ? 1 : 0;
}

// Recursively Query Nodes
// Returns number of sets on success, -1 on memory error

// This is a recursive function for querying sets descending from a node
// based on marked/unmarked status. It uses the standard method for
// traversal while keeping a record of the child indices of its current
// path, which are used as relative values when outputting the final
// set.

// There are three modes of querying: UNMARKED, MARKED, and ALL. If the
// node is flagged, it will either return immediately if it's in
// UNMARKED mode or simply switch into ALL mode. When in ALL mode, the
// flag status of a node doesn't matter, and in fact, nothing about the
// node matters, as the descendant nodes and eventual sets can just be
// algorithmically generated.
long long nodeQuery(const Node *node, size_t levels, unsigned long superc,
        unsigned long *rels, size_t relc, QueryMode mode,
        void (*out)(const unsigned long *, size_t))
{
    // If we're not outputting every set, the state of the current node
    // matters
    if (mode != QUERY_SETS_ALL)
    {
        // If this node doesn't exist, exit
        if (node == NULL) return -1;

        // Node is flagged, assume no allocated children
        else if (node->flag)
        {
            // Descendant sets are considered marked regardless, so
            // either stop or make sure to output everything
            if (mode == QUERY_SETS_UNMARKED) return 0;
            else mode = QUERY_SETS_ALL;
        }

        // Not flagged, but no allocated children
        else if (node->supers == NULL)
        {
            // Descendant sets are all considered unmarked, so either
            // stop or make sure to output everything
            if (mode == QUERY_SETS_MARKED) return 0;
            else mode = QUERY_SETS_ALL;
        }
    }

    // Counter for number of sets
    long long setc = 0;

    // If we are at the bottom of the tree, output and count the set
    // (unless we're still in MARKED mode, in which case we already know
    // it isn't)
    if (levels == 0) {
        if (mode != QUERY_SETS_MARKED)
        {
            setPass(rels, relc, out);
            setc = 1;
        }
    }

    // Otherwise, iterate through children
    else for (unsigned long i = 0; i < superc; i++)
    {
        // Keep track of relative value
        rels[relc - levels] = i;

        // If we're in the ALL mode, we can't rely on there being a
        // child node actually allocated, so pass in a null-pointer and
        // just count
        Node *next = NULL;

        // Otherwise, pass in the actual child node
        if (mode != QUERY_SETS_ALL) next = node->supers + i;

        // Simple traversal recurse
        long long n = nodeQuery(next, levels - 1, superc - i,
                rels, relc, mode, out);

        // Propogate any error
        if (n == -1) return -1;

        // Add to counter
        setc += n;
    }

    return setc;
}

// Pass On a Set from Relative Values
void setPass(const unsigned long *rels, size_t relc,
        void (*out)(const unsigned long *, size_t))
{
    // Proper values
    unsigned long value = 0;
    unsigned long *set = calloc(relc, sizeof(unsigned long));
    if (set == NULL) return;

    // Iterate over elements
    for (size_t i = 0; i < relc; i++)
    {
        // Proper value is incremented each level and offset by relative
        // value, stored in set
        value += rels[i] + 1;
        set[i] = value;
    }

    // Pass to function
    if (out != NULL) out(set, relc);

    // Deallocate Memory
    free(set);

    return;
}

// ============ Allocation Functions

// These functions are for allocating and deallocating tree nodes.

// Allocate Child Nodes if Unallocated
// Returns 0 on success, -1 on memory error

// This function allocates a node's children if they aren't already
// allocated. It isn't recursive, meaning it will only allocate the
// direct children of the node, and not their children.
int nodeAllocChildren(Node *node, size_t levels, unsigned long superc)
{
    // If this node doesn't exist, exit
    if (node == NULL) return -1;

    // If we're at the bottom level, exit
    if (levels == 0) return 0;

    // If this node already has children, exit
    if (node->supers != NULL) return 0;

    // Allocate Array of Child Pointers
    node->supers = calloc(superc, sizeof(Node));
    if (node->supers == NULL) return -1;

    // Iteratively Allocate and Initialize Child Nodes
    for (unsigned long i = 0; i < superc; i++)
    {
        node->supers[i].supers = NULL;
        node->supers[i].flag = false;
    }

    return 0;
}

// Recursively Allocate Descendant Nodes
// Returns 0 on success, -1 on memory error

// This is a recursive function for allocating a node's descendant
// nodes. It uses the standard method for traversal to allocate nodes
// and arrays for children.
int nodeAllocDescs(Node *node, size_t levels, unsigned long superc)
{
    // If this node doesn't exist, exit
    if (node == NULL) return -1;

    // If we're at the bottom level, exit
    if (levels == 0) return 0;

    // Allocate children if necessary
    int res = nodeAllocChildren(node, levels, superc);
    if (res == -1) return -1;

    // We're going down one level
    if (levels - 1 == 0) return 0;

    // Iterate over Children
    for (unsigned long i = 0; i < superc; i++)
    {
        // Child Node
        Node *child = node->supers + i;

        // Allocate the Child's Children
        res = nodeAllocDescs(child, levels - 1, superc - i);
        if (res == -1) return -1;
    }

    return 0;
}

// Recursively Deallocate Descendant Nodes

// This function works in a similar fashion to the allocation function,
// except that it has to free memory AFTER recursing on children rather
// than before. I think it's fairly obvious why this is.
void nodeFreeDescs(Node *node, size_t levels, unsigned long superc)
{
    // If this node doesn't exist, exit
    if (node == NULL) return;

    // Check if there are children to deallocate in the first place
    if (levels != 0 && node->supers != NULL)
    {
        // Iterate over Children
        for (unsigned long i = 0; i < superc; i++)
        {
            // Child Node
            Node *child = node->supers + i;

            // Recursively deallocate further descendants
            nodeFreeDescs(child, levels - 1, superc - i);
        }

        // Deallocate the array of Children
        free(node->supers);
    }

    // Set to a null pointer to indicate there are no children
    node->supers = NULL;

    return;
}
