#!/bin/sh
# builds /w/bench.cpp for each arm in the four builds, binaries and assembly in /w/b
jobs=""
for cxx in g++-16 clang++-23; do for level in -Os -O2; do for a in ${ARMS:-1 2 50 60 61 62}; do jobs="$jobs $cxx:$level:$a"; done; done; done
echo $jobs | tr ' ' '\n' | xargs -P 4 -I{} sh -c '
  cxx=$(echo {} | cut -d: -f1); level=$(echo {} | cut -d: -f2); a=$(echo {} | cut -d: -f3)
  case $cxx in g++*) align="-falign-functions=64 -falign-loops=64 -falign-jumps=64";; *) align="-falign-functions=64 -falign-loops=64";; esac
  f="-std=c++26 $level -march=x86-64-v4 $align -DARM=$a -I/src/include -Wno-deprecated-declarations"
  $cxx $f /w/bench.cpp -lbenchmark -lpthread -o /w/b/v-$cxx$level-$a > /w/b/v-$cxx$level-$a.log 2>&1 || { echo "FAIL $cxx $level $a"; head -5 /w/b/v-$cxx$level-$a.log; }
  $cxx $f /w/bench.cpp -S -masm=intel -fno-asynchronous-unwind-tables -o /w/b/v-$cxx$level-$a.s 2>>/w/b/v-$cxx$level-$a.log
  $cxx $f -D_GLIBCXX_ASSERTIONS /w/bench.cpp -S -masm=intel -fno-asynchronous-unwind-tables -o /w/b/asrt-$cxx$level-$a.s 2>>/w/b/v-$cxx$level-$a.log
'
for cxx in g++-16 clang++-23; do
  $cxx -std=c++26 -O2 -march=x86-64-v4 -Wall -Wextra -Werror -I/src/include /w/fit.cpp -o /w/b/fit-$cxx > /w/b/fit-$cxx.log 2>&1 || { echo "FAIL fit $cxx"; head -20 /w/b/fit-$cxx.log; }
  $cxx -std=c++26 -O2 -march=x86-64-v4 -Wall -Wextra -Werror -D_GLIBCXX_ASSERTIONS -I/src/include /w/fit.cpp -o /w/b/fita-$cxx > /w/b/fita-$cxx.log 2>&1 || { echo "FAIL fita $cxx"; head -20 /w/b/fita-$cxx.log; }
done
chmod -R a+rX /w/b
