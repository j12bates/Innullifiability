#!/usr/bin/python3

# Copyright (c) 2024, Jacob Bates
# SPDX-License-Identifier: BSD-2-Clause

# CONSTANTS/CONFIGS

# Hardware
THREADS_PER_NODE = 6
NODES = 1

# Execution
JOBS_PER_NODE = 2
THREADS_PER_JOB = THREADS_PER_NODE // JOBS_PER_NODE + 1

# Record Directory
COMPRESSION = False
NO_SUPERS_MARK = False

# Path to Util Binaries
BIN_DIR = "./bin"

import math
import os
import sys
import threading
import json

# TODO: ensure all errors are caught, all programs interrupt nicely and such, we want good behaviour

# TODO: progress tracking on the individual record level, progress indicators (multiple levels?)

# read in configurations from a JSON file
# config files can have three segments, and any which are configured will be loaded in to update
# what already exists. thus files can be loaded in sequentially to have a default which can be
# superseded
def readConfigs(confFile):
    global THREADS_PER_NODE, NODES, COMPRESSION, NO_SUPERS_MARK, JOBS_PER_NODE, THREADS_PER_JOB

    if os.path.isfile(confFile):
        with open(confFile) as raw:
            configs = json.load(raw)
        if 'hw' in configs:
            THREADS_PER_NODE = configs['hw']['threadsPerNode']
            NODES = configs['hw']['numaNodes']
        if 'dir' in configs:
            COMPRESSION = configs['dir']['compressionLevel']
            NO_SUPERS_MARK = configs['dir']['oneBitMarking']
        if 'exec' in configs:
            JOBS_PER_NODE = configs['exec']['jobsPerNode']
            BIN_DIR = configs['exec']['utilBinDir']

    THREADS_PER_JOB = THREADS_PER_NODE // JOBS_PER_NODE + 1

    return True

# write configurations to a JSON file
def writeConfigs(confFile):
    configs = {}
    configs['hw'] = {'threadsPerNode': THREADS_PER_NODE, 'numaNodes': NODES}
    configs['dir'] = {'compressionLevel': COMPRESSION, 'oneBitMarking': NO_SUPERS_MARK}
    configs['exec'] = {'jobsPerNode': JOBS_PER_NODE, 'utilBinDir': BIN_DIR}

    f = open(confFile, 'w')
    f.write(json.dumps(configs, indent=4) + '\n')
    f.close()

    return True

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
    cmd = f"{BIN_DIR}/create {k} {minM} {maxM} {len(fixed)} \"{' '.join(fixedArr)}\" {fname}"

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

# ====== GET RECORD FILENAME BY INDEX
# returns false if index is beyond bound or invalid
def getRecFname(dirname, idx):
    if idx < 0:
        return False

    for rec in os.listdir(dirname):
        if rec.split('_')[0] == f"{idx:04d}":
            return f"{dirname}/{rec}"

    return False

# count the minimum number of values that are in a set in range A that aren't in a set in range B.
# this is for computing Unmet Required and Poking Values.
def countInANotInB(recA, recB):
    (A_N, A_minM, A_maxM, A_fixed) = getRange(recA)
    (B_N, B_minM, B_maxM, B_fixed) = getRange(recB)
    B_valid = set(list(range(1, B_maxM + 1)) + B_fixed)

# all fixed values are in each set in A, count the ones that don't appear in B
    count = len([0 for n in A_fixed if n not in B_valid])

# one value in the M-range is definitely in sets in A, meaning if none are valid in B, we have one
# extra
    if set(range(A_minM, A_maxM + 1)).isdisjoint(B_valid):
        count += 1

    return count

# ====== COMMAND EXECUTION
# figures out NUMA job
def numajob(node):
    return f"numactl --cpunodebind={node} --membind={node} -- "

# decompress a compressed file
# return decompressed filename, or False on error
def decompress(file, threads, node):
    segments = file.split('.')
    if segments[-1] != "xz":
        return file

    cmd = f"{numajob(node)} xz -T {threads} -d {file}"
    print(cmd)
    fail = os.system(cmd)
    if fail:
        return False

    file = '.'.join(segments[:-1])
    return file

