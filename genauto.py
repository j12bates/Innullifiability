#!/usr/bin/python

import math
import os
import sys

def recSize(N, minM, maxM, fixed):
    k = N - len(fixed)
    return math.comb(maxM, k) - math.comb(max(0, minM - 1), k)

# returns (N, minM, maxM, fixed)
# This will take in the range info from the previous record, and figure
# out a successor range, as large as possible while keeping to a size
# restriction.
def nextRange(N, lastM, lastFixed, limitM, maxRecSize):
    nextFixed = lastFixed
    nextMinM = lastM + 1

# if this fixed value has run its course, break it back into an M-value
# and proceed
    if len(lastFixed) != 0:
        if nextMinM == lastFixed[0]:
            nextFixed = nextFixed[1:]
            return nextRange(N, nextMinM, nextFixed, limitM, maxRecSize)

# if a range of one M-value would exceed the size limit, make it a fixed
# value instead and proceed like it's a smaller set
    if recSize(N, nextMinM, nextMinM, nextFixed) > maxRecSize:
        nextFixed = [nextMinM] + lastFixed
        return nextRange(N, 0, nextFixed, limitM, maxRecSize)

# enumerate max M-values until we'd exceed the size limit
    nextMaxM = nextMinM
    for i in range(nextMinM, (nextFixed + [limitM])[0]):
        if recSize(N, nextMinM, i, nextFixed) <= maxRecSize:
            nextMaxM = i
        else:
            break

    return (N, nextMinM, nextMaxM, nextFixed)

# create a blank record file from given range
def createRec(N, minM, maxM, fixed, destDir):
    fixedArr = [str(n) for n in fixed]
    fname = f"{destDir}/rec_{N}_{minM}-{maxM}_{','.join(fixedArr)}.dat"
    cmd = f"./bin/create {N} {minM} {maxM} {len(fixed)} \"{' '.join(fixedArr)}\" {fname}"
    os.system(cmd)
    return fname

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print("Usage: ./genauto.py destSize dest src")
        sys.exit(1)

    destSize = int(sys.argv[1])
    destdir = sys.argv[2]
    srcdir = sys.argv[3]
