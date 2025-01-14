#!/usr/bin/python3

# Copyright (c) 2024, Jacob Bates
# SPDX-License-Identifier: BSD-2-Clause

import math
import os
import subprocess
import sys
import threading

from command import *
import configs

# TODO: ensure all errors are caught, all programs interrupt nicely and such, we want good behaviour

# TODO: progress tracking on the individual record level, progress indicators (multiple levels?)

# ====== GET RECORD FILENAME BY INDEX
# returns false if index is beyond bound or invalid
def getRecFname(dirname, idx):
    if idx < 0:
        return False

    for rec in os.listdir(dirname):
        if rec.split('_')[0] == f"{idx:04d}":
            file = f"{dirname}/{rec}"
            return file

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

# ====== JOBS
# these jobs are called by worker threads, and they do an operation on a destination record.
# Expansion is a full directory expansion, and Weeding is just a normal ranged weeding. each one has
# the same argument format.

# expand a directory completely into a single record file
def expandJob(params, dest, node):
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
                     mutate and not skip_mutate, node)
        if not res:
            return False

        srcIdx += 1

    return True

# perform special supersets on a complete directory into a single record file
def splSupJob(params, dest, node):
    (srcDir) = params

    srcIdx = 0
    while True:

# obtain next source record
        src = getRecFname(srcDir, srcIdx)
        if not src:
            break

# run the appropriate command
        res = splSup(src, dest, node)
        if not res:
            return False

        srcIdx += 1

    return True

# perform a weeding of a single record
def weedJob(params, dest, node):
    (minM, maxM) = params
    return weed(dest, minM, maxM, node)

# writes one record inspection result to the working file
def inspectJob(params, dest, node):
    (outfile) = params
    shortFname = dest.split('/')[-1]

# perform an inspection
    tableM = inspectByM(dest, node)
    if tableM == None:
        return False

# write a line for total count
    count = sum(tableM.values())
    lines = [f"Rec    ---- {shortFname:<32} -- {count:>12}"]

# write a line for each M-value
    for M in tableM:
        count = tableM[M]
        lines += [f"ZpartM {M:>4} {shortFname:<32} -- {count:>12}"]

# write this to the working file (preserved on task interruption), not the output file
    workfile = f"{outfile}.working"
    f = open(workfile, 'a')
    f.writelines([line + '\n' for line in lines])
    f.close()

    return True

# these are global variables for managing worker threads with the mass routine
th = []
workerJobIdxs = []
nextJobIdx = 0
stop = False
jobIdxLock = threading.Lock()

recN = 0

# these are global variables for task
destDir = ""
tasks = []
outlines = []

# ====== WORKER THREAD ROUTINE
# this function will run jobs into the next record that needs it. it'll run just one at a time, with
# however many threads specified, on whatever NUMA node it's assigned to. these global variables
# keep track of the next destination, so another thread can pick up work when it finishes.
def massWorker(node, wkr):
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
        dest = decompress(dest, node)
        if not dest:
            return False

# execute each task on it in sequence
        res = True
        for task in tasks:
            f = task['f']
            params = task['params']
            if res:
                res = f(params, dest, node)

# re-compress if/as configured
        if res:
            res = compress(dest, node)

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
        for node in range(configs.NODES):
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
        for node in range(configs.NODES):
            workerJobIdxs[node] = [int(s) for s in inlines[node + 1].split(' ')]

# ====== MASS PROCESSING
# this is the main routine for the Expansion and Sweeping modes. it'll create worker threads based
# off of the number of jobs we want to run per node. these jobs will collectively perform either a
# mass expansion or mass weeding.
def massProcess():
    global th, workerJobIdxs, nextJobIdx, stop, jobIdxLock, destDir
    nodes, wkrsEach = configs.NODES, configs.JOBS_PER_NODE
    th = [[0 for _ in range(wkrsEach)] for _ in range(nodes)]
    workerJobIdxs = [[nodes * wkr + node for wkr in range(wkrsEach)] for node in range(nodes)]
    nextJobIdx = nodes * wkrsEach
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
    for node in range(nodes):
        for i in range(wkrsEach):
            th[node][i] = threading.Thread(target=massWorker,
                    args=(node, i))
            th[node][i].start()

    for node in range(nodes):
        for i in range(wkrsEach):
            th[node][i].join()

    with jobIdxLock:
        error = stop
    if error:
        return False

