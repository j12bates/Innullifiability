import subprocess
import os
import configs

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

# TODO: get rid of the unused threads parameter
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

# run the expansion process, Generation util
# returns success boolean
def expand(src, dest, supers, mutate, node):
    (srcN, _, _, _) = getRange(src)
    if not supers and not mutate:
        return True

    opts = '-' + ('b' if configs.NO_SUPERS_MARK else '') + ('s' if supers else '') + ('m' if mutate else '')
    args = numajob(node) + [f"{configs.BIN_DIR}/gen", opts, str(srcN), src, dest,
            str(configs.THREADS_PER_JOB)]
    print(argsToCmd(args))
    fail = subprocess.call(args)
    return not fail

# run the special supersets process, General Multiple Supersets util
# returns success boolean
def splSup(src, dest, node):
    (srcN, _, _, _) = getRange(src)
    (destN, _, _, _) = getRange(dest)
    args = numajob(node) + [f"{configs.BIN_DIR}/msup", str(srcN), src, str(destN), dest,
            str(configs.THREADS_PER_JOB)]
    print(argsToCmd(args))
    fail = subprocess.call(args)
    return not fail

# run the weeding process, Weed util
# returns success boolean
def weed(dest, minM, maxM, node):
    (destN, _, _, _) = getRange(dest)
    args = numajob(node) + [f"{configs.BIN_DIR}/weed", str(destN), dest, str(minM), str(maxM),
            str(configs.THREADS_PER_JOB)]
    print(argsToCmd(args))
    fail = subprocess.call(args)
    return not fail

# returns a dictionary of all valid M-values mapped to number of sets for each
def inspectByM(dest, node):
    (destN, minM, maxM, fixed) = getRange(dest)
    if fixed:
        minM = maxM = fixed[-1]
    table = {}
    MRange = [M for M in range(minM, maxM + 1) if M != 0] # zeroes invalid
    filters = ' '.join([str(M) for M in MRange])

    args = numajob(node) + [f"{configs.BIN_DIR}/eval", "-s", str(destN), dest, "1", filters]
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
