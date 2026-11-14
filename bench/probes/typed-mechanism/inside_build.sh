#!/bin/sh
# Container side, as root, with network. Installs the tools, takes tools/compare.py from the release of Google Benchmark, builds the
# arms three at a time, checks that every arm writes the bytes of bench/render.cpp workload 2, and records the .text sizes.
set -u
. /w/src/bench/probes/typed-mechanism/flags.sh
probe=/w/src/bench/probes/typed-mechanism
mkdir -p /w/bin /w/out /w/check
: > /w/out/failed.txt
stamp() { date -u +%Y-%m-%dT%H:%M:%SZ; }
stamp > /w/out/phase-apt-start.txt
apt-get update -q > /w/out/apt.log 2>&1
apt-get install -y -q libbenchmark-dev libboost-dev sysbench python3 python3-scipy python3-numpy git ca-certificates >> /w/out/apt.log 2>&1 ||
  echo "apt install failed" >> /w/out/failed.txt
version=$(dpkg-query -W -f='${Version}' libbenchmark-dev | sed 's/[-+].*//')
echo "$version" > /w/out/gb-version.txt
git clone -q --depth 1 --branch "v$version" https://github.com/google/benchmark /w/gb > /w/out/git-gb.log 2>&1 || echo "clone google benchmark v$version failed" >> /w/out/failed.txt
git -C /w/gb rev-parse HEAD > /w/out/gb-commit.txt 2>> /w/out/git-gb.log
stamp > /w/out/phase-apt-end.txt
stamp > /w/out/phase-build-start.txt
echo "$flags_reflection -DWORKLOAD=2" > /w/bin/expected.flags
$flags_reflection -DWORKLOAD=2 -I/w/src/include /w/src/bench/render.cpp -lbenchmark -lpthread -o /w/bin/expected > /w/out/build-expected.log 2>&1 ||
  echo "build expected failed" >> /w/out/failed.txt &
for arm in R R2; do
  echo "$flags_reflection" > /w/bin/$arm.flags
  $flags_reflection -I/w/src/include $probe/typed.cpp -lbenchmark -lpthread -o /w/bin/$arm > /w/out/build-$arm.log 2>&1 ||
    echo "build $arm failed" >> /w/out/failed.txt &
done
wait
echo "$flags_pfr" > /w/bin/P.flags
$flags_pfr -I/w/src/include $probe/typed.cpp -lbenchmark -lpthread -o /w/bin/P > /w/out/build-P.log 2>&1 || echo "build P failed" >> /w/out/failed.txt
stamp > /w/out/phase-build-end.txt
chmod -R a+rX /w/bin
stamp > /w/out/phase-check-start.txt
for n in expected R R2 P; do
  /w/bin/$n --output_file=/w/check/$n.html 2>> /w/out/check.err < /dev/null || echo "output of $n failed" >> /w/out/failed.txt
done
for n in R R2 P; do
  if cmp /w/check/expected.html /w/check/$n.html >> /w/out/check.log 2>&1; then echo "$n equals expected" >> /w/out/check.log; else echo "$n differs from expected" >> /w/out/failed.txt; fi
done
wc -c /w/check/*.html > /w/out/check-bytes.txt
sha256sum /w/check/*.html > /w/out/check-sha256.txt
cp /w/check/expected.html /w/out/expected.html
stamp > /w/out/phase-check-end.txt
size -A /w/bin/R /w/bin/R2 /w/bin/P | grep -E '^/w|^\.text' > /w/out/sizes-text.txt
size /w/bin/R /w/bin/R2 /w/bin/P > /w/out/sizes.txt
sha256sum /w/bin/R /w/bin/R2 /w/bin/P > /w/out/binaries.sha256
{
echo "commit=$PROBE_COMMIT"
echo "dirty_files=$DIRTY"
echo "podman=$PODMAN_VERSION"
echo "g++=$(g++-16 --version | head -1)"
echo "boost=$(dpkg-query -W -f='${Version}' libboost-dev)"
echo "google_benchmark=$(dpkg-query -W -f='${Version}' libbenchmark-dev)"
echo "libstdcxx=$(ls -l $(ldconfig -p | awk '/libstdc\+\+.so.6 /{print $NF; exit}') | awk '{print $NF}')"
echo "libc=$(ldd --version | head -1)"
echo "flags_reflection=$flags_reflection"
echo "flags_pfr=$flags_pfr"
echo "instruction_set=$(g++-16 -march=x86-64-v4 -dM -E -x c++ /dev/null | grep -E '__AVX|__SSE4' | awk '{print $2}' | sort | tr '\n' ' ')"
echo "kernel=$(uname -srv)"
echo "scheduler=sched_autogroup_enabled $(cat /proc/sys/kernel/sched_autogroup_enabled)"
echo "machine=$(lscpu | grep -E 'Hypervisor vendor|Virtualization type' | tr -s ' ' | tr '\n' ';')"
echo "container=podman, cpus visible $(nproc)"
echo "processor=$(grep -m1 'model name' /proc/cpuinfo | cut -d: -f2)"
echo "processor_flags_digest=sha256:$(grep -m1 '^flags' /proc/cpuinfo | sha256sum | cut -c1-16)"
echo "google_benchmark_arguments=--benchmark_repetitions=30 --benchmark_min_time=0.05s, 10 processes per binary, a new random order per round"
} > /w/out/context-build.txt
chmod -R a+rX /w/out /w/check