# run end routines for tasks in sequence
    for task in tasks:
        if 'f_end' in task:
            f = task['f_end']
            params = task['params']
            f(params)

# output lines to log file
    f = open(f"{destDir}/log", 'a')
    f.writelines([line + '\n' for line in outlines])
    f.close()

    os.system(f"rm {progFile}")

    return True

# Read Directory Parameters from Log
# (N, minM, maxM, swept)
def readLog(dirname):
    fname = f"{dirname}/log"
    if not os.path.isfile(fname):
        return None

    f = open(fname, 'r')
    lines = [line.strip() for line in f.readlines()]
    f.close()

# M-range
    params = lines[0].split(' ')
    N = int(params[1].split('_')[1])
    minM = int(params[2].split('_')[1])
    maxM = int(params[2].split('_')[2])

# Swept status
    swept = "SWEPT" in lines

    return (N, minM, maxM, swept)

# ====== EXPANSION TASK CONFIGURATION
def taskExpand(srcDir, supers, mutate):
    global tasks, outlines

# load source directory information
    res = readLog(srcDir)
    if not res:
        print(f"Invalid Source Directory {srcDir}")
        return False
    (N, minM, maxM, swept) = res

# special case: only supersets and source is more than one size below
    if supers and not mutate and N + 1 < recN:
        return taskSplSup(srcDir)

# general case: continue, but we better have source be one size below
    elif N + 1 != recN:
        print(f"Task Parameters Invalid [Expand from {srcDir}]")
        return False

# configure this task
    tasks.append({'f': expandJob, 'params': (srcDir, supers, mutate)})

# set up new lines to output into the destination log
    logline = f"M_{minM}_{maxM} [from {srcDir}]"
    if not swept:
        logline += " WARN: source not marked SWEPT"

    if supers:
        outlines += ["XSUP: " + logline]
    if mutate:
        outlines += ["XMUT: " + logline]

    return True

# ====== SPECIAL SUPERSETS TASK CONFIGURATION
def taskSplSup(srcDir):
    global tasks, outlines

# load source directory information
    res = readLog(srcDir)
    if not res:
        print(f"Invalid Source Directory {srcDir}")
        return False
    (N, _, _, _) = res

# configure this task
    tasks.append({'f': splSupJob, 'params': (srcDir)})

# set up a new line to output into the destination log
    outlines += ["SPSS: [from {srcDir}] WARN: not thorough"]

    return True

# ====== WEEDING TASK CONFIGURATION
def taskWeed(minM, maxM):
    global tasks, outlines

# configure this task
    tasks.append({'f': weedJob, 'params': (minM, maxM)})

# check if parameters are fine
    if minM > maxM and maxM != 0:
        print(f"Task Parameters Invalid [Weed in {minM}-{maxM}]")
        return False

# set up a new line to output into the destination log
    logline = f"WEED: M_{minM}_{maxM}"
    if maxM == 0:
        logline += " INDEF MAX"
    outlines += [logline]

    return True

# ====== DIRECTORY INSPECTION TASK CONFIGURATION
def taskInspect(fileid):
    global tasks, outlines
    outfile = f"{destDir}/insp-{fileid}.txt"

# clear the output file
    f = open(outfile, 'w')
    f.close()

# configure this task
    tasks.append({'f': inspectJob, 'params': (outfile), 'f_end': finalizeInspection})

    return True

# take the working file for inspection and translate it into a nice readable output file
def finalizeInspection(params):
    (outfile) = params
    workfile = f"{outfile}.working"

