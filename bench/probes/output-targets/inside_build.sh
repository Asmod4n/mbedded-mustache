#!/bin/sh
# Container side, phase build. Builds every binary, four at a time, records .text sizes, and checks the bytes as nobody.
# Stderr goes to log files under /w/out. A binary that does not build is a line in /w/out/failed.txt.
set -u
. /w/src/bench/probes/output-targets/flags.sh
mkdir -p /w/out/check /w/bin
: > /w/out/failed.txt
chmod a+rwx /w/out /w/out/check /w/bin
chmod a+w /w/out/failed.txt
jobs=""
for cxx in $COMPILERS; do
  for arm in $ARMS; do jobs="${jobs}$arm $cxx\n"; done
done
printf "$jobs" | xargs -P 4 -L 1 sh /w/src/bench/probes/output-targets/build_one.sh
: > /w/out/sizes.txt
for cxx in $COMPILERS; do
  for arm in $ARMS; do
    b=/w/bin/stocks.$arm.$cxx
    [ -x "$b" ] || continue
    echo "$arm $cxx text_bytes=$(size -A "$b" | awk '$1 == ".text" {print $2}') compile_seconds=$(cat "$b.seconds")" >> /w/out/sizes.txt
  done
done
chmod -R a+rX /w/bin
runuser -u nobody -- sh /w/src/bench/probes/output-targets/check.sh
cat /w/out/check-diffs.txt
