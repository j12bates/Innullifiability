// ============================ GENERATION =============================

// This program takes in two records, a source and a destination, and it
// performs a generation. Given that all the marked sets on the source
// correspond to nullifiable sets, the program will mark all their
// supersets on the destination, as all supersets of a nullifiable set
// are also nullifiable. Then, it will also expand any of those sets
// that don't have the second bit set by introducing mutations to the
// values. Each of these expansion phases are optional.

#include <stdio.h>
#include <stdlib.h>

#include <errno.h>

#include "setRec/setRec.h"
#include "mutate/mutate.h"
#include "supers/supers.h"

#define NULLIF 1 << 0
#define ONLY_SUP 1 << 1
#define MARKED NULLIF | ONLY_SUP

// Number of Threads
unsigned int threads = 1;

// Set Records
SR_Base *src = NULL;
SR_Base *dest = NULL;
size_t srcSize;
char *srcFname, *destFname;

int main(int argc, char **argv)
{
    // Result Storage from Function Calls
    int res;

    // ============ Command-Line Arguments

    // Usage Check
    if (argc < 4) {
        fprintf(stderr, "Usage: %s %s %s %s %s\n",
                argv[0], "srcSize", "src.dat", "dest.dat", "[threads]");
        return 1;
    }

    // Numeric Arguments
    errno = 0;
    srcSize = strtoul(argv[1], NULL, 10);
    if (errno) {
        perror("srcSize argument");
        return 1;
    }

    // Record Filenames
    srcFname = argv[2];
    destFname = argv[3];

    // ============ Import Records

    // Initialize Records
    src = sr_initialize(srcSize);
    dest = sr_initialize(srcSize + 1);

    // Deal with Files
    {
        FILE *f;

        // Open Source Record File
        f = fopen(srcFname, "rb");
        if (f == NULL) {
            perror("src Import");
            return 1;
        }

        // Import if Successful
        else {
            res = sr_import(src, f);
            if (res); // some error checking; gah, I can't figure it out

            fclose(f);
        }

        // Open Destination Record File
        f = fopen(destFname, "rb");
        if (f == NULL) {
            perror("dest Import");
            return 1;
        }

        // Import if Successful
        else {
            res = sr_import(dest, f);
            if (res); // same as above

            fclose(f);
        }
    }

    // Record Information
    printf("src  - Size: %2zu; M: %4lu to %4lu\n",
            sr_getSize(src), sr_getMinM(src), sr_getMaxM(src));
    printf("dest - Size: %2zu; M: %4lu to %4lu\n",
            sr_getSize(dest), sr_getMinM(dest), sr_getMaxM(dest));

    return 0;
}
