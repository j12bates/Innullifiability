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

# if this fixed value has run its course, treat it like our last M-value
    if len(lastFixed) != 0:
        if nextMinM == lastFixed[0]:
            nextFixed = nextFixed[1:]
            return nextRange(N, nextMinM, nextFixed, limitM, maxRecSize)

# go into a new fixed value if there's not enough space for a whole new
# M-value
    if recSize(N, nextMinM, nextMinM, nextFixed) > maxRecSize:
        nextFixed = [nextMinM] + lastFixed
        nextMinM = 0

# enumerate max M-values until we'd exceed the size limit
    nextMaxM = nextMinM
    for i in range(nextMinM, (nextFixed + [limitM])[0]):
        if recSize(N, nextMinM, i, nextFixed) <= maxRecSize:
            nextMaxM = i
        else:
            break

    return (N, nextMinM, nextMaxM, nextFixed)

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print("Usage: ./genauto.py destSize dest src")
        sys.exit(1)

    destSize = int(sys.argv[1])
    destdir = sys.argv[2]
    srcdir = sys.argv[3]
