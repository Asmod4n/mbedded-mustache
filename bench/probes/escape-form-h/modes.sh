#!/bin/sh
# usage: modes.sh <binary> <count> [prefix command]; prints the cpu time in ns of arm/1024/5 for each process start
b=$1; n=$2; shift 2
for i in $(seq 1 $n); do
  "$@" $b --benchmark_filter=arm/1024/5 --benchmark_min_time=0.05s --benchmark_format=csv 2>>/w/res/modes.err | awk -F, '/^"arm/{printf "%d ", $4}'
done; echo
