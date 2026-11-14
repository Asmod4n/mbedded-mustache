#!/bin/sh
# Container side, phase build. Makes the arms, builds every binary four at a time, and checks the bytes as nobody.
# Stderr goes to log files under /w/out.
set -u
. /w/src/bench/probes/runtime-speed/flags.sh
mkdir -p /w/out/check /w/bin /w/arms
: > /w/out/failed.txt
chmod a+rwx /w/out /w/out/check /w/bin
chmod a+w /w/out/failed.txt
sh /w/src/bench/probes/runtime-speed/make_arms.sh > /w/out/arms.log 2>&1 || echo "make_arms failed" >> /w/out/failed.txt
jobs=""
for cxx in $COMPILERS; do for arm in ref $ARMS; do jobs="$jobs$arm $cxx\n"; done; done
printf "$jobs" | xargs -P 4 -L 1 sh /w/src/bench/probes/runtime-speed/build_one.sh
chmod -R a+rX /w/bin /w/arms
runuser -u nobody -- sh /w/src/bench/probes/runtime-speed/check.sh
cat /w/out/check-diffs.txt
