#!/usr/bin/python

THREADS_PER_NODE = 6
NODES = 1

JOBS_PER_NODE = 2
THREADS_PER_JOB = THREADS_PER_NODE // JOBS_PER_NODE + 1

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

# decompress a compressed file
# returns 0 on fail, 1 on success, 2 if non-compressed, as well as new filename
def decompress(file, threads, node):
    segments = file.split('.')
    if segments[-1] != "xz":
        return (2, file)

    cmd = f"{numajob(node)} xz -T {threads} -d {file}"
    print(cmd)
    fail = os.system(cmd)

    num = 0 if fail else 1
    file = '.'.join(segments[:-1])
    return (num, file)

# compress a non-compressed file
# returns success boolean
def compress(file, threads, node):
    cmd = f"{numajob(node)} xz -T {threads} {file}"
    print(cmd)
    fail = os.system(cmd)
    return not fail

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
def createDir(N, minM, maxM, maxRecSize, do_compress, dirname):
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

        file = createRec(N, recMinM, recMaxM, fixed, dirname, idx)
        if not file:
            return False

        if do_compress:
            res = compress(file, THREADS_PER_NODE, 0)
            if not res:
                return False

        idx += 1

# create base log file
    outlines = [f"INST: N_{N} M_{minM}_{maxM}"]
    f = open(f"{dirname}/log", 'w')
    f.writelines([line + '\n' for line in outlines])
    f.close()

    return True

# count the minimum number of values that are in a set in range A that aren't in
# a set in range B. this is for computing Unmet Required and Poking Values.
def countInANotInB(recA, recB):
    (A_N, A_minM, A_maxM, A_fixed) = getRange(recA)
    (B_N, B_minM, B_maxM, B_fixed) = getRange(recB)
    B_valid = set(list(range(1, B_maxM + 1)) + B_fixed)

# all fixed values are in each set in A, count the ones that don't appear in B
    count = len([0 for n in A_fixed if n not in B_valid])

# one value in the M-range is definitely in sets in A, meaning if none are valid
# in B, we have one extra
    if set(range(A_minM, A_maxM + 1)).isdisjoint(B_valid):
        count += 1

    return count

# ====== COMMAND EXECUTION
# TODO: make a constant variable for binary path
# figures out NUMA job
def numajob(node):
    return f"numactl --cpunodebind={node} --membind={node} -- "

# returns success boolean
def supers(src, dest, threads, node):
    (srcN, _, _, _) = getRange(src)
    cmd = f"{numajob(node)} ./bin/gen -s {srcN} {src} {dest} {threads}"
    print(cmd)
    fail = os.system(cmd)
    return not fail

# returns success boolean
def mutate(src, dest, threads, node):
    (srcN, _, _, _) = getRange(src)
    cmd = f"{numajob(node)} ./bin/gen -m {srcN} {src} {dest} {threads}"
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
    (srcDir, do_supers, do_mutate) = params

    srcs = [f"{srcDir}/{rec}" for rec in os.listdir(srcDir) if rec != 'log']
    srcs.sort()
    for src in srcs:
        skip_supers = False
        skip_mutate = False

# Unmet Required Values: values that must be in destination sets and do not appear in source sets;
# if there is only one, supersets can insert it, but if there are two, impossible
# if there are two, a mutation can insert them, but if three, impossible
        unmetReqd = countInANotInB(dest, src)
        if unmetReqd > 1 and do_supers:
            print(f"# SKIPPING SUPERS [UNMET REQD]: {src} into {dest}")
            skip_supers = True
        if unmetReqd > 2 and do_mutate:
            print(f"# SKIPPING MUTATE [UNMET REQD]: {src} into {dest}")
            skip_mutate = True

