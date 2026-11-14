#!/bin/sh
# Runs as nobody. Measures the noise floor with sysbench, then runs ten rounds. Each round starts every binary once as its own
# process, in a new random order. Google Benchmark writes one JSON file for each process.
set -u
cd /w/out
echo "user=$(id -un) nice=$(nice) threads=1 not pinned" > run-context.txt
date -u +%Y-%m-%dT%H:%M:%SZ > noise-start.txt
for i in 1 2 3 4 5 6 7 8 9 10; do sysbench cpu --threads=1 --time=1 run 2>> sysbench.err | awk '/events per second/ {printf "%s ", $4}'; done > noise.txt
date -u +%Y-%m-%dT%H:%M:%SZ > noise-end.txt
date -u +%Y-%m-%dT%H:%M:%SZ > timed-start.txt
for round in 1 2 3 4 5 6 7 8 9 10; do
  order=$(printf '%s\n' R R2 P | shuf | tr '\n' ' ')
  echo "round $round: $order" >> order.txt
  mkdir -p json/round$round
  for exe in $order; do
    /w/bin/$exe --benchmark_repetitions=30 --benchmark_min_time=0.05s \
      --benchmark_out=/w/out/json/round$round/$exe.json --benchmark_out_format=json \
      > json/round$round/$exe.console 2> json/round$round/$exe.err < /dev/null || echo "round $round $exe exit $?" >> failed.txt
  done
done
date -u +%Y-%m-%dT%H:%M:%SZ > timed-end.txt
