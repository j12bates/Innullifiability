// DEMO PROGRAM FOR NOW

#include <stdio.h>

#include "../setTree/setTree.h"

void printSet(const unsigned long *, size_t);

int main(int argc, char *argv[])
{
    TreeBase *tree = treeInitialize(3, 6, ALLOC_DYNAMIC);
    if (tree == NULL) return 1;
    printf("Tree constructed\n\n");

    int markReturnCode;

    unsigned long subset[2] = {2, 4};
    markReturnCode = treeMark(tree, subset, 2);
    printf("Sets containing 2 and 4 marked, %d\n", markReturnCode);

    markReturnCode = treeMark(tree, subset, 2);
    printf("Marked again, %d\n", markReturnCode);

    unsigned long three[1] = {3};
    markReturnCode = treeMark(tree, three, 1);
    printf("Sets containing 3 marked, %d\n", markReturnCode);

    unsigned long anotherThree[2] = {3, 5};
    markReturnCode = treeMark(tree, anotherThree, 2);
    printf("Marked again with added condition 5, %d\n\n", markReturnCode);

    unsigned long invalid[2] = {7, 2};
    markReturnCode = treeMark(tree, invalid, 2);
    printf("Invalid mark, %d\n\n", markReturnCode);

    long long queryReturnCode;

    queryReturnCode = treeQuery(tree, QUERY_SETS_ALL, &printSet);
    printf("\nShould be all the sets, %d\n\n", queryReturnCode);

    queryReturnCode = treeQuery(tree, QUERY_SETS_MARKED, &printSet);
    printf("\nShould be all those we marked, %d\n\n", queryReturnCode);

    queryReturnCode = treeQuery(tree, QUERY_SETS_UNMARKED, &printSet);
    printf("\nShould be all the others, %d\n\n", queryReturnCode);

    queryReturnCode = treeQuery(NULL, QUERY_SETS_ALL, &printSet);
    printf("\nShould be an error, %d\n\n", queryReturnCode);

    treeRelease(tree);
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
