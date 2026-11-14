#!/bin/sh
# Builds one binary. Usage: build_one.sh ARM COMPILER. The binary is /w/bin/stocks.ARM.COMPILER; reused defines ARM_REUSED.
# The compile time in seconds is in its .seconds file.
arm=$1
cxx=$2
. /w/src/bench/probes/output-targets/flags.sh
flags=$(flags_for "$cxx")
name=/w/bin/stocks.$arm.$cxx
echo "$flags" > "$name.flags"
define=""
[ "$arm" = reused ] && define="-DARM_REUSED"
start=$(date +%s.%N)
$flags $define -I/w/src/include -I/w/src/bench /w/src/bench/probes/output-targets/output_targets.cpp -lbenchmark -lpthread -o "$name" > "$name.log" 2>&1 ||
  echo "build $name: $(grep -m1 -E 'error|Error' "$name.log")" >> /w/out/failed.txt
end=$(date +%s.%N)
echo "$end $start" | awk '{printf "%.1f\n", $1 - $2}' > "$name.seconds"
