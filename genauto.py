#!/usr/bin/python

import math
import os
import sys

def recSize(N, minM, maxM):
    return math.comb(maxM, N) - math.comb(minM - 1, N)

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print("Usage: ./genauto.py destSize dest src")
        sys.exit(1)

    destSize = int(sys.argv[1])
    destdir = sys.argv[2]
    srcdir = sys.argv[3]
