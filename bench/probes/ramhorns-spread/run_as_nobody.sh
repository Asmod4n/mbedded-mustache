#!/bin/sh
# Runs as nobody. Five rounds. Each round runs A, Ap and M once, in a new random order. Before and after each executable it
# records /proc/loadavg and the cpu MHz lines of /proc/cpuinfo. A and Ap are the two builds of the ramhorns bench, M is the
# mbedded run-time arm built with clang++-23. Criterion arguments and Google Benchmark arguments are those of the engines probe.
set -u
cd /w/out
echo "user=$(id -un) nice=$(nice) threads=1 not pinned" > run-context.txt
for round in 1 2 3 4 5; do
  order=$(printf 'A\nAp\nM\n' | shuf | tr '\n' ' ')
  echo "round $round: $order" >> order.txt
  for exe in $order; do
    d=round$round/$exe
    mkdir -p $d
    cat /proc/loadavg > $d/loadavg-before.txt
    grep 'cpu MHz' /proc/cpuinfo > $d/mhz-before.txt
    date -u +%Y-%m-%dT%H:%M:%S.%NZ > $d/start.txt
    case $exe in
      M) /w/bin/mbedded.2.clang++-23 --benchmark_min_time=0.05s --benchmark_repetitions=6 \
           --benchmark_out=/w/out/$d/gb.json --benchmark_out_format=json > $d/console.txt 2> $d/err.txt < /dev/null ||
           echo "round $round $exe exit $?" >> failed.txt ;;
      *) CRITERION_HOME=/w/out/$d/criterion /w/rbin/$exe/render --bench w2 --warm-up-time 0.1 --measurement-time 1.5 \
           > $d/console.txt 2> $d/err.txt < /dev/null ||
           echo "round $round $exe exit $?" >> failed.txt ;;
    esac
    date -u +%Y-%m-%dT%H:%M:%S.%NZ > $d/end.txt
    cat /proc/loadavg > $d/loadavg-after.txt
    grep 'cpu MHz' /proc/cpuinfo > $d/mhz-after.txt
  done
done
