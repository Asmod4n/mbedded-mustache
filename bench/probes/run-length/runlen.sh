#!/bin/sh
set -u
for n in a b; do g++-16 -std=c++26 -freflection -O2 -march=x86-64-v4 -falign-functions=64 -falign-loops=64 -falign-jumps=64 -DNDEBUG -DWORKLOAD=1 -I/arms/new/include /arms/new/bench/render.cpp -lbenchmark -lpthread -o /b/rl-$n; done
for t in 0.05 0.1 0.3 1.0; do
  for round in 1 2 3; do
    for n in a b; do
      echo "t=$t round=$round bin=$n $(/b/rl-$n --benchmark_repetitions=5 --benchmark_report_aggregates_only=true --benchmark_min_time=${t}s 2>&1 | grep -E 'run_time_median|reflection_median' | awk '{printf "%s=%s ", $1, $2}')"
    done
  done
done
