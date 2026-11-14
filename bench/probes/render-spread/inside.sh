#!/bin/sh
# Container side, as root. Builds the five arms with g++-16 and clang++-23, four builds at a time, records the context,
# and then runs run_as_nobody.sh as nobody. Stderr of every step goes to a file under /w/out.
set -u
. /w/src/bench/probes/engines/flags.sh
probe=/w/src/bench/probes/render-spread
mkdir -p /w/bin
: > /w/out/failed.txt
build() {
  flags=$(flags_for "$2")
  echo "$flags -Wall -Wextra -Werror -DARM=$1" > /w/bin/$1.$2.flags
  $flags -Wall -Wextra -Werror -DARM=$1 -I/w/src/include $probe/probe.cpp -lbenchmark -lpthread -o /w/bin/$1.$2 \
    > /w/out/build-$1.$2.log 2>&1 || echo "build $1.$2 failed" >> /w/out/failed.txt
}
build heap g++-16 & build cap g++-16 & build warm g++-16 & build pmr g++-16 & wait
build typed g++-16 & build heap clang++-23 & build cap clang++-23 & build warm clang++-23 & wait
build pmr clang++-23 & build typed clang++-23 & wait
chmod -R a+rX /w/bin
sha256sum /w/bin/* /w/rbin/render > /w/out/binaries.sha256
ls -l /w/bin /w/rbin > /w/out/binaries.ls
{
echo "commit=$PROBE_COMMIT"
echo "dirty_files=$DIRTY"
echo "image=$IMAGE_ID"
echo "podman=$PODMAN_VERSION, --network none, --security-opt seccomp=unconfined"
echo "g++=$(g++-16 --version | head -1)"
echo "clang=$(clang++-23 --version | head -1)"
echo "google_benchmark=$(dpkg-query -W -f='${Version}' libbenchmark-dev)"
echo "libstdcxx=$(ls -l $(ldconfig -p | awk '/libstdc\+\+.so.6 /{print $NF; exit}') | awk '{print $NF}')"
echo "libc=$(ldd --version | head -1)"
echo "instruction_set=$(g++-16 -march=x86-64-v4 -dM -E -x c++ /dev/null | grep -E '__AVX|__SSE4' | awk '{print $2}' | sort | tr '\n' ' ')"
echo "kernel=$(uname -srv)"
echo "scheduler=sched_autogroup_enabled $(cat /proc/sys/kernel/sched_autogroup_enabled)"
echo "randomize_va_space=$(cat /proc/sys/kernel/randomize_va_space)"
echo "machine=$(lscpu | grep -E 'Hypervisor vendor|Virtualization type' | tr -s ' ' | tr '\n' ';')"
echo "container=podman, cpus visible $(nproc)"
echo "processor=$(grep -m1 'model name' /proc/cpuinfo | cut -d: -f2)"
echo "processor_flags_digest=sha256:$(grep -m1 '^flags' /proc/cpuinfo | sha256sum | cut -c1-16)"
echo "transparent_hugepage=$(cat /sys/kernel/mm/transparent_hugepage/enabled)"
echo "ramhorns=binary of bench/results/ramhorns-spread-20261114T153040Z (A, probe 9aa4b55), Criterion --warm-up-time 0.1 --measurement-time 1.5"
echo "google_benchmark_arguments=--benchmark_repetitions=${REPETITIONS:-6} --benchmark_min_time=0.05s"
echo "threads=1, not pinned"
} > /w/out/context.txt
chmod -R a+rX /w/out
chmod a+w /w/out/failed.txt
runuser -u nobody -- env EXECUTABLES="${EXECUTABLES:-}" REPETITIONS="${REPETITIONS:-6}" sh $probe/run_as_nobody.sh
