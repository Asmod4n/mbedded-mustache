#!/bin/sh
arms="1 7 20 50"
rev=$(echo $arms | tr ' ' '\n' | tac | tr '\n' ' ')
for round in $(seq 1 8); do
  for cxx in g++-16 clang++-23; do for level in -Os -O2; do
    if [ $((round % 2)) = 1 ]; then order="$arms"; else order="$rev"; fi
    for a in $order; do
      /w/b/v-$cxx$level-$a --benchmark_format=json --benchmark_repetitions=3 --benchmark_report_aggregates_only=false --benchmark_min_time=0.05s 2>> /w/res/final/run.err > /w/res/final/j-$round-$cxx$level-$a.json
    done
  done; done
done
