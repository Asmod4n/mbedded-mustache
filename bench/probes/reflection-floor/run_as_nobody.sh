#!/bin/sh
# Runs as nobody. Checks that R, R2 and C write the same bytes, measures the noise floor with sysbench, and then runs ten
# rounds. Each round starts every binary once as its own process, in a new random order. Before and after each process the
# cpu line of /proc/stat is recorded, so that work of other processes during the timing can be seen.
set -u
cd /w/out
echo "user=$(id -un) nice=$(nice) threads=1 not pinned; nobody owns no other process, so the run has the default priority" > run-context.txt
mkdir -p check
date -u +%Y-%m-%dT%H:%M:%SZ > check-start.txt
for n in R R2 C S; do
  /w/bin/$n --output_file=check/$n.out 2>> check/err.txt < /dev/null || echo "output of $n failed" >> failed.txt
done
sha256sum check/R.out check/R2.out check/C.out > check/sha256.txt
cat check/S.out > check/marks.txt
date -u +%Y-%m-%dT%H:%M:%SZ > check-end.txt
date -u +%Y-%m-%dT%H:%M:%SZ > noise-start.txt
for i in 1 2 3 4 5 6 7 8 9 10; do sysbench cpu --threads=1 --time=1 run 2>> sysbench.err | awk '/events per second/ {printf "%s ", $4}'; done > noise.txt
date -u +%Y-%m-%dT%H:%M:%SZ > noise-end.txt
date -u +%Y-%m-%dT%H:%M:%SZ > timed-start.txt
for round in 1 2 3 4 5 6 7 8 9 10; do
  order=$(printf '%s\n' R R2 C S | shuf | tr '\n' ' ')
  echo "round $round: $order" >> order.txt
  for exe in $order; do
    d=round$round/$exe
    mkdir -p $d
    grep '^cpu ' /proc/stat > $d/stat-before.txt
    date -u +%Y-%m-%dT%H:%M:%S.%NZ > $d/start.txt
    /w/bin/$exe --benchmark_repetitions=30 --benchmark_min_time=0.05s \
      --benchmark_out=/w/out/$d/gb.json --benchmark_out_format=json > $d/console.txt 2> $d/err.txt < /dev/null ||
      echo "round $round $exe exit $?" >> failed.txt
    date -u +%Y-%m-%dT%H:%M:%S.%NZ > $d/end.txt
    grep '^cpu ' /proc/stat > $d/stat-after.txt
  done
done
date -u +%Y-%m-%dT%H:%M:%SZ > timed-end.txt
