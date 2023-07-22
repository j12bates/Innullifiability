// ======================= COMMON UTIL FUNCTIONS =======================

// Some common functions used by the utility programs. Not implemented
// as a separate object because these are meant to be part of the utils
// themselves, and will simply be '#include'd in.

// Open Record File and Import
int openImport(SR_Base *rec, char *fname)
{
    // Open File
    FILE *f = fopen(fname, "rb");
    if (f == NULL) {
        perror("File Error");
        return 1;
    }

    // Import Record
    int res = sr_import(rec, f);
    if (res) {
        if (res == -1) perror("Import Error");
        else if (res == -2) fprintf(stderr, "Wrong Size\n");
        else if (res == -3) fprintf(stderr, "Invalid Record File\n");
        return 1;
    }

    // Close File
    fclose(f);
    fprintf(stderr, "Success\n");

    return 0;
}

// Open File and Export Record
int openExport(SR_Base *rec, char *fname)
{
    // Open File
    FILE *f = fopen(fname, "wb");
    if (f == NULL) {
        perror("File Error");
        return 1;
    }

    // Export Record
    int res = sr_export(rec, f);
    assert(res != -2);
    if (res == -1) {
        perror("Export Error");
        return 1;
    }

    // Close File
    fclose(f);
    fprintf(stderr, "Success\n");

    return 0;
}

// Parse Command-Line Arguments
// Returns 0 on success, 1 on invalid arguments

// Takes a null-terminated array of symbols that represent argument
// formats, the number of required arguments, the command-line arguments
// themselves, and then references to all the variables to store each
// argument to. Each variable will be updated with the value of the
// argument, unless the argument was not given. Undefined behaviour on
// invalid arguments.
int argParse(const Param *params, int reqd,
        int argc, char **argv, ...)
{
    va_list ap;

    // If not enough arguments, invalid usage
    if (argc - 1 < reqd) return 1;

    // Go Through Arguments
    va_start(ap, argv);
    for (int i = 1; i < argc; i++)
        switch (params[i - 1])
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

end:
    va_end(ap);
    return 0;

invalid:
    va_end(ap);
    return 1;
}
