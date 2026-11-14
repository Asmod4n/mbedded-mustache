#!/bin/sh
# Container side, phase build. Makes the arms, builds every binary four at a time, records .text sizes, and checks the bytes as nobody.
# Stderr goes to log files under /w/out. A binary that does not build is a line in /w/out/failed.txt.
set -u
. /w/src/bench/probes/fewer-calls/flags.sh
mkdir -p /w/out/check /w/bin /w/arms
: > /w/out/failed.txt
chmod a+rwx /w/out /w/out/check /w/bin
chmod a+w /w/out/failed.txt
sh /w/src/bench/probes/fewer-calls/make_arms.sh > /w/out/arms.log 2>&1 || echo "make_arms failed" >> /w/out/failed.txt
jobs=""
for cxx in $COMPILERS; do
  jobs="${jobs}head $cxx ref\n"
  for arm in $(cells_of $cxx); do
    jobs="${jobs}$arm $cxx stocks\n"
    [ "$arm" = twin ] || jobs="${jobs}$arm $cxx extras\n"
  done
done
printf "$jobs" | xargs -P 4 -L 1 sh /w/src/bench/probes/fewer-calls/build_one.sh
: > /w/out/sizes.txt
for cxx in $COMPILERS; do
  for arm in $(cells_of $cxx); do
    b=/w/bin/stocks.$arm.$cxx
    [ -x "$b" ] || continue
    echo "$arm $cxx text_bytes=$(size -A "$b" | awk '$1 == ".text" {print $2}') compile_seconds_stocks=$(cat "$b.seconds") compile_seconds_extras=$(cat /w/bin/extras.$arm.$cxx.seconds 2>> /w/out/sizes.err)" >> /w/out/sizes.txt
  done
done
chmod -R a+rX /w/bin /w/arms
runuser -u nobody -- sh /w/src/bench/probes/fewer-calls/check.sh
cat /w/out/check-diffs.txt
