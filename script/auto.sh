#!/bin/sh

# ================= AUTOMATIC INNULLIFIABLE SET SCRIPT =================

# Copyright (c) 2023, Jacob Bates
# SPDX-License-Identifier: BSD-2-Clause

# All record names will be stored in an array
declare -a tempf

# Make a named pipe for progress updates
progf=$(mktemp -u /tmp/prog.XXXXXX)
mkfifo $progf

tsize=$1
tmaxm=$2
th=$3
output=$4

utilpath=./bin
classic=0

usage="Usage: $0 target-size target-maxval [threads [output]]"
usage1="All but <output> are positive integers"

# Read in binary numbers from the named pipe, output them in progress
# form
progRead () {
    decnums=$(od -A n -t d8 -N 16 $1)
    read -r current total rest << END
$decnums
END
    [ -z $current ] || [ -z $total ] || \
    echo "$current / $total ($((current * 100 / total))%)"
}

# Send the signal to the program and print the progress data each second
progLoop () {
while true
do
    sleep 0.2
    printf "%s\r" "$(progRead $2)" >&2 &
    kill -s USR1 $1 || return
done
}

# Clean up backgrounded processes and files on exit
cleanup () {
    kill -s TERM $(jobs -p) 2> /dev/null
    rm -f $progf ${tempf[*]}
}
trap cleanup EXIT HUP INT TERM

# Validate command-line arguments -- after this we know they're valid
# numbers, no need for quotes
num='^[0-9]+$'
invalid=0
echo $tsize | grep -Pq $num || invalid=1
echo $tmaxm | grep -Pq $num || invalid=1
[ -z "$th" ] || echo $th | grep -Pq $num || invalid=1
[ -z "$th" ] && th=1
if [ $invalid -ne 0 ]
then
    echo "$usage" >&2
    echo "$usage1" >&2
    exit 1
fi

echo "N = $tsize, M <= $tmaxm" >&2

# Perform one 'generation' from all results from smaller sizes into a
# new size.
newGeneration () {
    destSize=$1

    echo >&2
    echo "================ Generating Size $size" >&2

# Store working records in a shared memory tempfile
    dest=$(mktemp /dev/shm/rec.XXXXXX.$destSize)
    tempf[destSize]=$dest
    $utilpath/create $size 0 $tmaxm 0 "" $dest || exit 1

# Whenever we run a work job, we'll background it, keep its PID, then
# launch a loop for progress updates and background that as well. We'll
# wait for the work program to end, then kill the loop.

# Do the Expansive work, marking off all supersets of precarious sets
# and mutations of precarious sets in range. In classic, just expand the
# previous record
    srcSize=3
    if [ $classic -ne 0 ] && [ $destSize -gt 3 ]
    then
        srcSize=$((destSize - 1))
    fi
    while [ $srcSize -lt $destSize ]
    do
        echo "Expanding Size $srcSize                " >&2

        $utilpath/baseUp $srcSize ${tempf[srcSize]} $destSize $dest \
            p p $th $progf & curwork=$!
        progLoop $curwork $progf & curloop=$!
        wait $curwork || exit 1
        kill $curloop

        srcSize=$((srcSize + 1))
    done

# Now do the Reductive work, testing what remains, assuming no supersets
# were missed or in-range mutations. In classic, only do this for the
# last generation and do a blanket test
    if [ $classic -eq 0 ] || [ $destSize -eq $tsize ]
    then
        echo "Testing Remaining Sets                " >&2

        if [ $classic -eq 0 ]
        then
            $utilpath/topDown $destSize $dest 0 $tmaxm $th $progf \
                & curwork=$!
        else
            $utilpath/topDown -s $destSize $dest 0 0 $th $progf \
                & curwork=$!
        fi
        progLoop $curwork $progf & curloop=$!
        wait $curwork || exit 1
        kill $curloop
    fi
}

# Start from size 3 and keep generating set records of higher sizes
size=3
while [ $size -le $tsize ]
do
    newGeneration $size || exit 1
    size=$((size + 1))
done

# Print out the resulting innullifiable sets
echo >&2
echo "================ Result" >&2
tempout=${tempf[tsize]}
$utilpath/eval $tsize $tempout i || exit 1

# Copy output
if [ -n "$output" ]
then
    mv $tempout "$output" || exit 1
fi

exit 0
