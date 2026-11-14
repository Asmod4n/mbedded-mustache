#!/bin/sh
apt-get update > /w/res/apt.log 2>&1
apt-get install -y libbenchmark-dev >> /w/res/apt.log 2>&1 || { echo "apt failed"; tail -5 /w/res/apt.log; exit 1; }
sh /w/build.sh
for cxx in g++-16 clang++-23; do /w/b/fit-$cxx; echo "exit $?"; /w/b/fita-$cxx > /w/b/fita-$cxx.out 2>&1; echo "with assertions: exit $?"; tail -2 /w/b/fita-$cxx.out; done
sh /w/meas.sh run1 6 "g++-16 clang++-23" "-Os -O2" "1 2 50 60 61 62"
python3 /w/med.py /w/res/run1 1 > /w/res/run1/table.txt 2>&1
{ uname -r; g++-16 --version | head -1; clang++-23 --version | head -1; grep -m1 'model name' /proc/cpuinfo; nproc; id -un; } > /w/res/run1/context.txt 2>&1
