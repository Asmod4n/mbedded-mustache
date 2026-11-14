#!/bin/sh
set -u
out=/b/result-$(date -u +%Y%m%dT%H%M%SZ).txt
{
echo "context: ran_as=$(id -un) nice=$(nice) cpus=$(nproc) kernel=$(uname -r) cpu=$(grep -m1 'model name' /proc/cpuinfo | cut -d: -f2)"
echo "compilers: $(g++-16 --version | head -1); $(clang++-23 --version | head -1); libstdc++ from g++-16"
echo "noise floor, sysbench cpu, events per second, one thread:"
for i in 1 2 3 4 5 6 7 8 9 10; do sysbench cpu --threads=1 --time=1 run | awk '/events per second/ {printf "%s ", $4}'; done
echo
for round in 1 2; do
  for w in 1 2 3; do for c in g++-16 clang++-23; do for l in -Os -O2; do
    if [ $round = 1 ]; then arms="old new"; else arms="new old"; fi
    for a in $arms; do
      f=/b/bin/render-$a-$w-$c$l
      echo "== round=$round arm=$a workload=$w compiler=$c level=$l"
      $f --benchmark_repetitions=5 --benchmark_report_aggregates_only=true --benchmark_min_time=0.3s 2>&1 | grep -E "_median|_cv"
    done
  done; done; done
done
} > $out 2>&1
echo $out
