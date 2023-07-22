// ====================== COMMON UTIL DEFINITIONS ======================

// Some common macros and type definitions for the utility programs. No
// function declarations; those should be done locally in blocks.

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
