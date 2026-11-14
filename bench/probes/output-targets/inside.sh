#!/bin/sh
# Container side. One build (g++-16, release flags). Three binaries: fresh (render into a new string), twin (fresh built
# again, the A/A arm) and reused (render_to into a kept string). Checks the bytes, then runs each binary once as nobody,
# 30 repetitions of at least 0.05 s, in the order fresh, reused, twin. Compares with tools/compare.py of Google Benchmark.
set -u
cd /w/out
src=/w/src/bench/probes/output-targets
flags="g++-16 -std=c++26 -freflection -O3 -DNDEBUG -march=x86-64-v4"
mkdir -p bin check json compare
: > failed.txt
stamp() { echo "$1 $(date -u +%H:%M:%S.%N | cut -c1-12)" >> steps.txt; }
stamp build-start
for arm in fresh twin reused; do
  define=""
  [ "$arm" = reused ] && define="-DARM_REUSED"
  $flags $define -I/w/src/include -I/w/src/bench $src/output_targets.cpp -lbenchmark -lpthread -o bin/$arm > bin/$arm.log 2>&1 ||
    echo "build $arm failed" >> failed.txt &
done
wait
stamp build-end
chmod -R a+rwX /w/out
for arm in fresh twin reused; do
  runuser -u nobody -- bin/$arm --output_file=check/$arm.html 2>> check.err || echo "output $arm failed" >> failed.txt
  if cmp -s check/$arm.html check/fresh.html; then r=same; else r=DIFFERENT; echo "bytes of $arm differ" >> failed.txt; fi
  echo "$r $arm against fresh, $(wc -c < check/$arm.html) bytes" >> check.log
done
stamp check-end
if [ -s failed.txt ]; then cat failed.txt; exit 1; fi
{
echo "commit=$PROBE_COMMIT"
echo "dirty_files=$DIRTY"
echo "image=$IMAGE_ID"
echo "podman=$PODMAN_VERSION"
echo "flags=$flags"
echo "compiler=$(readelf -p .comment bin/fresh | grep -m1 GCC | sed 's/^ *\[ *[0-9a-f]*\] *//')"
echo "instruction_set=$(g++-16 -march=x86-64-v4 -dM -E -x c++ /dev/null | grep -E '__AVX|__SSE4' | awk '{print $2}' | sort | tr '\n' ' ')"
echo "libstdcxx=$(ls -l $(ldconfig -p | awk '/libstdc\+\+.so.6 /{print $NF; exit}') | awk '{print $NF}')"
echo "libc=$(ldd --version | head -1)"
echo "kernel=$(uname -srv)"
echo "machine=$(lscpu | grep -E 'Hypervisor vendor|Virtualization type' | tr -s ' ' | tr '\n' ';')"
echo "container=podman, cpus visible $(nproc)"
echo "processor=$(grep -m1 'model name' /proc/cpuinfo | cut -d: -f2)"
echo "processor_flags_digest=sha256:$(grep -m1 '^flags' /proc/cpuinfo | sha256sum | cut -c1-16)"
echo "user=nobody"
echo "priority=nobody cannot lower nice; default priority, not pinned"
echo "threads=1"
echo "google_benchmark=one process per binary, 30 repetitions, --benchmark_min_time=0.05s, order fresh reused twin"
echo "compare=tools/compare.py of google/benchmark $(cat /opt/gb-commit.txt)"
} > context.txt
for arm in fresh reused twin; do
  set --
  while IFS= read -r line; do set -- "$@" "--benchmark_context=$line"; done < context.txt
  runuser -u nobody -- bin/$arm --benchmark_repetitions=30 --benchmark_min_time=0.05s \
    --benchmark_out=/w/out/json/$arm.json --benchmark_out_format=json --benchmark_context=binary=$arm "$@" \
    > json/$arm.console 2> json/$arm.err < /dev/null || echo "run $arm failed" >> failed.txt
done
stamp timed-end
for arm in twin reused; do
  python3 /opt/gb/tools/compare.py --no-color -d compare/$arm.json benchmarks json/fresh.json json/$arm.json > compare/$arm.txt 2> compare/$arm.err ||
    echo "compare $arm failed" >> failed.txt
done
stamp compare-end
jq -n --arg commit "$PROBE_COMMIT" --rawfile steps steps.txt --rawfile check check.log \
  --slurpfile fresh json/fresh.json --slurpfile twin json/twin.json --slurpfile reused json/reused.json \
  --slurpfile ctwin compare/twin.json --slurpfile creused compare/reused.json '{
  name: "output-targets",
  what: "the stocks page, rendered at run time from std::unordered_map data: render_to into a kept std::string (reused) against render into a new std::string (fresh); twin is fresh built again, the A/A arm",
  method: "one container: one build (g++-16, the flags in context.flags), three binaries built in parallel, a byte check, each binary run once as nobody with 30 repetitions of at least 0.05 s in the order fresh, reused, twin, then tools/compare.py of Google Benchmark (Mann-Whitney U) against fresh",
  commit: $commit,
  steps_utc: ($steps | split("\n") | map(select(length > 0))),
  byte_check: ($check | split("\n") | map(select(length > 0))),
  google_benchmark: {fresh: $fresh[0], twin: $twin[0], reused: $reused[0]},
  compare_against_fresh: {twin: $ctwin[0], reused: $creused[0]}
}' > result.json || echo "join of the result failed" >> failed.txt
cat steps.txt check.log failed.txt
for arm in twin reused; do echo "== $arm against fresh"; grep -E "_(mean|median|stddev|cv)|U Test" compare/$arm.txt; done
