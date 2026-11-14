#!/bin/sh
cd /w
for cxx in g++-16 clang++-23; do
  echo "== $cxx, room = size + T"
  $cxx -std=c++26 -O2 -Wall -Wextra -Werror p5.cpp -o p5_$cxx > p5_$cxx.log 2>&1 && ./p5_$cxx || tail -8 p5_$cxx.log
  echo "== $cxx, room = size (must not compile)"
  $cxx -std=c++26 -O2 -Wall -Wextra -Werror -DSHORT p5.cpp -o p5s_$cxx > p5s_$cxx.log 2>&1 && echo COMPILED || grep -m3 "error" p5s_$cxx.log
done
