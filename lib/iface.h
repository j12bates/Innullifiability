// ========================== UTIL INTERFACE ==========================

// Some common macros and type definitions for the utility programs.

#ifndef IFACE_H
#define IFACE_H

#include <errno.h>
#include <string.h>

#include "setRec.h"

#define NULLIF 1 << 0
#define ONLY_SUP 1 << 1
#define MARKED NULLIF | ONLY_SUP

#define FAULT() { \
            fprintf(stderr, "Fault at %s:%d -- %s\n", \
                    __FILE__, __LINE__, strerror(errno)); \
            safeExit(); \
        }

#define CK_NO(NO) { \
            int __no = (NO); \
            if (__no > 0) FAULT(); \
        }

#define CK_RES(RES) { \
            int __res = (RES); \
            if (__res < 0) FAULT(); \
        }

#define CK_PTR(PTR) { \
            void *__p = ((void *) PTR); \
            if (__p == NULL) FAULT(); \
        }

typedef enum ParamType {
    PARAM_END = 0,
    PARAM_CT,
    PARAM_SIZE,
    PARAM_VAL,
    PARAM_FNAME,
    PARAM_STR
} Param;

// Open Record File and Import
int openImport(SR_Base *, char *);

// Open File and Export Record
int openExport(SR_Base *, char *);

// Parse Command-Line Arguments
int argParse(const Param *, int, int, char **, ...);

// Handle Command-Line Options
int optHandle(const char *, _Bool, int, char **, ...);

// Safely Exit
_Noreturn void safeExit(void);

#endif
