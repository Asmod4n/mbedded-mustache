#!/bin/sh
# $1 = tree, $2 = compiler, $3 = extra flags
set -e
cd /w/$1
cmake -S . -B build-$2 -G Ninja -DCMAKE_CXX_COMPILER=$2 -DCMAKE_CXX_STANDARD=26 "-DCMAKE_CXX_FLAGS=$3 -O2 -march=x86-64-v3 -Wall -Wextra -Werror" > build-$2.cmake.log 2>&1
cmake --build build-$2 -j 2 > build-$2.build.log 2>&1 || { tail -40 build-$2.build.log; exit 1; }
ctest --test-dir build-$2 -j 2 > build-$2.ctest.log 2>&1 || true
tail -25 build-$2.ctest.log
