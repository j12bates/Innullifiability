import json
import os

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

# Inspection
IDX_BUCKET_SIZE = 5000
IDX_BUCKET_COUNT = 20

# Path to Util Binaries
BIN_DIR = "./bin"

# read in configurations from a JSON file
# config files can have three segments, and any which are configured will be loaded in to update
# what already exists. thus files can be loaded in sequentially to have a default which can be
# superseded
def readConfigs(confFile):
    global THREADS_PER_NODE, NODES
    global COMPRESSION, NO_SUPERS_MARK
    global JOBS_PER_NODE, BIN_DIR
    global IDX_BUCKET_SIZE, IDX_BUCKET_COUNT
    global THREADS_PER_JOB

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
        if 'insp' in configs:
            IDX_BUCKET_SIZE = configs['insp']['idxBucketSize']
            IDX_BUCKET_COUNT = configs['insp']['idxBucketCount']

    THREADS_PER_JOB = THREADS_PER_NODE // JOBS_PER_NODE + 1

    return True

# write configurations to a JSON file
def writeConfigs(confFile):
    configs = {}
    configs['hw'] = {'threadsPerNode': THREADS_PER_NODE, 'numaNodes': NODES}
    configs['dir'] = {'compressionLevel': COMPRESSION, 'oneBitMarking': NO_SUPERS_MARK}
    configs['exec'] = {'jobsPerNode': JOBS_PER_NODE, 'utilBinDir': BIN_DIR}
    configs['insp'] = {'idxBucketSize': IDX_BUCKET_SIZE, 'idxBucketCount': IDX_BUCKET_COUNT}

    f = open(confFile, 'w')
    f.write(json.dumps(configs, indent=4) + '\n')
    f.close()

    return True
