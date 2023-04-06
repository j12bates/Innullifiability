// DEMO PROGRAM FOR NOW

#include <stdio.h>

#include "setRec.h"

void printSet(const unsigned long *, size_t);

int main(int argc, char *argv[])
{
    SR_Base *rec = sr_initialize(4, 8);
    if (rec == NULL) return 1;
    printf("Record Initialized\n\n");

    long long markReturnCode;

    unsigned long subset[2] = {2, 4};
    markReturnCode = sr_mark(rec, subset, 2);
    printf("Sets containing 2 and 4 marked, %d\n", markReturnCode);

    markReturnCode = sr_mark(rec, subset, 2);
    printf("Marked again, %d\n", markReturnCode);

    unsigned long three[1] = {3};
    markReturnCode = sr_mark(rec, three, 1);
    printf("Sets containing 3 marked, %d\n", markReturnCode);

    unsigned long anotherThree[2] = {3, 5};
    markReturnCode = sr_mark(rec, anotherThree, 2);
    printf("Marked again with added constraint 5, %d\n", markReturnCode);

    unsigned long invalid[2] = {6, 4};
    markReturnCode = sr_mark(rec, invalid, 2);
    printf("Should be an error, %d\n\n", markReturnCode);

    long long queryReturnCode;

    queryReturnCode = sr_query(rec, QUERY_SETS_ALL, &printSet);
    printf("\nShould be all the sets, %d\n\n", queryReturnCode);

    queryReturnCode = sr_query(rec, QUERY_SETS_MARKED, &printSet);
    printf("\nShould be all those we marked, %d\n\n", queryReturnCode);

    queryReturnCode = sr_query(rec, QUERY_SETS_UNMARKED, &printSet);
    printf("\nShould be all the others, %d\n\n", queryReturnCode);

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
