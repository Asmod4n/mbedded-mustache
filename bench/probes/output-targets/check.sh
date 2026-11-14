#!/bin/sh
# Runs as nobody. Every binary writes its bytes; each file is compared byte for byte with the file of fresh on the same compiler,
# and the files of the two compilers are compared. A binary whose two forms render different bytes ends with a code that is not zero.
set -u
. /w/src/bench/probes/output-targets/flags.sh
cd /w/out
: > check.log
: > check.err
diffs=0
for cxx in $COMPILERS; do
  for arm in $ARMS; do
    /w/bin/stocks.$arm.$cxx --output_file=/w/out/check/stocks.$arm.$cxx.html 2>> check.err || { echo "stocks.$arm.$cxx failed" >> check.log; diffs=$((diffs + 1)); }
    if cmp -s check/stocks.$arm.$cxx.html check/stocks.fresh.$cxx.html; then r=same; else r=DIFFERENT; diffs=$((diffs + 1)); fi
    echo "$r $cxx $arm against fresh, $(sha256sum < check/stocks.$arm.$cxx.html | cut -c1-16) $(wc -c < check/stocks.$arm.$cxx.html) bytes" >> check.log
  done
done
if cmp -s check/stocks.fresh.g++-16.html check/stocks.fresh.clang++-23.html; then r=same; else r=DIFFERENT; diffs=$((diffs + 1)); fi
echo "$r fresh of g++-16 against fresh of clang++-23" >> check.log
echo "$diffs" > check-diffs.txt
