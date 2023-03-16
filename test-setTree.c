// DEMO PROGRAM FOR NOW

#include <stdio.h>

#include "setTree.h"

void printSet(const unsigned long *, size_t);

int main(int argc, char *argv[])
{
    Base *tree = treeConstruct(3, 6);
    if (tree == NULL) return 1;
    printf("Tree constructed\n\n");

    int markReturnCode;

    unsigned long subset[2] = {2, 4};
    markReturnCode = treeMark(tree, subset, 2);
    printf("Sets containing 2 and 4 marked, %d\n", markReturnCode);

    unsigned long three[1] = {3};
    markReturnCode = treeMark(tree, three, 1);
    printf("Sets containing 3 marked, %d\n\n", markReturnCode);

    unsigned long invalid[2] = {7, 2};
    markReturnCode = treeMark(tree, invalid, 2);
    printf("Invalid mark, %d\n\n", markReturnCode);

    long long queryReturnCode;

    queryReturnCode = treeQuery(tree, PRINT_SETS_ALL, &printSet);
    printf("Should be all the sets, %d\n\n", queryReturnCode);

    queryReturnCode = treeQuery(tree, PRINT_SETS_MARKED, &printSet);
    printf("Should be all those we marked, %d\n\n", queryReturnCode);

    queryReturnCode = treeQuery(tree, PRINT_SETS_UNMARKED, &printSet);
    printf("Should be all the others, %d\n\n", queryReturnCode);

    queryReturnCode = treeQuery(NULL, PRINT_SETS_ALL, &printSet);
    printf("Should be an error, %d\n\n", queryReturnCode);

    treeDestruct(tree);
    printf("Tree freed\n");

    return 0;
}

void printSet(const unsigned long *set, size_t setc)
{
    for (size_t i = 0; i < setc; i++)
        printf("%c%d", (i == 0 ? '(' : ','), set[i]);

    printf("%c ", ')');

    return;
}
