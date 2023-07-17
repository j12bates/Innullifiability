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
}
