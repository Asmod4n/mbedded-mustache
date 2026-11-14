#!/bin/sh
# builds /w/final.cpp as arm 50 in the four builds
for cxx in g++-16 clang++-23; do for level in -Os -O2; do
  case $cxx in g++*) align="-falign-functions=64 -falign-loops=64 -falign-jumps=64";; *) align="-falign-functions=64 -falign-loops=64";; esac
  f="-std=c++26 $level -march=x86-64-v4 $align -I/src/include -Wno-deprecated-declarations"
  ( $cxx $f /w/final.cpp -lbenchmark -lpthread -o /w/b/v-$cxx$level-50 > /w/b/v-$cxx$level-50.log 2>&1 || echo "FAIL $cxx $level"
    $cxx $f /w/final.cpp -S -masm=intel -fno-asynchronous-unwind-tables -o /w/b/v-$cxx$level-50.s 2>>/w/b/v-$cxx$level-50.log ) &
done; done; wait
chmod -R a+rX /w/b
