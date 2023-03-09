// DEMO PROGRAM FOR NOW

#include <stdio.h>

#include "setTree.h"

int main(int argc, char *argv[])
{
    Base *tree = treeConstruct(3, 6);
    printf("Tree constructed\n\n");

    unsigned long subset[2] = {2, 4};
    treeMark(tree, subset, 2);
    printf("Sets containing 2 and 4 marked\n");

    unsigned long three[1] = {3};
    treeMark(tree, three, 1);
    printf("Sets containing 3 marked\n\n");

    treePrint(tree, PRINT_SETS_ALL);
    printf("Should be all the sets\n\n");

    treePrint(tree, PRINT_SETS_MARKED);
    printf("Should be all those sets we marked\n\n");

    treePrint(tree, PRINT_SETS_UNMARKED);
    printf("Should be all the others\n\n");

    treeDestruct(tree);
    printf("Tree freed\n");

    return 0;
}
