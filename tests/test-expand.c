#include <stdio.h>
#include <stdlib.h>

#include "../lib/expand.h"

void expandPrint(const unsigned long *, size_t);
void printSet(const unsigned long *, size_t);

const unsigned long max = 15;

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

    printf("Supersets: ");
    res = expand(set, size, max, EXPAND_SUPERS, &printSet);
    printf("\n");
    if (res) perror("Thing");

    printf("Additive Mutations: ");
    res = expand(set, size, max, EXPAND_MUT_ADD, &printSet);
    printf("\n");
    if (res) perror("Thing");

    printf("Multiplicative Mutations: ");
    res = expand(set, size, max, EXPAND_MUT_MUL, &printSet);
    printf("\n");
    if (res) perror("Thing");

    return;
}

void printSet(const unsigned long *set, size_t size)
{
    for (size_t i = 0; i < size; i++)
        printf("%c%d", i == 0 ? '(' : ',', set[i]);
    printf("%c ", ')');
}
