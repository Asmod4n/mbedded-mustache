#!/bin/sh
# Builds one binary. Usage: build_one.sh ARM COMPILER. The binary is /w/bin/ARM.COMPILER.
# ARM ref is bench/render.cpp of 3e49a1c, workload 2, over the headers of H.
arm=$1
cxx=$2
. /w/src/bench/probes/runtime-speed/flags.sh
flags=$(flags_for "$cxx")
name=/w/bin/$arm.$cxx
here=/w/src/bench/probes/runtime-speed
echo "$flags" > "$name.flags"
case $arm in
  ref) $flags -DWORKLOAD=2 -I/w/arms/H/include /w/ref/bench/render.cpp -lbenchmark -lpthread -o "$name" ;;
  *)   $flags -I/w/arms/$arm/include "$here/stocks.cpp" -lbenchmark -lpthread -o "$name" ;;
esac > "$name.log" 2>&1 || echo "build $name: $(grep -m1 -E 'error|Error' "$name.log")" >> /w/out/failed.txt
