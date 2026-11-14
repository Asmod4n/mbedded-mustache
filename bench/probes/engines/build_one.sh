#!/bin/sh
# Builds one C++ binary. Usage: build_one.sh ENGINE COMPILER WORKLOAD
# ENGINE is mbedded, mbedded-twin, mbedded-typed, inja, mstch or kainjow. The binary is /w/bin/ENGINE.WORKLOAD.COMPILER.
engine=$1
cxx=$2
workload=$3
. /w/flags.sh
flags=$(flags_for "$cxx")
name=/w/bin/$engine.$workload.$cxx
probe=/w/src/bench/probes/engines/cpp
echo "$flags" > "$name.flags"
case $engine in
  mbedded|mbedded-twin) $flags -DWORKLOAD="$workload" -I/w/src/include /w/src/bench/render.cpp -lbenchmark -lpthread -o "$name" ;;
  mbedded-typed) $flags -DWORKLOAD="$workload" -I/w/src/include "$probe/typed.cpp" -lbenchmark -lpthread -o "$name" ;;
  inja) $flags -DWORKLOAD="$workload" -I/w/src/inja/single_include "$probe/inja.cpp" -lbenchmark -lpthread -o "$name" ;;
  kainjow) $flags -DWORKLOAD="$workload" "$probe/kainjow.cpp" -lbenchmark -lpthread -o "$name" ;;
  mstch) $flags -DWORKLOAD="$workload" -I/w/src/mstch/include "$probe/mstch.cpp" "/w/obj/$cxx/libmstch.a" -lbenchmark -lpthread -o "$name" ;;
esac > "$name.log" 2>&1 || echo "build $name: $(grep -m1 -E 'error|Error' "$name.log")" >> /w/out/failed.txt