# compress a non-compressed file
# returns success boolean
def compress(file, threads, node):
    if COMPRESSION == False:
        return True

    cmd = f"{numajob(node)} xz -{COMPRESSION} -T {threads} {file}"
    print(cmd)
    fail = os.system(cmd)
    return not fail

# run the expansion process, Generation util
# returns success boolean
def expand(src, dest, supers, mutate, threads, node):
    (srcN, _, _, _) = getRange(src)
    if not supers and not mutate:
        return True

    opts = ('b' if NO_SUPERS_MARK else '') + ('s' if supers else '') + ('m' if mutate else '')
    cmd = f"{numajob(node)} {BIN_DIR}/gen -{opts} {srcN} {src} {dest} {threads}"
    print(cmd)
    fail = os.system(cmd)
    return not fail

# run the weeding process, Weed util
# returns success boolean
def weed(dest, minM, maxM, threads, node):
    (destN, _, _, _) = getRange(dest)
    cmd = f"{numajob(node)} {BIN_DIR}/weed {destN} {dest} {minM} {maxM} {threads}"
    print(cmd)
    fail = os.system(cmd)
    return not fail

# inspect a singular record, Evaluate util
# returns number of sets, -1 on error
def inspect(dest, node):
    (destN, _, _, _) = getRange(dest)
    cmd = f"{numajob(node)} {BIN_DIR}/eval -s {destN} {dest}"
    print(cmd)
    out = os.popen(cmd).readlines()
    try:
        count = int(out[0].split(' ')[0])
    except e:
        return -1
    else:
        return count

# ====== JOBS
# these jobs are called by worker threads, and they do an operation on a destination record.
# Expansion is a full directory expansion, and Weeding is just a normal ranged weeding. each one has
# the same argument format.

# expand a directory completely into a single record file
def expandJob(params, dest, threads, node):
    (srcDir, supers, mutate) = params

    srcIdx = 0
    while True:
        skip_supers = []
        skip_mutate = []

# obtain next source record
        src = getRecFname(srcDir, srcIdx)
        if not src:
            break

# Unmet Required Values: values that must be in destination sets and do not appear in source sets;
# Poking Values: values that always appear in source sets and cannot be in destination sets;
        unmetReqd = countInANotInB(dest, src)
        poking = countInANotInB(src, dest)

# supersets can't remove any poking values, and can only fill in one unmet required value
        if unmetReqd > 1:
            skip_supers += ["UNMET"]
        if poking > 0:
            skip_supers += ["POKING"]

# mutations take a value away, so can address one poking value, and can fill in up to two unmet
# required values
        if unmetReqd > 2:
            skip_mutate += ["UNMET"]
        if poking > 1:
            skip_mutate += ["POKING"]

# address any skipping
        if supers and skip_supers:
            print(f"# SKIPPING SUPERS {skip_supers}: {src} into {dest}")
        if mutate and skip_mutate:
            print(f"# SKIPPING MUTATE {skip_supers}: {src} into {dest}")

# run the appropriate command
        res = expand(src, dest, supers and not skip_supers,
                     mutate and not skip_mutate, threads, node)
        if not res:
            return False

        srcIdx += 1

    return True

# perform a weeding of a single record
def weedJob(params, dest, threads, node):
    (minM, maxM) = params
    return weed(dest, minM, maxM, threads, node)

# writes one record inspection result to output file
def inspectJob(params, dest, threads, node):
    (outfile) = params

    count = inspect(dest, node)
    if count == -1:
        return False

    shortFname = dest.split('/')[-1]
    line = f"{shortFname:<32} -- {count:>12}\n"

    f = open(outfile, 'a')
    f.writelines([line])
    f.close()

    return True

# these are global variables for managing worker threads with the mass routine
th = []
workerJobIdxs = []
nextJobIdx = 0
destDir = ""
stop = False
jobIdxLock = threading.Lock()

# ====== WORKER THREAD ROUTINE
# this function will run jobs into the next record that needs it. it'll run just one at a time, with
# however many threads specified, on whatever NUMA node it's assigned to. these global variables
# keep track of the next destination, so another thread can pick up work when it finishes.
def massWorker(job, params, node, wkr):
    global workerJobIdxs, nextJobIdx, stop, jobIdxLock

    dest = ""
    while True:
# get the job to work (destination record)
        with jobIdxLock:
            dest = getRecFname(destDir, workerJobIdxs[node][wkr])
            if not dest:
                workerJobIdxs[node][wkr] = -1
                break
            elif stop:
                break

