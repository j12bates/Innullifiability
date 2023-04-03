// DEMO PROGRAM FOR NOW

#include <stdio.h>

#include "setRec.h"

void printSet(const unsigned long *, size_t);

int main(int argc, char *argv[])
{
    SR_Base *rec = sr_initialize(3, 6);
    if (rec == NULL) return 1;
    printf("Record Initialized\n");

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
