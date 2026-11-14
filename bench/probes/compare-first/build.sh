#!/bin/sh
set -eu
out=/b/bin
mkdir -p "$out"
jobs=""
for arm in old new; do for cxx in g++-16 clang++-23; do for level in -Os -O2; do for workload in 1 2 3; do
  jobs="$jobs $arm:$cxx:$level:$workload"
done; done; done; done
echo $jobs | tr ' ' '\n' | xargs -P 4 -I{} sh -c '
  arm=$(echo {} | cut -d: -f1); cxx=$(echo {} | cut -d: -f2); level=$(echo {} | cut -d: -f3); workload=$(echo {} | cut -d: -f4)
  case $cxx in
    g++*) std="-std=c++26 -freflection"; align="-falign-functions=64 -falign-loops=64 -falign-jumps=64";;
    *)    std="-std=c++26"; align="-falign-functions=64 -falign-loops=64";;
  esac
  name=/b/bin/render-$arm-$workload-$cxx$level
  $cxx $std $level -march=x86-64-v4 $align -DNDEBUG -DWORKLOAD=$workload -I/arms/$arm/include \
    /arms/new/bench/render.cpp -lbenchmark -lpthread -o $name > $name.log 2>&1 \
    || echo "$name: $(grep -m1 error $name.log)"
'
ls "$out" | grep -v log
