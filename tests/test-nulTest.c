// DEMO PROGRAM FOR NOW

#include <stdio.h>

#include "../nulTest/nulTest.h"

int main(int argc, char *argv[])
{
    int n;

    unsigned long exInnullifiable[4] = {1, 4, 6, 8};
    n = nulTest(exInnullifiable, 4);
    printf("(1,4,6,8): %d\n", n);

    unsigned long exNullifiable[4] = {1, 4, 6, 9};
    n = nulTest(exNullifiable, 4);
    printf("(1,4,6,9): %d\n", n);

    unsigned long singletonZero[1] = {0};
    n = nulTest(singletonZero, 1);
    printf("(0): %d\n", n);

    unsigned long singletonNonzero[1] = {2};
    n = nulTest(singletonNonzero, 1);
    printf("(1): %d\n", n);

    unsigned long anotherThing[3] = {15, 2, 6};
    n = nulTest(anotherThing, 3);
    printf("(15, 2, 6): %d\n", n);

    return 0;
}
