#!/bin/sh
# Container side: builds one binary for each workload and compiler at -O2, measures the noise floor, runs the
# binaries as nobody in 6 rounds with a rotating order, writes /w/out. Stderr goes to log files.
set -u
rounds=${ROUNDS:-6}
apt-get update -q > /w/out/apt.log 2>&1
apt-get install -y -q libbenchmark-dev sysbench >> /w/out/apt.log 2>&1
mkdir -p /w/bin
chmod a+rwx /w/bin
jobs=""
for cxx in g++-16 clang++-23; do for level in -O2; do for workload in 1 2 3; do
  jobs="$jobs $cxx:$level:$workload"
done; done; done
echo $jobs | tr ' ' '\n' | xargs -P 4 -I{} sh -c '
  cxx=$(echo {} | cut -d: -f1); level=$(echo {} | cut -d: -f2); workload=$(echo {} | cut -d: -f3)
  case $cxx in
    g++*) std="-std=c++26 -freflection"; align="-falign-functions=64 -falign-loops=64 -falign-jumps=64";;
    *)    std="-std=c++26"; align="-falign-functions=64 -falign-loops=64";;
  esac
  name=/w/bin/render-$workload-$cxx$level
  $cxx $std $level -march=x86-64-v4 $align -DNDEBUG -DWORKLOAD=$workload -I/w/src/include \
    /w/src/bench/render.cpp -lbenchmark -lpthread -o $name > $name.log 2>&1 || echo "$name: $(grep -m1 error $name.log)"
'
chmod -R a+rX /w/bin
ls /w/bin | grep -v '\.log$' > /w/out/binaries.txt
cat > /w/run_as_user.sh <<'F'
#!/bin/sh
set -u
cd /w
cat /proc/loadavg > /w/out/loadavg-before.txt
for i in 1 2 3 4 5 6 7 8 9 10; do sysbench cpu --threads=1 --time=1 run 2>> /w/out/sysbench.err | awk '/events per second/ {printf "%s ", $4}'; done > /w/out/noise.txt
mkdir -p /w/out/runs
count=$(wc -l < /w/out/binaries.txt)
round=1
while [ $round -le $ROUNDS ]; do
  shift_by=$(( (round - 1) * count / ROUNDS ))
  { tail -n +$((shift_by + 1)) /w/out/binaries.txt; head -n $shift_by /w/out/binaries.txt; } | while read -r b; do
    /w/bin/$b --benchmark_format=json --benchmark_repetitions=5 --benchmark_report_aggregates_only=false \
      --benchmark_min_time=0.05s > /w/out/runs/r$round-$b.json 2> /w/out/runs/r$round-$b.err ||
      echo "round $round binary $b exit $?" >> /w/out/failed.txt
  done
  round=$((round + 1))
done
cat /proc/loadavg > /w/out/loadavg-after.txt
F
chmod a+rx /w/run_as_user.sh
runuser -u nobody -- env ROUNDS=$rounds sh /w/run_as_user.sh
{
echo "probe commit: $PROBE_COMMIT (dirty files: $DIRTY)"
echo "run: $rounds rounds, the order of the binaries rotates by round, each run 5 repetitions of at least 0.05 s, one thread, as $(id -un nobody), nice $(runuser -u nobody -- nice)"
echo "image: $IMAGE_ID; $PODMAN_VERSION"
echo "instruction set: $(g++-16 -march=x86-64-v4 -dM -E -x c++ /dev/null | grep -E '__AVX|__SSE4' | awk '{print $2}' | sort | tr '\n' ' ')"
echo "compiler of the binaries: $(readelf -p .comment /w/bin/render-1-g++-16-O2 | grep GCC | head -1 | sed 's/^ *\[ *[0-9a-f]*\] *//'); $(readelf -p .comment /w/bin/render-1-clang++-23-O2 | grep -i clang | head -1 | sed 's/^ *\[ *[0-9a-f]*\] *//')"
echo "libstdc++ and libc: $(ls -l $(ldconfig -p | awk '/libstdc\+\+.so.6 /{print $NF; exit}') | awk '{print $NF}'); $(ldd --version | head -1)"
echo "kernel: $(uname -srv); sched_autogroup_enabled=$(cat /proc/sys/kernel/sched_autogroup_enabled)"
echo "machine: $(lscpu | grep -E 'Hypervisor vendor|Virtualization type' | tr -s ' ' | tr '\n' ';'); container: podman, cpus visible $(nproc)"
echo "processor: $(grep -m1 'model name' /proc/cpuinfo | cut -d: -f2); flags digest sha256:$(grep -m1 '^flags' /proc/cpuinfo | sha256sum | cut -c1-16)"
echo "avx-512 flags of the processor: $(grep -m1 '^flags' /proc/cpuinfo | tr ' ' '\n' | grep avx512 | sort | tr '\n' ' ')"
echo "load average before and after: $(cat /w/out/loadavg-before.txt) / $(cat /w/out/loadavg-after.txt)"
} > /w/out/context.txt 2> /w/out/context.err
