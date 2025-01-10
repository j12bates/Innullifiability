#include <stdio.h>
#include <stdlib.h>

#include "../lib/multi.h"

void printSet(const unsigned long *, size_t);

int main(int argc, char **argv)
{
    unsigned long fixedv[2] = {13, 15};
    unsigned long minM = 9, maxM = 11;
    size_t destSize = strtoul(argv[1], NULL, 0);

    size_t srcSize = argc - 2;
    unsigned long set[srcSize];
    for (int i = 0; i < srcSize; i++)
        set[i] = strtoul(argv[i + 2], NULL, 0);

    multiExpand(set, srcSize, minM, maxM,
            2, fixedv, destSize, &printSet);

    return 0;
}

void printSet(const unsigned long *set, size_t size)
{
    for (size_t i = 0; i < size; i++)
        printf("%4zu", set[i]);
    printf("\n");
    return;
}
