import subprocess
import os
import configs
from dbcreate import recSize, recStartIdx

# COMMANDS

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

# convert argument list (for subprocess) to shell command
# basically just put quote-marks about arguments containing whitespace, and place explicit empty
# strings
def argsToCmd(args):
    newArgs = []
    for arg in args:
        newArg = arg if (' ' not in arg and arg != "") else f"\"{arg}\""
        newArgs.append(newArg)
    return ' '.join(newArgs)

# ====== COMMAND EXECUTION
# command words for NUMA job
def numajob(node):
    return ["numactl", f"--cpunodebind={node}", f"--membind={node}", "--"]

# decompress a compressed file
# return decompressed filename, or False on error
def decompress(file, node):
    segments = file.split('.')
    if segments[-1] != "xz":
        return file

    args = numajob(node) + ["xz", "-T", str(configs.THREADS_PER_JOB), "-d", file]
    print(argsToCmd(args))
    fail = subprocess.call(args)
    if fail:
        return False

    file = '.'.join(segments[:-1])
    return file

# compress a non-compressed file
# returns success boolean
def compress(file, node):
    cmpLevel = configs.COMPRESSION
    if cmpLevel == False:
        return True

    args = numajob(node) + ["xz", f"-{cmpLevel}", "-T", str(configs.THREADS_PER_JOB), file]
    print(argsToCmd(args))
    fail = subprocess.call(args)
    return not fail

# run the expansion process, Base-Up util
# returns success boolean
def expand(src, dest, supers, mutate, ideal, node):
    (srcN, _, _, _) = getRange(src)
    (destN, _, _, _) = getRange(dest)
    if not supers and not mutate:
        return True

    supMode = 'p' if supers else '-'
    mutMode = 'p' if mutate else '-'
    if not ideal and supers:
        supMode = 'n'

    args = numajob(node) + [f"{configs.BIN_DIR}/baseUp", "-", str(srcN), src, str(destN), dest,
            supMode, mutMode, str(configs.THREADS_PER_JOB)]
    print(argsToCmd(args))
    fail = subprocess.call(args)
    return not fail

# run the reduction process, Top-Down util
# returns success boolean
def reduce(dest, minM, maxM, weak, node):
    (destN, _, _, _) = getRange(dest)
    opts = "-s" if weak else "-"
    args = numajob(node) + [f"{configs.BIN_DIR}/topDown", opts, str(destN), dest,
            str(minM), str(maxM), str(configs.THREADS_PER_JOB)]
    print(argsToCmd(args))
    fail = subprocess.call(args)
    return not fail

# returns a dictionary of all valid M-values mapped to number of sets for each
def inspectByM(dest, mode, node):
    (destN, minM, maxM, fixed) = getRange(dest)
    if fixed:
        minM = maxM = fixed[-1]
    table = {}
    MRange = [M for M in range(minM, maxM + 1) if M != 0] # zeroes invalid
    filters = ' '.join([str(M) for M in MRange])

    args = numajob(node) + [f"{configs.BIN_DIR}/eval", "-s", str(destN), dest, mode,
            str(configs.THREADS_PER_JOB), filters]
    print(argsToCmd(args))
    count = subprocess.run(args, capture_output = True, text = True)
    if count.returncode:
        return False

# form this into a table of M-value vs. count
    strCounts = count.stdout.split('\n')[0].split(' ')[2:]
    for i in range(len(MRange)):
        M = MRange[i]
        table[M] = int(strCounts[i])

    return table

# returns a dictionary of buckets of lexicographic indices and their set counts
def inspectByIdx(dest, bucketSize, mode, node):
    (destN, minM, maxM, fixed) = getRange(dest)
    table = {}

# index bucket cutoff values
    startIdx = recStartIdx(destN, minM, maxM, fixed)
    endIdx = startIdx + recSize(destN, minM, maxM, fixed)
    firstBucket = max(0, (startIdx - 1)) // bucketSize + 1
    lastBucket = max(0, (endIdx - 2)) // bucketSize + 1
    cutoffs = range(firstBucket * bucketSize, lastBucket * bucketSize + 1, bucketSize)

# there's a limit on the number of buckets the utility program will read into
    offset = 0
    filterCtMax = 1024
    skipFirst = False
    while True:
        offsetTop = offset + filterCtMax
        if offsetTop > len(cutoffs):
            offsetTop = len(cutoffs)

        filters = ' '.join([str(c) for c in cutoffs[offset:offsetTop]])
        args = numajob(node) + [f"{configs.BIN_DIR}/eval", "-sl", str(destN), dest, mode,
                str(configs.THREADS_PER_JOB), filters]
        print(argsToCmd(args))
        count = subprocess.run(args, capture_output = True, text = True)
        if count.returncode:
            return False

# form this into a table of buckets
        strCounts = count.stdout.split('\n')[0].split(' ')[2:-1]
        for i in range(len(strCounts)):
            if i == 0 and skipFirst:
                continue
            table[firstBucket + offset + i] = int(strCounts[i])

# if we have more buckets to fill, do this all again with the next frame of cutoffs, but keep one
# below to absorb the already-counted sets
        if offsetTop < len(cutoffs):
            offset += filterCtMax - 1
            skipFirst = True
        else:
            break

    return table