# Poking Values: values that always appear in source sets and cannot be in destination sets;
# if there are any of these, supersets can't get rid of them
# if there is one, a mutation can replace it, but if two, impossible
        poking = countInANotInB(src, dest)
        if poking > 0 and do_supers:
            print(f"# SKIPPING SUPERS [POKING]: {src} into {dest}")
            skip_supers = True
        if poking > 1 and do_mutate:
            print(f"# SKIPPING MUTATE [POKING]: {src} into {dest}")
            skip_mutate = True

        if do_supers and not skip_supers:
            res = supers(src, dest, threads, node)
            if not res:
                return False
        if do_mutate and not skip_mutate:
            res = mutate(src, dest, threads, node)
            if not res:
                return False

    return True

def weedJob(params, dest, threads, node):
    (minM, maxM) = params
    return weed(dest, minM, maxM, threads, node)

# these are global variables for managing worker threads with the mass routine
th = [[None] * JOBS_PER_NODE] * NODES
workerJobIdxs = [[NODES * wkr + node for wkr in range(JOBS_PER_NODE)] for node in range(NODES)]
nextJobIdx = NODES * JOBS_PER_NODE
destDir = ""
stop = False
jobIdxLock = threading.Lock()

# get a destination record filename by index
# returns false if index is beyond bound
def getDest(idx):
    global destDir

    if idx == -1:
        return False

    for rec in os.listdir(destDir):
        if rec.split('_')[0] == f"{idx:04d}":
            return f"{destDir}/{rec}"

    return False

# ====== WORKER THREAD ROUTINE
# this function will run jobs into the next record that needs it. it'll run just
# one at a time, with however many threads specified, on whatever NUMA node it's
# assigned to. these global variables keep track of the next destination, so
# another thread can pick up work when it finishes.
def massWorker(job, params, node, wkr):
    global workerJobIdxs, nextJobIdx, stop, jobIdxLock

    dest = ""
    while True:
# get the job to work (destination record)
        with jobIdxLock:
            dest = getDest(workerJobIdxs[node][wkr])
            if not dest:
                workerJobIdxs[node][wkr] = -1
                break
            elif stop:
                break

# decompress if necessary
        (res, dest) = decompress(dest, THREADS_PER_JOB, node)
        if res == 0:
            return False
        recompress = res != 2

# execute the job on it
        res = job(params, dest, THREADS_PER_JOB, node)

# re-compress if applicable
        if recompress and res:
            res = compress(dest, THREADS_PER_JOB, node)

# set up for the next job, mark this as done (we might have to break), save
# progress
        with jobIdxLock:
            if not res:
                stop = True
            else:
                workerJobIdxs[node][wkr] = nextJobIdx
                nextJobIdx += 1
        massProgDump()
    massProgDump()

    return True

# ====== SAVE TO PROGRESS FILE
def massProgDump():
    global workerJobIdxs, nextJobIdx, jobIdxLock, destDir

# grab all the current job indices of each worker thread, the next job counter
    outlines = []
    with jobIdxLock:
        outlines = [str(nextJobIdx)]
        for node in range(NODES):
            outlines += [' '.join([str(idx) for idx in workerJobIdxs[node]])]

# dump to progress file
    f = open(f"{destDir}/prog", 'w')
    f.writelines([line + '\n' for line in outlines])
    f.close()

# ====== LOAD FROM PROGRESS FILE
def massProgLoad():
    global workerJobIdxs, nextJobIdx, jobIdxLock, destDir

# read progress file lines
    f = open(f"{destDir}/prog", 'r')
    inlines = [line.strip() for line in f.readlines()]
    f.close()

# enter data into global variables
    with jobIdxLock:
        nextJobIdx = int(inlines[0])
        for node in range(NODES):
            workerJobIdxs[node] = [int(s) for s in inlines[node + 1].split(' ')]

# ====== MASS PROCESSING
# this is the main routine for the Expansion and Sweeping modes. it'll create
# worker threads based off of the number of jobs we want to run per node. these
# jobs will collectively perform either a mass expansion or mass weeding.
def massProcess(job, params):
    global th, nextJobIdx, stop, jobIdxLock, destDir
    stop = False