# read in data from the working file, process through it all
    f = open(workfile, 'r')
    lines = [line.strip() for line in f.readlines()]
    f.close()

    seenSignatures = []
    finalLines = []
    tableM = {}
    for line in lines:
        tokens = line.split()
        signature = ' '.join(tokens[0:3])

# we don't want to show a 'Rec' line twice or double-count sets... we could have line duplicates
# from an interruption/resumption
        if signature in seenSignatures:
            continue
        seenSignatures.append(signature)

# record counts are good to go
        if tokens[0] == "Rec":
            finalLines.append(line)

# sum together M-value counts across all records
        elif tokens[0] == "ZpartM":
            M = tokens[1]
            count = int(tokens[4])
            if not M in tableM:
                tableM[M] = 0
            tableM[M] += count

# enter a total count line for each M-value concerned
    for M in tableM:
        count = tableM[M]
        finalLines.append(f"M {M:>4} -- {count:>12}")

# dividing lines
    finalLines.append("A===== M-VALUE COUNTS ======")
    finalLines.append("P===== RECORD COUNTS =======")

# sort lines and write them to the final output file
    finalLines.sort()
    f = open(outfile, 'w')
    f.writelines([line + '\n' for line in finalLines])
    f.close()

    os.system(f"rm {workfile}")

    return True

# script usage message
def usage():
    name = sys.argv[0]
    print(f"Usage: {name} dest [task1] [task2] ...")
    print("Tasks can be configured this way:")
    print(f"EXPAND  -- x src")
    print(f"MUTATE  -- m src")
    print(f"SUPERS  -- s src (can be of lower size than predecessor)")
    print(f"WEED    -- w minM maxM")
    print(f"INSPECT -- i fileID")

    return True

# interpret and configure a task from the command line arguments
# returns number of arguments used, or 0 if invalid
def interpretTask(argIdx):
    global recN

    taskArgs = sys.argv[argIdx:]
    argsRemaining = len(taskArgs)
    mode = taskArgs[0]

    res = readLog(destDir)
    if res:
        (recN, _, _, _) = res

# Mass Expansion Modes
    if mode == 'x' and argsRemaining >= 2:
        srcDir = taskArgs[1]
        res = taskExpand(srcDir, True, True)
        return 2 * res

    elif mode == 's' and argsRemaining >= 2:
        srcDir = taskArgs[1]
        res = taskExpand(srcDir, True, False)
        return 2 * res

    elif mode == 'm' and argsRemaining >= 2:
        srcDir = taskArgs[1]
        res = taskExpand(srcDir, False, True)
        return 2 * res

# Mass Weeding
    elif mode == 'w' and argsRemaining >= 3:
        minM = int(taskArgs[1])
        maxM = int(taskArgs[2])
        res = taskWeed(minM, maxM)
        return 3 * res

# Directory Inspection
    elif mode == 'i' and argsRemaining >= 2:
        fileid = taskArgs[1]
        res = taskInspect(fileid)
        return 2 * res

# Invalid Task Character
    else:
        print(f"Invalid Task Character {mode}")
        return 0

# ====== SCRIPT INVOCATION ROUTINE
if __name__ == '__main__':
    if len(sys.argv) < 2:
        usage()
        sys.exit(1)

    destDir = sys.argv[1]
    if not os.path.isdir(destDir):
        print(f"Nonexistent Target Directory {destDir}")
        sys.exit(1)

# there's a config in the invocation directory, read it in if it exists and then write one back
# out with everything just 'cuz, then there might be special configs for the record directory
    configs.readConfigs("config.json")
    configs.writeConfigs("config.json")
    configs.readConfigs(f"{destDir}/config.json")

    nextArg = 2

# interpret task commands while we still have arguments
    while nextArg < len(sys.argv):
        adv = interpretTask(nextArg)
        if adv:
            nextArg += adv
        else:
            usage()
            sys.exit(1)

# run the Mass Processing Routine
    res = massProcess()

    sys.exit(not res)
