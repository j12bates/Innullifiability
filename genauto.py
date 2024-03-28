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

# take a record filename and extract the range information
def getRange(fname):
    not_a_rec = (0, 0, 0, [])

# "directory/rec_N_minM-maxM_f1,f2.dat" -> ["rec_N_minM-maxM_f1,f2", "dat"]
    segments = fname.split('/')[-1].split('.')
    if len(segments) != 2:
        return not_a_rec
    if segments[1] != 'dat':
        return not_a_rec

# "rec_N_minM-maxM_f1,f2" -> ["rec", "N", "minM-maxM", "f1,f2"]
    attrs = segments[0].split('_')
    if len(attrs) != 4:
        return not_a_rec
    if attrs[0] != 'rec':
        return not_a_rec

    N = int(attrs[1])
    fixed = [int(n) for n in attrs[3].split(',') if n != '']

    MRange = attrs[2].split('-')
    if len(MRange) != 2:
        return not_a_rec
    minM = int(MRange[0])
    maxM = int(MRange[1])
    if minM > maxM:
        return not_a_rec

    return (N, minM, maxM, fixed)

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print("Usage: ./genauto.py destSize dest src")
        sys.exit(1)

    destSize = int(sys.argv[1])
    destdir = sys.argv[2]
    srcdir = sys.argv[3]
