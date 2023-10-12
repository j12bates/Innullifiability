// ============================== EXPAND ===============================

// Copyright (c) 2023, Jacob Bates
// SPDX-License-Identifier: BSD-2-Clause

// This program is for expanding sets, the inverse operation to 'merging
// and reducing,' in order to find all the sets that could reduce
// immediately to whatever set is input, based on the given rules. A set
// is input, as well as some configuration, and the expansions are
// output through a function pointer.

#include <stdlib.h>

// Produce All Set Expansions
// Returns 0 on success, -1 on error (check errno)
int expand(const unsigned long *set, size_t size,
        unsigned long max, int mode,
        void (*out)(const unsigned long *, size_t))
{
    return 0;
}
