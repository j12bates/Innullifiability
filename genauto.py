#!/usr/bin/python

import math
import os
import sys
import threading

# compute the size of a range (number of sets)
def recSize(N, minM, maxM, fixed):
    k = N - len(fixed)
    return math.comb(maxM, k) - math.comb(max(0, minM - 1), k)

# returns (N, minM, maxM, fixed), or False if no successor
# This will take in the range info from the previous record, and figure
# out a successor range, as large as possible while keeping to a size
# restriction.
def nextRange(N, lastM, lastFixed, limitM, maxRecSize):
    nextFixed = lastFixed
    nextMinM = lastM + 1

# if we've come to the end of the range, end here
    if nextMinM > limitM:
        return False

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
    cmd = f"./bin/create {k} {minM} {maxM} {len(fixed)} \"{' '.join(fixedArr)}\" {fname}"

    fail = os.system(cmd)
    return None if fail else fname

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
    if len(attrs) != 5:
        return not_a_rec
    if attrs[1] != 'rec':
        return not_a_rec

    N = int(attrs[2])
    fixed = [int(n) for n in attrs[4].split(',') if n != '']

    MRange = attrs[3].split('-')
    if len(MRange) != 2:
        return not_a_rec
    minM = int(MRange[0])
    maxM = int(MRange[1])
    if minM > maxM:
        return not_a_rec

    return (N, minM, maxM, fixed)

# ====== CREATE DIRECTORY
# automatically generate a directory of records with M-range
def createDir(N, minM, maxM, maxRecSize, dirname):
    cmd = f"mkdir {dirname}"
    fail = os.system(cmd)
    if fail:
        return False

# generate successive ranges and create records until we get through the
# range we're given
    idx = 0
    fixed = []
    (recMinM, recMaxM, limitM) = (0, minM - 1, maxM)
    while True:
        res = nextRange(N, recMaxM, fixed, limitM, maxRecSize)
        if not res:
            break
        (N, recMinM, recMaxM, fixed) = res

        res = createRec(N, recMinM, recMaxM, fixed, dirname, idx)
        if not res:
            return False
        idx += 1

# create base log file
    outlines = [f"N_{N} M_{minM}_{maxM}"]
    f = open(f"{dirname}/log", 'w')
    f.writelines([line + '\n' for line in outlines])
    f.close()

    return True

# ====== COMMAND EXECUTION
# figures out NUMA job
def numajob(node):
    return f"numactl --cpunodebind={node} --membind={node} -- "

# returns success boolean
def expand(src, dest, threads, node):
    (srcN, _, _, _) = getRange(src)
    cmd = f"{numajob(node)} ./bin/gen {srcN} {src} {dest} {threads}"
    print(cmd)
    fail = os.system(cmd)
    return not fail

# returns success boolean
def weed(dest, minM, maxM, threads, node):
    (destN, _, _, _) = getRange(dest)
    cmd = f"{numajob(node)} ./bin/weed {destN} {dest} {minM} {maxM} {threads}"
    print(cmd)
    fail = os.system(cmd)
    return not fail

# ====== JOBS
# these jobs are called by worker threads, and they do an operation on a
# destination record. Expansion is a full directory expansion, and Weeding is
# just a normal ranged weeding. each one has the same argument format.

# expand a directory completely into a single record file
def expandJob(params, dest, threads, node):
    (srcdir) = params
    srcs = [f"{srcdir}/{rec}" for rec in os.listdir(srcdir) if rec != 'log']
    srcs.sort()
    for src in srcs:
        res = expand(src, dest, threads, node)
        if not res:
            return False

    return True

def weedJob(params, dest, threads, node):
    (minM, maxM) = params
    return weed(dest, minM, maxM, threads, node)

# ====== WORKER THREAD ROUTINE
# this function will run jobs into the next record that needs it. it'll run just
# one at a time, with however many threads specified, on whatever NUMA node it's
# assigned to. these global variables keep track of the next destination, so
# another thread can pick up work when it finishes.
nextDestIdx = 0
dests = []              # TODO: don't do this, use function to retrieve rec by idx
error = False
jobIdxLock = threading.Lock()
def massWorker(job, params, threads, node):
    global nextDestIdx
    global dests
    global error
    global jobIdxLock
    while True:
        # get the next destination to work
        with jobIdxLock:
            if nextDestIdx == len(dests) or error:
                break
            else:
                destIdx = nextDestIdx
                nextDestIdx += 1

        # execute the job on it
        res = job(params, dests[destIdx], threads, node)
        if not res:
            with jobIdxLock:
                error = True

    return True

# ====== MASS PROCESSING
# this is the main routine for the Expansion and Sweeping modes. it'll create
# worker threads based off of the number of jobs we want to run per node. these
# jobs will collectively perform either a mass expansion or mass weeding.
def massProcess(job, params, destDir, jobsPerNode, threadsPerNode, totalNodes):
    global nextDestIdx
    global dests
    global error
    global jobIdxLock
    dests = [f"{destdir}/{rec}" for rec in os.listdir(destDir) if rec != 'log']
    dests.sort()

    th = [[0] * jobsPerNode] * totalNodes
    threadsPerJob = int(threadsPerNode / jobsPerNode + 1)

# we're just creating a bunch of job threads. nothing special... then we join
# them
    for node in range(totalNodes):
        for i in range(jobsPerNode):
            th[node][i] = threading.Thread(target=massWorker,
                    args=(job, params, threadsPerJob, node))
            th[node][i].start()

    for node in range(totalNodes):
        for i in range(jobsPerNode):
            th[node][i].join()

    return not error

# ====== MASS EXPANSION
def massExpand(destdir, srcdir):
    res = massProcess(expandJob, (srcdir), destdir, 2, 6, 1)
    return res

# ====== MASS WEEDING
def massWeed(destdir, minM, maxM):
    res = massProcess(weedJob, (minM, maxM), destdir, 2, 6, 1)
    return res

# script usage message
def usage():
    print("Usage: ./genauto.py c dest N minM maxM maxRecSize")  # create a dir
    print("       ./genauto.py x dest src")                     # expand dir to dir
    print("       ./genauto.py s dest")                         # sweep dir


# ====== SCRIPT INVOCATION ROUTINE
if __name__ == '__main__':
    if len(sys.argv) < 3:
        usage()
        sys.exit(1)

    mode = sys.argv[1]
    destdir = sys.argv[2]

# Create a Directory
    if mode == 'c':
        N = int(sys.argv[3])
        minM = int(sys.argv[4])
        maxM = int(sys.argv[5])
        maxRecSize = int(sys.argv[6])
        createDir(N, minM, maxM, maxRecSize, destdir)

# Perform Mass Expansion
    elif mode == 'x':
        srcdir = sys.argv[3]
        massExpand(destdir, srcdir)

# Sweep Directory
    elif mode == 's':
        massWeed(destdir, 0, 0)

    else:
        usage()
        sys.exit(1)
