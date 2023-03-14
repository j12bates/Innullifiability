// DEMO PROGRAM FOR NOW

#include <stdio.h>

#include "eqSets.h"

void printSet(const unsigned long *, size_t);

int main(int argc, char *argv[])
{
    eqSetsInit(6, 3);
    printf("hallo\n");

    unsigned long set[2] = {3, 3};
    eqSets(set, 2, &printSet);
    printf("\n");

    eqSetsInit(0, 0);

    return 0;
}

void printSet(const unsigned long *set, size_t setc)
{
    for (size_t i = 0; i < setc; i++)
        printf("%c%d", i == 0 ? '(' : ',', set[i]);
    printf("%c ", ')');
}
