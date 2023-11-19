// ========================== UTIL INTERFACE ===========================

// Copyright (c) 2023, Jacob Bates
// SPDX-License-Identifier: BSD-2-Clause

// Some common functions used by the utility programs. Particularly File
// I/O and Command-Line Argument stuff. Needs to be linked with every
// utility program.

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include "iface.h"
#include "setRec.h"

const char *userHeader1 =
        "Maximum M-value of Contiguous Complete Source (positive for "
        "actual number, -1 for weeded): %ld\n";

// Open Record File and Import
// Returns 0 on success, 1 on error
int openImport(SR_Base *rec, char *fname, struct RecExts *exts)
{
    // Open File
    FILE *f = fopen(fname, "rb");
    if (f == NULL) {
        fprintf(stderr, "Error on Opening '%s': %s\n",
                fname, strerror(errno));
        return 1;
    }

    // Read User Header
    long headerCSMM;
    fscanf(f, userHeader1, &headerCSMM);
    if (exts != NULL) {
        exts->completeSourceMaxM = headerCSMM;
        exts->weeded = headerCSMM == -1;
    }

    // Import Record
    int res = sr_import(rec, f);
    if (res) {
        if (res == -1)
            fprintf(stderr, "Error on Importing '%s': %s\n",
                    fname, strerror(errno));
        else if (res == -2)
            fprintf(stderr, "Error on Importing '%s': %s\n",
                    fname, "Wrong Size");
        else if (res == -3)
            fprintf(stderr, "Error on Importing '%s': %s\n",
                    fname, "Invalid Record File");
    }

    fclose(f);
    return res != 0;
}

// Open File and Export Record
// Returns 0 on success, 1 on error
int openExport(SR_Base *rec, char *fname, struct RecExts *exts)
{
    // Open File
    FILE *f = fopen(fname, "wb");
    if (f == NULL) {
        fprintf(stderr, "Error on Opening '%s': %s\n",
                fname, strerror(errno));
        return 1;
    }

    // Write User Header
    long headerCSMM = 0;
    if (exts != NULL) {
        if (exts->weeded) headerCSMM = -1;
        else headerCSMM = exts->completeSourceMaxM;
    }
    fprintf(f, userHeader1, headerCSMM);

    // Export Record
    int res = sr_export(rec, f);
    if (res == -1)
        fprintf(stderr, "Error on Exporting '%s': %s\n",
                fname, strerror(errno));

    fclose(f);
    return res != 0;
}

// Push Progress Update
// Returns 0 on success, 1 on error
int pushProg(size_t prog, size_t total, size_t output, char *fname)
{
    int res;

    // Raw Number Buffers, Fixed Size
    uint64_t buf[3] = {prog, total, output};

    // Open Progress File
    int fd = open(fname, O_WRONLY | O_TRUNC);
    if (fd < 0) return 1;

    // Output Progress and Close
    res = write(fd, buf, sizeof(buf));
    if (res != sizeof(buf)) return 1;

    res = close(fd);
    if (res == -1) return 1;

    return 0;
}

// Parse Command-Line Arguments
// Returns 0 on success, 1 on invalid arguments

// This function is for parsing command-line arguments. It takes a null-
// terminated array of symbols for argument formats, which are used to
// interpret the actual command-line args and then set the references
// provided as the variable number of arguments. A certain number of
// required parameters may be specified. On invalid arguments, this will
// print the usage message and error out.
int argParse(const Param *params, int reqd, const char *usage,
        int argc, char **argv, ...)
{
    // Check if options are present
    _Bool optsPresent = 0;
    if (argc > 1) optsPresent = argv[1][0] == '-';
    int first = 1 + optsPresent;

    // If not enough arguments, invalid usage
    if (argc - first < reqd) goto usage;

    // Go Through Arguments
    va_list ap;
    va_start(ap, argv);
    for (int i = first; i < argc; i++)
    switch (params[i - first])
    {
        char *endptr;

    // For numeric parameters, interpret argument as an integer, check
    // for errors
    case PARAM_VAL:
        {
            unsigned long *p = va_arg(ap, unsigned long *);
            *p = strtoul(argv[i], &endptr, 0);
        }
        goto errCk;

    case PARAM_CT:
    case PARAM_SIZE:
        {
            size_t *p = va_arg(ap, size_t *);
            *p = strtoul(argv[i], &endptr, 0);
        }

    errCk:
        if (*endptr != '\0') errno = EINVAL;
        if (errno) {
            fprintf(stderr, "argv[%d] ", i);
            perror("Validation");
            goto invalid;
        }
        break;

    // For string parameters, just point to the argument
    case PARAM_FNAME:
    case PARAM_STR:
        {
            char **p = va_arg(ap, char **);
            *p = argv[i];
        }
        break;

    // If we hit the end, invalid usage
    default:
        goto invalid;
    }

    va_end(ap);
    return 0;

invalid:
    va_end(ap);

usage:
    // Show Usage Message, error out
    fprintf(stderr, usage, argv[0]);
    errno = EINVAL;

    return 1;
}

// Handle Command-Line Options
// Returns 0 on success, 1 on invalid option

// This is for handling options provided in command-line args. The
// string provides the valid option characters, and the variable
// arguments are references to booleans to set according to whether the
// corresponding option was used. On invalid option, usage message is
// printed and an error is returned.
int optHandle(const char *opts, _Bool setting, const char *usage,
        int argc, char **argv, ...)
{
    // Options passed: none or first arg if leading hyphen
    char *passed = "";
    if (argc > 1) if (argv[1][0] == '-') passed = argv[1] + 1;
    size_t passc = strlen(passed);

    // For remembering whether or not an option was used
    size_t optc = strlen(opts);
    _Bool used[optc];
    for (size_t i = 0; i < optc; i++) used[i] = 0;

    // Go through the passed options and mark them in
    for (size_t i = 0; i < passc; i++) {
        char *opt = strchr(opts, passed[i]);
        if (opt == NULL) {
            fprintf(stderr, "Invalid option '%c'\n", passed[i]);
            fprintf(stderr, usage, argv[0]);
            errno = EINVAL;
            return 1;
        }
        used[opt - opts] = 1;
    }

    // Set references from how we marked each option
    va_list ap;
    va_start(ap, argv);
    for (size_t i = 0; i < optc; i++) {
        _Bool *flag = va_arg(ap, _Bool *);
        *flag = used[i] == setting;
    }
    va_end(ap);

    return 0;
}

// Safely Exit
_Noreturn void safeExit(void)
{
    // exit() isn't thread-safe
    static pthread_mutex_t exitLock = PTHREAD_MUTEX_INITIALIZER;

    pthread_mutex_lock(&exitLock);
    exit(1);
}
