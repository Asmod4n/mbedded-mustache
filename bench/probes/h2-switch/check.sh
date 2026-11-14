#!/bin/sh
# Runs as nobody. Every binary writes its renders; each file is compared byte for byte with the file of head on the same
# compiler. head is compared with bench/render.cpp of 3e49a1c. head of one compiler is compared with head of the other.
# A binary of the timed source that renders reflection bytes unlike its run-time bytes ends with a code that is not zero.
set -u
. /w/src/bench/probes/h2-switch/flags.sh
cd /w/out
: > check.log
: > check.err
diffs=0
mkdir -p check
for cxx in $COMPILERS; do
  /w/bin/ref.head.$cxx --output_file=/w/out/check/ref.$cxx.html 2>> check.err || { echo "ref.head.$cxx failed" >> check.log; diffs=$((diffs + 1)); }
  for arm in $(cells_of $cxx); do
    /w/bin/stocks.$arm.$cxx --output_file=/w/out/check/stocks.$arm.$cxx.html 2>> check.err || { echo "stocks.$arm.$cxx failed" >> check.log; diffs=$((diffs + 1)); }
    [ "$arm" = twin ] || /w/bin/extras.$arm.$cxx --output_file=/w/out/check/extras.$arm.$cxx.txt 2>> check.err || { echo "extras.$arm.$cxx failed" >> check.log; diffs=$((diffs + 1)); }
  done
  for arm in $(cells_of $cxx); do
    if cmp -s check/stocks.$arm.$cxx.html check/stocks.head.$cxx.html; then r=same; else r=DIFFERENT; diffs=$((diffs + 1)); fi
    echo "$r $cxx $arm stocks against head, $(sha256sum < check/stocks.$arm.$cxx.html | cut -c1-16) $(wc -c < check/stocks.$arm.$cxx.html) bytes" >> check.log
    [ "$arm" = twin ] && continue
    if cmp -s check/extras.$arm.$cxx.txt check/extras.head.$cxx.txt; then r=same; else r=DIFFERENT; diffs=$((diffs + 1)); fi
    echo "$r $cxx $arm extras against head, $(sha256sum < check/extras.$arm.$cxx.txt | cut -c1-16) $(wc -c < check/extras.$arm.$cxx.txt) bytes" >> check.log
  done
  if cmp -s check/stocks.head.$cxx.html check/ref.$cxx.html; then r=same; else r=DIFFERENT; diffs=$((diffs + 1)); fi
  echo "$r $cxx head stocks against bench/render.cpp of 3e49a1c" >> check.log
done
if cmp -s check/stocks.head.g++-16.html check/stocks.head.clang++-23.html; then r=same; else r=DIFFERENT; diffs=$((diffs + 1)); fi
echo "$r head stocks of g++-16 against head stocks of clang++-23" >> check.log
echo "$diffs" > check-diffs.txt
