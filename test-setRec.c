// DEMO PROGRAM FOR NOW

#include <stdio.h>

#include "setRec.h"

#define PATTERN 1 << 1
#define THREE   1 << 2

void printSet(const unsigned long *, size_t);

int main(int argc, char *argv[])
{
    SR_Base *rec = sr_initialize(4, 7);
    if (rec == NULL) return 1;
    printf("Record Initialized\n\n");

    long long markReturnCode;

    unsigned long subset[2] = {2, 4};
    markReturnCode = sr_mark(rec, subset, 2, PATTERN, PATTERN);
    printf("Marked (2,4) bit 0 \t\t%d\n", markReturnCode);

    markReturnCode = sr_mark(rec, subset, 2, PATTERN, PATTERN);
    printf("Repeat Previous \t\t%d\n", markReturnCode);

    unsigned long three[1] = {3};
    markReturnCode = sr_mark(rec, three, 1, THREE, THREE);
    printf("Marked (3) bit 1 \t\t%d\n", markReturnCode);

    unsigned long anotherThree[2] = {3, 5};
    markReturnCode = sr_mark(rec, anotherThree, 2, PATTERN, PATTERN);
    printf("Marked (3,5) bit 0 \t\t%d\n", markReturnCode);

    unsigned long yetAnotherThree[3] = {3, 5, 7};
    markReturnCode = sr_mark(rec, yetAnotherThree, 3, PATTERN, 0);
    printf("Unmarked (3,5,7) bit 0 \t\t%d\n", markReturnCode);

    unsigned long invalid[2] = {6, 4};
    markReturnCode = sr_mark(rec, invalid, 2, ~0, 0);
    printf("Should be an error \t\t%d\n\n", markReturnCode);

    long long queryReturnCode;

    queryReturnCode = sr_query(rec, 0, 0, &printSet);
    printf("\nShould be all the sets, %d\n\n", queryReturnCode);

    queryReturnCode = sr_query(rec, PATTERN, PATTERN, &printSet);
    printf("\nShould be those with bit 0 marked, %d\n\n",
            queryReturnCode);

    queryReturnCode = sr_query(rec, THREE, THREE, &printSet);
    printf("\nShould be those with bit 1 marked, %d\n\n",
            queryReturnCode);

    queryReturnCode = sr_query(rec, PATTERN | THREE, PATTERN,
            &printSet);
    printf("\nShould be those with bit 0 marked and bit 1 unmarked, "
            "%d\n\n", queryReturnCode);

    queryReturnCode = sr_query(rec, 0, PATTERN | THREE, &printSet);
    printf("\nShould be those with either bit marked, %d\n\n",
            queryReturnCode);

    queryReturnCode = sr_query(rec, PATTERN | THREE, 0, &printSet);
    printf("\nShould be all the completely unmarked sets, %d\n\n",
            queryReturnCode);

    sr_release(rec);
    printf("Record Released\n");

    return 0;
}

void printSet(const unsigned long *set, size_t setc)
{
    for (size_t i = 0; i < setc; i++)
        printf("%c%d", (i == 0 ? '(' : ','), set[i]);

    printf("%c ", ')');

    return;
}
