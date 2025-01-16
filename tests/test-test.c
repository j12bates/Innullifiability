#include <stdio.h>

#include "../lib/test.h"

int main(int argc, char **argv)
{
    size_t size = strtoul(argv[1], NULL, 0);

    unsigned long set[size];
    for (size_t i = 0; i < size; i++)
        set[i] = strtoul(argv[2 + i], NULL, 0);

    if (test(set, size, 0, 0)) printf("Nullifiable.\n");
    else printf("Innullifiable.\n");

    return 0;
}
