#!/bin/sh
# Runs as nobody. Every binary writes its renders; each file is compared byte for byte with the file of main on the same compiler
# and the same page. main p20 is compared with bench/render.cpp of 5d3c154. main of one compiler is compared with main of the
# other. The extras of every variant are compared with the extras of main. A binary of the timed source that renders reflection
# bytes unlike its run-time bytes ends with a code that is not zero.
set -u
. /w/src/bench/probes/engine-work/flags.sh
cd /w/out
: > check.log
: > check.err
diffs=0
mkdir -p check
for cxx in $COMPILERS; do
  /w/bin/ref.$cxx --output_file=/w/out/check/ref.$cxx.html 2>> check.err || { echo "ref.$cxx failed" >> check.log; diffs=$((diffs + 1)); }
  for arm in $ARMS; do
    for page in $PAGES; do
      /w/bin/stocks.$arm.$page.$cxx --output_file=/w/out/check/stocks.$arm.$page.$cxx.html 2>> check.err || { echo "stocks.$arm.$page.$cxx failed" >> check.log; diffs=$((diffs + 1)); }
    done
  done
  for v in $EXTRA_VARIANTS; do
    /w/bin/extras.$v.$cxx --output_file=/w/out/check/extras.$v.$cxx.txt 2>> check.err || { echo "extras.$v.$cxx failed" >> check.log; diffs=$((diffs + 1)); }
  done
  for arm in $ARMS; do
    for page in $PAGES; do
      if cmp -s check/stocks.$arm.$page.$cxx.html check/stocks.main.$page.$cxx.html; then r=same; else r=DIFFERENT; diffs=$((diffs + 1)); fi
      echo "$r $cxx $arm $page against main, $(sha256sum < check/stocks.$arm.$page.$cxx.html | cut -c1-16) $(wc -c < check/stocks.$arm.$page.$cxx.html) bytes" >> check.log
    done
  done
  for v in $EXTRA_VARIANTS; do
    if cmp -s check/extras.$v.$cxx.txt check/extras.main.$cxx.txt; then r=same; else r=DIFFERENT; diffs=$((diffs + 1)); fi
    echo "$r $cxx extras $v against main, $(sha256sum < check/extras.$v.$cxx.txt | cut -c1-16) $(wc -c < check/extras.$v.$cxx.txt) bytes" >> check.log
  done
  if cmp -s check/stocks.main.p20.$cxx.html check/ref.$cxx.html; then r=same; else r=DIFFERENT; diffs=$((diffs + 1)); fi
  echo "$r $cxx main p20 against bench/render.cpp of 5d3c154" >> check.log
done
for page in $PAGES; do
  if cmp -s check/stocks.main.$page.g++-16.html check/stocks.main.$page.clang++-23.html; then r=same; else r=DIFFERENT; diffs=$((diffs + 1)); fi
  echo "$r main $page of g++-16 against main $page of clang++-23" >> check.log
done
if cmp -s check/extras.main.g++-16.txt check/extras.main.clang++-23.txt; then r=same; else r=DIFFERENT; echo "note: extras of g++-16 (with reflection sections) and clang++-23 differ by design" >> check.log; fi
echo "$diffs" > check-diffs.txt
