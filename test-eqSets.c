// DEMO PROGRAM FOR NOW

#include <stdio.h>

#include "eqSets.h"

void printSet(const unsigned long *, size_t);

int main(int argc, char *argv[])
{
    eqSetsInit(3, 6);
    printf("Initializing with N = 3, M = 6\n\n");

    unsigned long nullThree[2] = {3, 3};
    eqSets(nullThree, 2, &printSet);
    printf("\nSets equivalent to (3,3)\n\n");

    unsigned long nullTwo[2] = {2, 2};
    eqSets(nullTwo, 2, &printSet);
    printf("\nSets equivalent to (2,2)\n\n");

    unsigned long oneFour[2] = {1, 4};
    eqSets(oneFour, 2, &printSet);
    printf("\nSets equivalent to (1,4)\n\n");

    unsigned long justTwo[1] = {2};
    eqSets(justTwo, 1, &printSet);
    printf("\nSets equivalent to (2)\n\n");

    eqSetsInit(0, 0);
    printf("Freeing Memory\n");

    return 0;
}

void printSet(const unsigned long *set, size_t setc)
{
    for (size_t i = 0; i < setc; i++)
        printf("%c%d", i == 0 ? '(' : ',', set[i]);
    printf("%c ", ')');
}
