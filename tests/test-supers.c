// DEMO PROGRAM FOR NOW

#include <stdio.h>

#include "../lib/supers.h"

void expandPrint(const unsigned long *, size_t);
void printSet(const unsigned long *, size_t);

int main(int argc, char *argv[])
{
    supersInit(6);
    printf("Initialized with M = 6\n\n");

    int res;

    unsigned long nullThree[2] = {3, 3};
    res = supers(nullThree, 2, &printSet);
    printf("\nExpanded from (3,3), %d\n\n", res);

    unsigned long nullTwo[2] = {2, 2};
    res = supers(nullTwo, 2, &printSet);
    printf("\nExpanded from (2,2), %d\n\n", res);

    unsigned long oneFour[2] = {1, 4};
    res = supers(oneFour, 2, &printSet);
    printf("\nExpanded from (1,4), %d\n\n", res);

    unsigned long justTwo[1] = {2};
    res = supers(justTwo, 1, &printSet);
    printf("\nExpanded from (2), %d\n\n", res);

    res = supers(justTwo, 1, &expandPrint);
    printf("\nExpanded from above expansions of (2), %d\n\n", res);

    return 0;
}

void expandPrint(const unsigned long *set, size_t setc)
{
    int res = supers(set, setc, &printSet);
    printf(", %d\n", res);
    return;
}

void printSet(const unsigned long *set, size_t setc)
{
    for (size_t i = 0; i < setc; i++)
        printf("%c%lu", i == 0 ? '(' : ',', set[i]);
    printf("%c ", ')');
}