# if a progress file exists, give option to load it and resume
# TODO: log commands while executing in another file, verify command is the same
    progFile = f"{destDir}/prog"
    if os.path.isfile(progFile):
        resp = input("Resume previous invocation? [Y/n] ")
        if resp in ['Y', 'y', '']:
            massProgLoad()
        elif resp in ['N', 'n']:
            pass
        else:
            print("Cancelling")
            return False
    massProgDump()

# we're just creating a bunch of job threads. nothing special... then we join
# them
    for node in range(NODES):
        for i in range(JOBS_PER_NODE):
            th[node][i] = threading.Thread(target=massWorker,
                    args=(job, params, node, i))
            th[node][i].start()

    for node in range(NODES):
        for i in range(JOBS_PER_NODE):
            th[node][i].join()

    with jobIdxLock:
        error = stop

    if not error:
        os.system(f"rm {progFile}")

    return not error

# ====== MASS EXPANSION
def massExpand(srcDir, supers, mutate):
    res = massProcess(expandJob, (srcDir, supers, mutate))
    if not res:
        return False

# read log from source
    f = open(f"{srcDir}/log", 'r')
    inlines = [line.strip() for line in f.readlines()]
    f.close()

# search for the (manually added) swept marker in source
    swept = "SWEPT" in inlines

# get source N, M-range
    params = inlines[0].split(' ')
    N = params[1].split('_')[1]
    minM = params[2].split('_')[1]
    maxM = params[2].split('_')[2]

# output a new line into the destination log
    logline = f"M_{minM}_{maxM} [from {srcDir}]"
    if not swept:
        logline += " WARN: source not marked SWEPT"

    outlines = []
    if supers:
        outlines += ["_SUP: " + logline]
    if mutate:
        outlines += ["_MUT: " + logline]

    global destDir
    f = open(f"{destDir}/log", 'a')
    f.writelines([line + '\n' for line in outlines])
    f.close()

    return True

# ====== MASS WEEDING
def massWeed(minM, maxM):
    res = massProcess(weedJob, (minM, maxM))
    if not res:
        return False

# output a new line into the destination log
    outlines = [f"WEED: M_{minM}_{maxM}"]
    if maxM == 0:
        outlines[0] += " INDEF MAX"

    global destDir
    f = open(f"{destDir}/log", 'a')
    f.writelines([line + '\n' for line in outlines])
    f.close()

    return True

# script usage message
def usage():
    print("Usage:")
    print("CREATE -- ./genauto.py c dest N minM maxM maxRecSize [compress]")    # create a dir
    print("EXPAND -- ./genauto.py x dest src")                     # expand dir to dir
    print("SUPERS -- ./genauto.py s dest src")                     # expand dir to dir (only supersets)
    print("MUTATE -- ./genauto.py m dest src")                     # expand dir to dir (only mutations)
    print("WEED   -- ./genauto.py w dest minM maxM")               # weed dir


# ====== SCRIPT INVOCATION ROUTINE
if __name__ == '__main__':
    if len(sys.argv) < 3:
        usage()
        sys.exit(1)

    mode = sys.argv[1]
    destDir = sys.argv[2]

# Create a Directory
    if mode == 'c':
        N = int(sys.argv[3])
        minM = int(sys.argv[4])
        maxM = int(sys.argv[5])
        maxRecSize = int(sys.argv[6])
        do_compress = len(sys.argv) > 7
        createDir(N, minM, maxM, maxRecSize, do_compress, destDir)

# Mass Expansion Modes
    elif mode == 'x':
        srcDir = sys.argv[3]
        massExpand(srcDir, True, True)

    elif mode == 's':
        srcDir = sys.argv[3]
        massExpand(srcDir, True, False)

    elif mode == 'm':
        srcDir = sys.argv[3]
        massExpand(srcDir, False, True)

# TODO: inspect mode, for counting number of yet-unmarked sets

# Mass Weeding
    elif mode == 'w':
        minM = int(sys.argv[3])
        maxM = int(sys.argv[4])
        massWeed(minM, maxM)

    else:
        usage()
        sys.exit(1)
