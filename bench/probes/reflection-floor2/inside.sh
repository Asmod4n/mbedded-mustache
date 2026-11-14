#!/bin/sh
# Container side, as root. Builds the three arms with g++-16, three builds at a time, records the context, and then runs
# run_as_nobody.sh as nobody. Stderr of every step goes to a file under /w/out.
set -u
probe=/w/src/bench/probes/reflection-floor2
flags="g++-16 -std=c++26 -freflection -O2 -march=x86-64-v4 -falign-functions=64 -falign-loops=64 -falign-jumps=64 -DNDEBUG"
mkdir -p /w/bin
: > /w/out/failed.txt
date -u +%Y-%m-%dT%H:%M:%SZ > /w/out/build-start.txt
for arm in R R2 S2; do
  echo "$flags -Wall -Wextra -Werror -DARM=$arm" > /w/bin/$arm.flags
  $flags -Wall -Wextra -Werror -DARM=$arm -I/w/src/include $probe/probe.cpp -lbenchmark -lpthread -o /w/bin/$arm \
    > /w/out/build-$arm.log 2>&1 || echo "build $arm failed" >> /w/out/failed.txt &
done
wait
date -u +%Y-%m-%dT%H:%M:%SZ > /w/out/build-end.txt
chmod -R a+rX /w/bin
sha256sum /w/bin/R /w/bin/R2 /w/bin/S2 > /w/out/binaries.sha256
ls -l /w/bin > /w/out/binaries.ls
{
echo "commit=$PROBE_COMMIT"
echo "dirty_files=$DIRTY"
echo "image=$IMAGE_ID"
echo "podman=$PODMAN_VERSION, --network none"
echo "compile=$flags -Wall -Wextra -Werror -DARM=<arm>"
echo "g++=$(g++-16 --version | head -1)"
echo "google_benchmark=$(dpkg-query -W -f='${Version}' libbenchmark-dev)"
echo "libstdcxx=$(ls -l $(ldconfig -p | awk '/libstdc\+\+.so.6 /{print $NF; exit}') | awk '{print $NF}')"
echo "libc=$(ldd --version | head -1)"
echo "instruction_set=$(g++-16 -march=x86-64-v4 -dM -E -x c++ /dev/null | grep -E '__AVX|__SSE4' | awk '{print $2}' | sort | tr '\n' ' ')"
echo "kernel=$(uname -srv)"
echo "scheduler=sched_autogroup_enabled $(cat /proc/sys/kernel/sched_autogroup_enabled)"
echo "machine=$(lscpu | grep -E 'Hypervisor vendor|Virtualization type' | tr -s ' ' | tr '\n' ';')"
echo "container=podman, cpus visible $(nproc)"
echo "processor=$(grep -m1 'model name' /proc/cpuinfo | cut -d: -f2)"
echo "processor_flags_digest=sha256:$(grep -m1 '^flags' /proc/cpuinfo | sha256sum | cut -c1-16)"
echo "google_benchmark_arguments=--benchmark_repetitions=30 --benchmark_min_time=0.05s, 10 processes per binary"
echo "threads=1, not pinned"
} > /w/out/context.txt
chmod -R a+rX /w/out
chmod a+w /w/out/failed.txt
runuser -u nobody -- sh $probe/run_as_nobody.sh
