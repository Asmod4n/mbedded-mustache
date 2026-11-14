#!/bin/sh
# Runs as nobody. Checks the output of every engine against the output of bench/render.cpp, measures the noise floor, and runs
# every binary once in the order of /w/out/order.txt. Google Benchmark writes JSON; Criterion writes under /w/out/criterion.
# A binary of C++ is /w/bin/ENGINE.WORKLOAD.COMPILER; a Rust bench is "rust:CRATE" in order.txt.
set -u
cd /w/out
here=/w/src/bench/probes/engines
if [ "$PILOT" = 1 ]; then
  repetitions=2
  gb_extra="--benchmark_min_time=0.02s"
  criterion_extra="--warm-up-time 0.2 --measurement-time 0.5 --sample-size 10"
else
  repetitions=30
  gb_extra="--benchmark_min_time=0.05s"
  criterion_extra="--warm-up-time 0.1 --measurement-time 1.5"
fi
cat /proc/loadavg > loadavg-before.txt

for w in 1 2 3; do
  /w/bin/mbedded.$w.g++-16 --output_file=/w/out/check/expected-$w.html 2>> check.err || echo "expected output of workload $w failed" >> check.log
done
: >> check.log
while read -r b; do
  case $b in
    rust:*)
      crate=${b#rust:}
      mkdir -p /w/out/check/$crate
      /w/rbin/$crate/render --output-dir /w/out/check/$crate 2>> check.err < /dev/null || echo "output of $b failed" >> check.log
      for w in 1 2 3; do
        printf '%s\n' "check $crate rustc workload $w: $(python3 -I $here/check.py /w/out/check/expected-$w.html /w/out/check/$crate/w$w.html | tr '\n' ' ')" >> check.log
      done
      printf '%s\n' "escapes $crate rustc: $(python3 -I $here/escapes.py /w/out/check/$crate/w2.html /w/out/check/$crate/w3.html | tr '\n' ' ')" >> check.log
      ;;
    *)
      "/w/bin/$b" --output_file="/w/out/check/$b.html" 2>> check.err < /dev/null || echo "output of $b failed" >> check.log
      engine=$(echo "$b" | cut -d. -f1)
      w=$(echo "$b" | cut -d. -f2)
      compiler=$(echo "$b" | cut -d. -f3)
      printf '%s\n' "check $engine $compiler workload $w: $(python3 -I $here/check.py /w/out/check/expected-$w.html /w/out/check/$b.html | tr '\n' ' ')" >> check.log
      if [ "$w" = 3 ]; then
        printf '%s\n' "escapes $engine $compiler: $(python3 -I $here/escapes.py /w/out/check/$engine.2.$compiler.html /w/out/check/$engine.3.$compiler.html | tr '\n' ' ')" >> check.log
      fi
      ;;
  esac
done < /w/out/order.txt

for i in 1 2 3 4 5 6 7 8 9 10; do sysbench cpu --threads=1 --time=1 run 2>> sysbench.err | awk '/events per second/ {printf "%s ", $4}'; done > noise.txt
{
echo "commit=$PROBE_COMMIT"
echo "dirty_files=$DIRTY"
echo "image=$IMAGE_ID"
echo "podman=$PODMAN_VERSION"
echo "pilot=$PILOT"
echo "instruction_set=$(g++-16 -march=x86-64-v4 -dM -E -x c++ /dev/null | grep -E '__AVX|__SSE4' | awk '{print $2}' | sort | tr '\n' ' ')"
echo "libstdcxx=$(ls -l $(ldconfig -p | awk '/libstdc\+\+.so.6 /{print $NF; exit}') | awk '{print $NF}')"
echo "libc=$(ldd --version | head -1)"
echo "kernel=$(uname -srv)"
echo "sched_autogroup_enabled=$(cat /proc/sys/kernel/sched_autogroup_enabled)"
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
echo "order_seed=$(cat seed.txt)"
echo "order=$(tr '\n' ' ' < order.txt)"
cat /w/out/context-tools.txt
echo "rust_profile=opt-level 3, codegen-units 1, lto fat, RUSTFLAGS -C target-cpu=x86-64-v4, bench profile inherits release"
echo "criterion=defaults of 0.8.2 (cargo_bench_support only) $criterion_extra"
echo "google_benchmark=repetitions $repetitions, random interleaving, $gb_extra"
echo "loadavg_at_context=$(cat /proc/loadavg)"
} > context-common.txt

date -u +%Y-%m-%dT%H:%M:%SZ > timed-start.txt
while read -r b; do
  case $b in
    rust:*)
      crate=${b#rust:}
      { cat context-common.txt; echo "binary=$b"; echo "build=cargo bench --no-run as root, executable run as nobody with CRITERION_HOME=/w/out/criterion/$crate"; } > context-$b.txt
      mkdir -p /w/out/criterion/$crate
      CRITERION_HOME=/w/out/criterion/$crate /w/rbin/$crate/render --bench $criterion_extra > json/$crate.console 2> json/$crate.err < /dev/null ||
        echo "binary $b exit $?" >> failed.txt
      ;;
    *)
      { cat context-common.txt
        echo "binary=$b"
        echo "flags=$(cat /w/bin/$b.flags)"
        echo "compiler=$(readelf -p .comment /w/bin/$b | grep -iE 'GCC|clang' | head -1 | sed 's/^ *\[ *[0-9a-f]*\] *//')"
      } > context-$b.txt
      set --
      while IFS= read -r line; do set -- "$@" "--benchmark_context=$line"; done < context-$b.txt
      /w/bin/$b --benchmark_repetitions=$repetitions --benchmark_enable_random_interleaving=true $gb_extra \
        --benchmark_out=/w/out/json/$b.json --benchmark_out_format=json "$@" \
        > json/$b.console 2> json/$b.err < /dev/null ||
        echo "binary $b exit $?" >> failed.txt
      ;;
  esac
done < order.txt
date -u +%Y-%m-%dT%H:%M:%SZ > timed-end.txt
cat /proc/loadavg > loadavg-after.txt
