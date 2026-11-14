#!/bin/sh
# Builds one binary. Usage: build_one.sh KIND ARM PAGE COMPILER.
# KIND stocks: the timed source, /w/bin/stocks.ARM.PAGE.COMPILER, over the include of the arm, with the rows of the page and the
#   initial capacity of the arm.
# KIND extras: the byte check, /w/bin/extras.ARM.COMPILER (ARM is a variant, PAGE is -).
# KIND ref: bench/render.cpp of 5d3c154 with WORKLOAD 2 over the include of main, /w/bin/ref.COMPILER.
# The compile time in seconds is in the .seconds file of the binary.
kind=$1
arm=$2
page=$3
cxx=$4
here=/w/src/bench/probes/engine-work
. "$here/flags.sh"
flags=$(flags_for "$cxx")
inc=/w/arms/$(include_of "$arm")/include
case $kind in
  stocks) name=/w/bin/stocks.$arm.$page.$cxx ;;
  extras) name=/w/bin/extras.$arm.$cxx; inc=/w/arms/$arm/include ;;
  ref)    name=/w/bin/ref.$cxx; inc=/w/arms/main/include ;;
esac
echo "$flags" > "$name.flags"
start=$(date +%s.%N)
case $kind in
  ref)    $flags -DWORKLOAD=2 -I$inc /w/src/bench/render.cpp -lbenchmark -lpthread -o "$name" ;;
  stocks) $flags -DROWS=$(rows_of "$page") -DINITIAL_CAPACITY=$(capacity_of "$arm") -I$inc -I/w/src/bench "$here/stocks.cpp" -lbenchmark -lpthread -o "$name" ;;
  extras) $flags -I$inc -I/w/src/bench "$here/extras.cpp" -o "$name" ;;
esac > "$name.log" 2>&1 || echo "build $name: $(grep -m1 -E 'error|Error' "$name.log")" >> /w/out/failed.txt
end=$(date +%s.%N)
echo "$end $start" | awk '{printf "%.1f\n", $1 - $2}' > "$name.seconds"
