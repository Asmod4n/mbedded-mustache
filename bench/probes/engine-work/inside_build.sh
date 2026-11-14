#!/bin/sh
# Container side, phase build. Makes the arms, builds every binary four at a time, records .text sizes, and checks the bytes as nobody.
# Stderr goes to log files under /w/out. A binary that does not build is a line in /w/out/failed.txt.
set -u
. /w/src/bench/probes/engine-work/flags.sh
mkdir -p /w/out/check /w/bin /w/arms
: > /w/out/failed.txt
chmod a+rwx /w/out /w/out/check /w/bin
chmod a+w /w/out/failed.txt
sh /w/src/bench/probes/engine-work/make_arms.sh > /w/out/arms.log 2>&1 || echo "make_arms failed" >> /w/out/failed.txt
jobs=""
for cxx in $COMPILERS; do
  jobs="${jobs}ref main p20 $cxx\n"
  for v in $EXTRA_VARIANTS; do jobs="${jobs}extras $v - $cxx\n"; done
  for arm in $ARMS; do
    for page in $PAGES; do jobs="${jobs}stocks $arm $page $cxx\n"; done
  done
done
printf "$jobs" | xargs -P 4 -L 1 sh /w/src/bench/probes/engine-work/build_one.sh
: > /w/out/sizes.txt
for cxx in $COMPILERS; do
  for arm in $ARMS; do
    for page in $PAGES; do
      b=/w/bin/stocks.$arm.$page.$cxx
      [ -x "$b" ] || continue
      echo "$arm $page $cxx text_bytes=$(size -A "$b" | awk '$1 == ".text" {print $2}') compile_seconds=$(cat "$b.seconds")" >> /w/out/sizes.txt
    done
  done
done
chmod -R a+rX /w/bin /w/arms
runuser -u nobody -- sh /w/src/bench/probes/engine-work/check.sh
cat /w/out/check-diffs.txt
