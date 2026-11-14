#!/bin/sh
# usage: build.sh "<compilers>" "<levels>" "<arms>" [extra flags]; builds binaries and assembly in /w/b
extra="${4:-}"
jobs=""
for cxx in $1; do for level in $2; do for a in $3; do jobs="$jobs $cxx:$level:$a"; done; done; done
export extra
echo $jobs | tr ' ' '\n' | xargs -P 4 -I{} sh -c '
  cxx=$(echo {} | cut -d: -f1); level=$(echo {} | cut -d: -f2); a=$(echo {} | cut -d: -f3)
  case $cxx in g++*) align="-falign-functions=64 -falign-loops=64 -falign-jumps=64";; *) align="-falign-functions=64 -falign-loops=64";; esac
  f="-std=c++26 $level -march=x86-64-v4 $align $extra -DARM=$a -I/src/include -Wno-deprecated-declarations"
  $cxx $f /w/v.cpp -lbenchmark -lpthread -o /w/b/v-$cxx$level-$a > /w/b/v-$cxx$level-$a.log 2>&1 || { echo "FAIL $cxx $level $a"; head -5 /w/b/v-$cxx$level-$a.log; }
  $cxx $f /w/v.cpp -S -masm=intel -fno-asynchronous-unwind-tables -o /w/b/v-$cxx$level-$a.s 2>>/w/b/v-$cxx$level-$a.log
'
chmod -R a+rX /w/b
