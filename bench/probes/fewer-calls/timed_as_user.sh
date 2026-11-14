#!/bin/sh
# Runs as nobody. Noise floor first, then 5 rounds of 6 repetitions of every binary, each round in a new random order.
set -u
. /w/src/bench/probes/fewer-calls/flags.sh
cd /w/out
rounds=5
repetitions=6
gb_extra="--benchmark_min_time=0.05s --benchmark_enable_random_interleaving=true"
if [ "${PILOT:-0}" = 1 ]; then
  rounds=1
  repetitions=2
  gb_extra="--benchmark_min_time=0.01s --benchmark_enable_random_interleaving=true"
fi
cat /proc/loadavg > loadavg-before.txt
for i in 1 2 3 4 5 6 7 8 9 10; do sysbench cpu --threads=1 --time=1 run 2>> sysbench.err | awk '/events per second/ {printf "%s ", $4}'; done > noise.txt
: > fixed-order.txt
for cxx in $COMPILERS; do for arm in $(cells_of $cxx); do echo "$arm.$cxx" >> fixed-order.txt; done; done
seed=$(od -An -N4 -tu4 /dev/urandom | tr -d ' ')
echo "$seed" > seed.txt
{
echo "commit=$PROBE_COMMIT"
echo "dirty_files=$DIRTY"
echo "image=$IMAGE_ID"
echo "podman=$PODMAN_VERSION"
echo "instruction_set=$(g++-16 -march=x86-64-v4 -dM -E -x c++ /dev/null | grep -E '__AVX|__SSE4' | awk '{print $2}' | sort | tr '\n' ' ')"
echo "libstdcxx=$(ls -l $(ldconfig -p | awk '/libstdc\+\+.so.6 /{print $NF; exit}') | awk '{print $NF}')"
echo "libc=$(ldd --version | head -1)"
echo "kernel=$(uname -srv)"
echo "sched_autogroup_enabled=$(cat /proc/sys/kernel/sched_autogroup_enabled 2> /dev/null)"
echo "machine=$(lscpu | grep -E 'Hypervisor vendor|Virtualization type' | tr -s ' ' | tr '\n' ';')"
echo "container=podman, cpus visible $(nproc)"
echo "processor=$(grep -m1 'model name' /proc/cpuinfo | cut -d: -f2)"
echo "processor_flags_digest=sha256:$(grep -m1 '^flags' /proc/cpuinfo | sha256sum | cut -c1-16)"
echo "avx512_flags=$(grep -m1 '^flags' /proc/cpuinfo | tr ' ' '\n' | grep avx512 | sort | tr '\n' ' ')"
echo "user=$(id -un)"
echo "nice=$(nice)"
echo "priority=nobody cannot lower nice; the run has the default priority and is not pinned"
echo "threads=1"
echo "sysbench_cpu_events_per_second_10_runs_of_1s_1_thread=$(cat noise.txt)"
echo "loadavg_before=$(cat loadavg-before.txt)"
echo "order_seed=$seed"
echo "order=a new random order of the binaries in each round: random.Random(seed * 1000 + round).shuffle of fixed-order.txt, shuffle.py"
echo "pilot=${PILOT:-0}"
echo "google_benchmark=$rounds rounds of $repetitions repetitions, $gb_extra"
} > context-common.txt
date -u +%Y-%m-%dT%H:%M:%SZ > timed-start.txt
round=1
while [ $round -le $rounds ]; do
  python3 -I /w/src/bench/probes/fewer-calls/shuffle.py fixed-order.txt "$seed" $round > order-$round.txt
  mkdir -p json/round$round
  cat /proc/loadavg > loadavg-round$round-before.txt
  while read -r b; do
    { cat context-common.txt
      echo "round=$round"
      echo "order=$(tr '\n' ' ' < order-$round.txt)"
      echo "binary=$b"
      echo "flags=$(cat /w/bin/stocks.$b.flags)"
      echo "compiler=$(readelf -p .comment /w/bin/stocks.$b | grep -iE 'GCC|clang' | head -1 | sed 's/^ *\[ *[0-9a-f]*\] *//')"
    } > json/round$round/$b.context.txt
    set --
    while IFS= read -r line; do set -- "$@" "--benchmark_context=$line"; done < json/round$round/$b.context.txt
    /w/bin/stocks.$b --benchmark_repetitions=$repetitions $gb_extra \
      --benchmark_out=/w/out/json/round$round/$b.json --benchmark_out_format=json "$@" \
      > json/round$round/$b.console 2> json/round$round/$b.err < /dev/null ||
      echo "round $round binary $b exit $?" >> failed.txt
  done < order-$round.txt
  cat /proc/loadavg > loadavg-round$round-after.txt
  round=$((round + 1))
done
date -u +%Y-%m-%dT%H:%M:%SZ > timed-end.txt
cat /proc/loadavg > loadavg-after.txt