# decompress if necessary
        dest = decompress(dest, THREADS_PER_JOB, node)
        if not dest:
            return False

# execute the job on it
        res = job(params, dest, THREADS_PER_JOB, node)

# re-compress if/as configured
        if res:
            res = compress(dest, THREADS_PER_JOB, node)

# set up for the next job, mark this as done (we might have to break), save progress
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
    global workerJobIdxs, nextJobIdx

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
# this is the main routine for the Expansion and Sweeping modes. it'll create worker threads based
# off of the number of jobs we want to run per node. these jobs will collectively perform either a
# mass expansion or mass weeding.
def massProcess(job, params):
    global th, workerJobIdxs, nextJobIdx, stop, jobIdxLock, destDir
    th = [[0 for _ in range(JOBS_PER_NODE)] for _ in range(NODES)]
    workerJobIdxs = [[NODES * wkr + node for wkr in range(JOBS_PER_NODE)] for node in range(NODES)]
    nextJobIdx = NODES * JOBS_PER_NODE
    stop = False

# if a progress file exists, give option to load it and resume
    progFile = f"{destDir}/prog"
    if os.path.isfile(progFile):
        print("There exists a progress file from an interrupted invocation. Before")
        print("resuming, please ensure the current invocation is the same command as")
        print("the previously executed one (which produced the progress file):")
        os.system(f"cat {destDir}/cmdLast")
        resp = input("Resume previous invocation? [y/n/C] ")
        if resp in ['Y', 'y']:
            massProgLoad()
        elif resp in ['N', 'n']:
            pass
        else:
            print("Cancelling")
            return False

    os.system(f"echo {' '.join(sys.argv)} > {destDir}/cmdLast")
    massProgDump()

# we're just creating a bunch of job threads. nothing special... then we join them
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
        outlines += ["XSUP: " + logline]
    if mutate:
        outlines += ["XMUT: " + logline]

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

    f = open(f"{destDir}/log", 'a')
    f.writelines([line + '\n' for line in outlines])
    f.close()

    return True

# ====== DIRECTORY INSPECTION
def massInspect(outfile):
# clear file
    f = open(outfile, 'w')
    f.close()

    res = massProcess(inspectJob, (outfile))
    if not res:
        return False

# read in and sort lines (to order records by index), then overwrite
    f = open(outfile, 'r')
    lines = f.readlines()
    f.close()
    lines.sort()

    f = open(outfile, 'w')
    f.writelines([line for line in lines])
    f.close()

    return True

# script usage message
def usage():
    name = sys.argv[0]
    print("Usage:")
    print(f"CREATE  -- {name} c dest N minM maxM maxRecSize")   # create a dir
    print(f"EXPAND  -- {name} x dest src")                      # expand dir to dir
    print(f"SUPERS  -- {name} s dest src")                      # expand dir to dir (only supersets)
    print(f"MUTATE  -- {name} m dest src")                      # expand dir to dir (only mutations)
    print(f"WEED    -- {name} w dest minM maxM")                # weed dir
    print(f"INSPECT -- {name} i dest outfile")                  # inspect dir


# ====== SCRIPT INVOCATION ROUTINE
if __name__ == '__main__':
    if len(sys.argv) < 3:
        usage()
        sys.exit(1)

    mode = sys.argv[1]
    destDir = sys.argv[2]

# there's a config in the invocation directory, read it in if it exists and then write one back
# out with everything just 'cuz, then there might be special configs for the record directory
    readConfigs("config.json")
    writeConfigs("config.json")
    readConfigs(f"{destDir}/config.json")

# Create a Directory
    if mode == 'c':
        N = int(sys.argv[3])
        minM = int(sys.argv[4])
        maxM = int(sys.argv[5])
        maxRecSize = int(sys.argv[6])
        createDir(N, minM, maxM, maxRecSize, destDir)

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

# Mass Weeding
    elif mode == 'w':
        minM = int(sys.argv[3])
        maxM = int(sys.argv[4])
        massWeed(minM, maxM)

# Directory Inspection
    elif mode == 'i':
        outfile = sys.argv[3]
        massInspect(outfile)

    else:
        usage()
        sys.exit(1)

    sys.exit(0)
