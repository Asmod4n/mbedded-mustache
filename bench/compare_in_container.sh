#!/bin/sh
# Container side: builds 24 binaries, measures the noise floor, runs the arms alternately, writes /w/out/result.txt.
set -u
rounds=${ROUNDS:-6}
apt-get update -q > /w/out/apt.log 2>&1
apt-get install -y -q libbenchmark-dev sysbench >> /w/out/apt.log 2>&1
mkdir -p /w/bin
chmod a+rwx /w/bin
jobs=""
for arm in old new; do for cxx in g++-16 clang++-23; do for level in -Os -O2; do for workload in 1 2 3; do
  jobs="$jobs $arm:$cxx:$level:$workload"
done; done; done; done
echo $jobs | tr ' ' '\n' | xargs -P 4 -I{} sh -c '
  arm=$(echo {} | cut -d: -f1); cxx=$(echo {} | cut -d: -f2); level=$(echo {} | cut -d: -f3); workload=$(echo {} | cut -d: -f4)
  case $cxx in
    g++*) std="-std=c++26 -freflection"; align="-falign-functions=64 -falign-loops=64 -falign-jumps=64";;
    *)    std="-std=c++26"; align="-falign-functions=64 -falign-loops=64";;
  esac
  name=/w/bin/render-$arm-$workload-$cxx$level
  $cxx $std $level -march=x86-64-v4 $align -DNDEBUG -DWORKLOAD=$workload -I/w/$arm/include \
    /w/new/bench/render.cpp -lbenchmark -lpthread -o $name > $name.log 2>&1 || echo "$name: $(grep -m1 error $name.log)"
'
chmod -R a+rX /w/bin
cat > /w/run_as_user.sh <<'F'
#!/bin/sh
set -u
cd /w
for i in 1 2 3 4 5 6 7 8 9 10; do sysbench cpu --threads=1 --time=1 run | awk '/events per second/ {printf "%s ", $4}'; done > /w/out/noise.txt
mkdir -p /w/out/runs
round=1
while [ $round -le $ROUNDS ]; do
  for w in 1 2 3; do for c in g++-16 clang++-23; do for l in -Os -O2; do
    if [ $((round % 2)) = 1 ]; then arms="old new"; else arms="new old"; fi
    for a in $arms; do
      /w/bin/render-$a-$w-$c$l --benchmark_format=json --benchmark_repetitions=5 --benchmark_report_aggregates_only=false \
        --benchmark_min_time=0.05s > /w/out/runs/r$round-$a-$w-$c$l.json 2> /w/out/runs/r$round-$a-$w-$c$l.err
    done
  done; done; done
  round=$((round + 1))
done
F
chmod a+rx /w/run_as_user.sh
runuser -u nobody -- env ROUNDS=$rounds sh /w/run_as_user.sh
{
echo "what: render time of bench/render.cpp, workloads 1 to 3, run-time render and reflection render, real time in ns per render"
echo "arms: old = headers of $OLD, new = headers of $NEW; bench source and this script from commit $SCRIPT_COMMIT (dirty files: $DIRTY)"
echo "build: -march=x86-64-v4 -DNDEBUG -falign-functions=64 -falign-loops=64 (g++ also -falign-jumps=64), -std=c++26, one binary for each arm, workload, compiler and level"
echo "run: $rounds rounds, arm order alternates by round, each run 5 repetitions of at least 0.05 s, one thread, as $(id -un nobody), nice $(runuser -u nobody -- nice)"
echo "image: $IMAGE_ID; $PODMAN_VERSION"
echo "instruction set: $(g++-16 -march=x86-64-v4 -dM -E -x c++ /dev/null | grep -E '__AVX|__SSE4' | awk '{print $2}' | sort | tr '\n' ' ')"
echo "compiler of the binaries: $(readelf -p .comment /w/bin/render-new-1-g++-16-O2 | grep GCC | head -1 | sed 's/^ *\[ *[0-9a-f]*\] *//'); $(readelf -p .comment /w/bin/render-new-1-clang++-23-O2 | grep -i clang | head -1 | sed 's/^ *\[ *[0-9a-f]*\] *//')"
echo "libstdc++ and libc: $(ls -l $(ldconfig -p | awk '/libstdc\+\+.so.6 /{print $NF; exit}') | awk '{print $NF}'); $(ldd --version | head -1)"
echo "kernel: $(uname -srv); scheduler files: $(ls /proc/sys/kernel | grep sched | tr '\n' ' '); sched_autogroup_enabled=$(cat /proc/sys/kernel/sched_autogroup_enabled)"
echo "machine: $(lscpu | grep -E 'Hypervisor vendor|Virtualization type' | tr -s ' ' | tr '\n' ';'); container: podman, cpus visible $(nproc)"
echo "processor: $(grep -m1 'model name' /proc/cpuinfo | cut -d: -f2); flags digest sha256:$(grep -m1 '^flags' /proc/cpuinfo | sha256sum | cut -c1-16)"
echo "threads of the bench: 1"
echo "noise floor, sysbench cpu, events per second, 10 runs of 1 s, one thread: $(cat /w/out/noise.txt)"
echo
python3 /w/summarize.py /w/out/runs
} > /w/out/result.txt
