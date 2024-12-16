// ======================= COUNT SETS BY M-VALUE =======================

// Copyright (c) 2024, Jacob Bates
// SPDX-License-Identifier: BSD-2-Clause

// This program takes in a list of sets, such as one produced by the
// Evaluate util, scans for sets with given M-values, and prints out a
// tally for each M-value queried. The list need not be ordered.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Line Buffer
const size_t size_buf = 256;
char *buf;

// Set Counts
size_t *counts;

int main(int argc, char **argv)
{
    // Allocate Memory
    counts = calloc(argc - 1, sizeof(size_t));
    buf = calloc(size_buf, sizeof(char));

    // Read All File Lines
    char *res;
    do {
        res = fgets(buf, size_buf, stdin);
        size_t len = strlen(buf) - 1;

        // Compare this line to each given M-value
        for (int i = 1; i < argc; i++) {
            size_t numlen = strlen(argv[i]);
            if (len < numlen) continue;
            int cmp = strncmp(argv[i], buf + len - numlen, numlen);
            int precedingSpace = buf[len - numlen - 1] == ' ';
            if (cmp == 0 && precedingSpace) counts[i - 1]++;
        }
    }
    while (res);

    // Print the count for each given M-value
    for (int i = 1; i < argc; i++)
        printf("%zu ", counts[i - 1]);
    printf("\n");

    return 0;
}
