#!/bin/sh
# Builds one binary. Usage: build_one.sh ARM COMPILER KIND. KIND is stocks (the timed source), extras (the byte check) or ref.
# The binary is /w/bin/KIND.ARM.COMPILER. The compile time in seconds is in its .seconds file.
# ref is bench/render.cpp of 3e49a1c, workload 2, over the headers of head, with the data of node.hpp at test/node.hpp.
arm=$1
cxx=$2
kind=$3
. /w/src/bench/probes/fewer-calls/flags.sh
flags=$(flags_for "$cxx")
name=/w/bin/$kind.$arm.$cxx
here=/w/src/bench/probes/fewer-calls
echo "$flags" > "$name.flags"
start=$(date +%s.%N)
case $kind in
  ref)    $flags -DWORKLOAD=2 -I/w/arms/head/include /w/ref/bench/render.cpp -lbenchmark -lpthread -o "$name" ;;
  stocks) $flags -I/w/arms/$arm/include "$here/stocks.cpp" -lbenchmark -lpthread -o "$name" ;;
  extras) $flags -I/w/arms/$arm/include "$here/extras.cpp" -o "$name" ;;
esac > "$name.log" 2>&1 || echo "build $name: $(grep -m1 -E 'error|Error' "$name.log")" >> /w/out/failed.txt
end=$(date +%s.%N)
echo "$end $start" | awk '{printf "%.1f\n", $1 - $2}' > "$name.seconds"
