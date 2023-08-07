// DEMO PROGRAM FOR NOW

#include <stdio.h>

#include "../lib/setRec.h"

#define SINGLE  1 << 0
#define SKIP    1 << 1

unsigned long setv[4][3] = {
    {1, 2, 3},
    {2, 3, 4},
    {3, 4, 5},
    {4, 5, 6}
};

unsigned long special[2][3] = {
    {1, 3, 5},
    {2, 4, 6}
};

void printSet(const unsigned long *, size_t);

int main(int argc, char *argv[])
{
    SR_Base *rec = sr_initialize(3);
    if (rec == NULL) return 1;
    int res = sr_alloc(rec, 0, 6);
    if (res) return 1;
    printf("Record Initialized\n\n");

    FILE *f;

    f = fopen("record.dat", "rb");
    if (f == NULL) perror("Import Error");
    else {
        int res = sr_import(rec, f);
        if (res == -1) perror("Import Error");
        if (res == -2) fprintf(stderr, "Incorrect Record Size\n");
        if (res == -3) fprintf(stderr, "Invalid File\n");
        fclose(f);
    }

    unsigned long minm = sr_getMinM(rec);
    unsigned long maxm = sr_getMaxM(rec);
    printf("M-Value Ranges from %lu to %lu\n\n", minm, maxm);

    long long markReturnCode;

    for (size_t i = 0; i < 4; i++) {
        markReturnCode = sr_mark(rec, setv[i], 3, SINGLE);
        printf("%lld, ", markReturnCode);
    }
    printf("\nSingles bit 0\n\n");

    for (size_t i = 0; i < 2; i++) {
        markReturnCode = sr_mark(rec, special[i], 3, SKIP);
        printf("%lld, ", markReturnCode);
    }
    printf("\nSkips bit 1\n\n");

    unsigned long anotherSpecial[3] = {2, 3, 4};
    markReturnCode = sr_mark(rec, anotherSpecial, 3, SKIP);
    printf("%lld, (2, 3, 4) bit 1\n", markReturnCode);

    markReturnCode = sr_mark(rec, anotherSpecial, 3, SKIP);
    printf("%lld, Repeat Previous\n", markReturnCode);

    unsigned long invalid[2] = {6, 4};
    markReturnCode = sr_mark(rec, invalid, 2, ~0);
    printf("%lld, Should be an error\n\n", markReturnCode);

    long long queryReturnCode;

    queryReturnCode = sr_query(rec, 0, 0, &printSet);
    printf("\nShould be all the sets, %d\n\n", queryReturnCode);

    queryReturnCode = sr_query_parallel(rec, 0, 0, 2, 1, &printSet);
    printf("\nShould be every other set, %d\n\n", queryReturnCode);

    queryReturnCode = sr_query(rec, SINGLE, SINGLE, &printSet);
    printf("\nShould be those with bit 0 marked, %d\n\n",
            queryReturnCode);

    queryReturnCode = sr_query(rec, SKIP, SKIP, &printSet);
    printf("\nShould be those with bit 1 marked, %d\n\n",
            queryReturnCode);

    queryReturnCode = sr_query(rec, SINGLE | SKIP, SINGLE,
            &printSet);
    printf("\nShould be those with bit 0 marked and bit 1 unmarked, "
            "%d\n\n", queryReturnCode);

    queryReturnCode = sr_query(rec, 0, SINGLE | SKIP, &printSet);
    printf("\nShould be those with either bit marked, %d\n\n",
            queryReturnCode);

    queryReturnCode = sr_query(rec, SINGLE | SKIP, 0, &printSet);
    printf("\nShould be all the completely unmarked sets, %d\n\n",
            queryReturnCode);

    f = fopen("record.dat", "wb");
    if (f == NULL) perror("Export Error");
    else {
        if (sr_export(rec, f) == -1) perror("Export Error");
        fclose(f);
    }

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
