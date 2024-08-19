#!/usr/bin/python3

# Copyright (c) 2024, Jacob Bates
# SPDX-License-Identifier: BSD-2-Clause

import math
import sys
from command import *
import configs

# compute the size of a range (number of sets)
def recSize(N, minM, maxM, fixed):
    k = N - len(fixed)
    return math.comb(maxM, k) - math.comb(max(0, minM - 1), k)

# returns (N, minM, maxM, fixed), or False if no successor
# This will take in the range info from the previous record, and figure out a successor range, as
# large as possible while keeping to a size restriction.
def nextRange(N, lastM, lastFixed, limitM, maxRecSize):
    nextFixed = lastFixed
    nextMinM = lastM + 1

# if we've come to the end of the range, end here
    if nextMinM > limitM:
        return False

# if this fixed value has run its course, break it back into an M-value and proceed
    if len(lastFixed) != 0:
        if nextMinM == lastFixed[0]:
            nextFixed = nextFixed[1:]
            return nextRange(N, nextMinM, nextFixed, limitM, maxRecSize)

# if a range of one M-value would exceed the size limit, make it a fixed value instead and proceed
# like it's a smaller set
    if recSize(N, nextMinM, nextMinM, nextFixed) > maxRecSize:
        nextFixed = [nextMinM] + lastFixed
        return nextRange(N, 0, nextFixed, limitM, maxRecSize)

# enumerate max M-values until we'd exceed the size limit
    nextMaxM = nextMinM
    for i in range(nextMinM, (nextFixed + [limitM + 1])[0]):
        if recSize(N, nextMinM, i, nextFixed) <= maxRecSize:
            nextMaxM = i
        else:
            break

    return (N, nextMinM, nextMaxM, nextFixed)

# create a blank record file from given range
def createRec(N, minM, maxM, fixed, destDir, idx):
    k = N - len(fixed)
    fixedArr = [str(n) for n in fixed]
    fname = f"{destDir}/{idx:04d}_rec_{N}_{minM}-{maxM}_{','.join(fixedArr)}.dat"

    args = [f"{configs.BIN_DIR}/create", str(k), str(minM), str(maxM), str(len(fixed)),
            f"{' '.join(fixedArr)}", fname]
    print(argsToCmd(args))
    fail = subprocess.call(args)

    return None if fail else fname

# ====== CREATE DIRECTORY
# automatically generate a directory of records with M-range
def createDir(N, minM, maxM, maxRecSize, dirname):
    os.mkdir(dirname)

# generate successive ranges and create records until we get through the range we're given
    idx = 0
    fixed = []
    (recMinM, recMaxM, limitM) = (0, minM - 1, maxM)
    while True:
        res = nextRange(N, recMaxM, fixed, limitM, maxRecSize)
        if not res:
            break
        (N, recMinM, recMaxM, fixed) = res

        file = createRec(N, recMinM, recMaxM, fixed, dirname, idx)
        if not file:
            return False

        res = compress(file, 0)
        if not res:
            return False

        idx += 1

# create base log file
    outlines = [f"INST: N_{N} M_{minM}_{maxM}"]
    f = open(f"{dirname}/log", 'w')
    f.writelines([line + '\n' for line in outlines])
    f.close()

    return True

# ====== SCRIPT INVOCATION ROUTINE
if __name__ == '__main__':
    if len(sys.argv) != 6:
        print(f"Usage: {sys.argv[0]} dest N minM maxM maxSize")
        sys.exit(1)

    configs.readConfigs("config.json")
    configs.writeConfigs("config.json")

    dirname = sys.argv[1]
    N = int(sys.argv[2])
    minM = int(sys.argv[3])
    maxM = int(sys.argv[4])
    maxSize = int(sys.argv[5])

    res = createDir(N, minM, maxM, maxSize, dirname)

    sys.exit(not res)
