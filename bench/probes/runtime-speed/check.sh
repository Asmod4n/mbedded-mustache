#!/bin/sh
# Runs as nobody. Every binary writes its renders; each file is compared byte for byte with the file of H on the same form
# and the same compiler. H is compared with bench/render.cpp of 3e49a1c. H of one compiler is compared with H of the other.
set -u
. /w/src/bench/probes/runtime-speed/flags.sh
cd /w/out
: > check.log
diffs=0
for cxx in $COMPILERS; do
  /w/bin/ref.$cxx --output_file=/w/out/check/ref.$cxx.html 2>> check.err || { echo "ref.$cxx failed" >> check.log; diffs=$((diffs + 1)); }
  for arm in $ARMS; do
    mkdir -p check/$arm.$cxx
    /w/bin/$arm.$cxx --output_dir=/w/out/check/$arm.$cxx 2>> check.err || { echo "$arm.$cxx failed" >> check.log; diffs=$((diffs + 1)); }
  done
  for arm in $ARMS; do
    for form in ordered_map flat_map unordered_map; do
      if cmp -s check/$arm.$cxx/stocks.$form.html check/H.$cxx/stocks.$form.html; then r=same; else r=DIFFERENT; diffs=$((diffs + 1)); fi
      echo "$r $cxx $arm $form against H, $(sha256sum < check/$arm.$cxx/stocks.$form.html | cut -c1-16) $(wc -c < check/$arm.$cxx/stocks.$form.html) bytes" >> check.log
    done
  done
  if cmp -s check/H.$cxx/stocks.ordered_map.html check/ref.$cxx.html; then r=same; else r=DIFFERENT; diffs=$((diffs + 1)); fi
  echo "$r $cxx H ordered_map against bench/render.cpp of 3e49a1c" >> check.log
done
if cmp -s check/H.g++-16/stocks.ordered_map.html check/H.clang++-23/stocks.ordered_map.html; then r=same; else r=DIFFERENT; diffs=$((diffs + 1)); fi
echo "$r H ordered_map of g++-16 against H ordered_map of clang++-23" >> check.log
echo "$diffs" > check-diffs.txt
