// ========================== UTIL INTERFACE ==========================

// Some common macros and type definitions for the utility programs.

#ifndef IFACE_H
#define IFACE_H

#define NULLIF 1 << 0
#define ONLY_SUP 1 << 1
#define MARKED NULLIF | ONLY_SUP

typedef enum ParamType Param;
enum ParamType {
    PARAM_END = 0,
    PARAM_CT,
    PARAM_SIZE,
    PARAM_VAL,
    PARAM_FNAME,
    PARAM_STR
};

// Open Record File and Import
int openImport(SR_Base *, char *);

// Open File and Export Record
int openExport(SR_Base *, char *);

// Parse Command-Line Arguments
int argParse(const Param *, int, int, char **, ...);

// Handle Command-Line Options
int optHandle(const char *, _Bool, int, char **, ...);

#endif
