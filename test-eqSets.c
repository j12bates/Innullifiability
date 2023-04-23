// DEMO PROGRAM FOR NOW

#include <stdio.h>

#include "eqSets.h"

void expandPrint(const unsigned long *, size_t);
void printSet(const unsigned long *, size_t);

int main(int argc, char *argv[])
{
    eqSetsInit(6);
    printf("Initialized with M = 6\n\n");

    unsigned long nullThree[2] = {3, 3};
    eqSets(nullThree, 2, &printSet);
    printf("\nExpanded from (3,3)\n\n");

    unsigned long nullTwo[2] = {2, 2};
    eqSets(nullTwo, 2, &printSet);
    printf("\nExpanded from (2,2)\n\n");

    unsigned long oneFour[2] = {1, 4};
    eqSets(oneFour, 2, &printSet);
    printf("\nExpanded from (1,4)\n\n");

    unsigned long justTwo[1] = {2};
    eqSets(justTwo, 1, &printSet);
    printf("\nExpanded from (2)\n\n");

    eqSets(justTwo, 1, &expandPrint);
    printf("\nExpanded from above expansions of (2)\n\n");

    eqSetsInit(0);
    printf("Freeing Memory\n");

    return 0;
}

void expandPrint(const unsigned long *set, size_t setc)
{
    eqSets(set, setc, &printSet);
    return;
}

void printSet(const unsigned long *set, size_t setc)
{
    for (size_t i = 0; i < setc; i++)
        printf("%c%d", i == 0 ? '(' : ',', set[i]);
    printf("%c ", ')');
}
