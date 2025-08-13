#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "../lib/reduce.h"

void expandPrint(const unsigned long *, size_t);
int printSet(const unsigned long *, size_t);
int printSetIdx(const unsigned long *, size_t, size_t);

const unsigned long minM = 13;
const unsigned long maxM = 15;
const unsigned long destSize = 4;

int main(int argc, char **argv)
{
    unsigned long set[argc - 1];

    for (int i = 1; i < argc; i++)
        set[i - 1] = strtoul(argv[i], NULL, 10);

    expandPrint(set, argc - 1);

    return 0;
}

void expandPrint(const unsigned long *set, size_t size)
{
    int res;

    printf("Subsets: ");
    res = subset(set, size, minM, maxM, true, destSize, &printSet);
    printf("\n");
    if (res) perror("Thing");

    printf("Contractions: ");
    res = contraction(set, size, minM, maxM, true, destSize,
            &printSetIdx);
    printf("\n");
    if (res) perror("Thing");

    return;
}


int printSet(const unsigned long *set, size_t size)
{
    for (size_t i = 0; i < size; i++)
        printf("%c%d", i == 0 ? '(' : ',', set[i]);
    printf("%c ", ')');

    return 0;
}

int printSetIdx(const unsigned long *set, size_t size, size_t idx)
{
    for (size_t i = 0; i < size; i++) {
        char emph = idx == i ? '*' : ' ';
        printf("%c%c%d%c", i == 0 ? '(' : ',', emph, set[i], emph);
    }
    printf("%c ", ')');

    return 0;
}
