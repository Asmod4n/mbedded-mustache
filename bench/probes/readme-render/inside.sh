#!/bin/sh
# Container side: builds one binary for each workload and compiler at -O2, measures the noise floor, runs each
# binary once as nobody with 30 repetitions in random interleaving, and writes the JSON of Google Benchmark to
# /w/out/json. Stderr goes to log files.
set -u
apt-get update -q > /w/out/apt.log 2>&1
apt-get install -y -q libbenchmark-dev sysbench >> /w/out/apt.log 2>&1
mkdir -p /w/bin /w/out/json
chmod a+rwx /w/bin
chmod a+rwx /w/out/json
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
  flags="$cxx $std $level -march=x86-64-v4 $align -DNDEBUG -DWORKLOAD=$workload"
  echo "$flags" > $name.flags
  $flags -I/w/src/include /w/src/bench/render.cpp -lbenchmark -lpthread -o $name > $name.log 2>&1 ||
    echo "$name: $(grep -m1 error $name.log)" >> /w/out/failed.txt
'
chmod -R a+rX /w/bin
ls /w/bin | grep -v '\.log$' | grep -v '\.flags$' > /w/out/binaries.txt
cat > /w/run_as_user.sh <<'F'
#!/bin/sh
set -u
cd /w
cat /proc/loadavg > /w/out/loadavg-before.txt
for i in 1 2 3 4 5 6 7 8 9 10; do sysbench cpu --threads=1 --time=1 run 2>> /w/out/sysbench.err | awk '/events per second/ {printf "%s ", $4}'; done > /w/out/noise.txt
{
echo "commit=$PROBE_COMMIT"
echo "dirty_files=$DIRTY"
echo "image=$IMAGE_ID"
echo "podman=$PODMAN_VERSION"
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
echo "threads=1"
echo "sysbench_cpu_events_per_second_10_runs_of_1s_1_thread=$(cat /w/out/noise.txt)"
echo "loadavg_before=$(cat /w/out/loadavg-before.txt)"
} > /w/out/context-common.txt
while read -r b; do
  { cat /w/out/context-common.txt
    echo "binary=$b"
    echo "flags=$(cat /w/bin/$b.flags)"
    echo "compiler=$(readelf -p .comment /w/bin/$b | grep -iE 'GCC|clang' | head -1 | sed 's/^ *\[ *[0-9a-f]*\] *//')"
  } > /w/out/context-$b.txt
  set --
  while IFS= read -r line; do set -- "$@" "--benchmark_context=$line"; done < /w/out/context-$b.txt
  /w/bin/$b --benchmark_repetitions=30 --benchmark_enable_random_interleaving=true \
    --benchmark_out=/w/out/json/$b.json --benchmark_out_format=json "$@" \
    > /w/out/json/$b.console 2> /w/out/json/$b.err ||
    echo "binary $b exit $?" >> /w/out/failed.txt
done < /w/out/binaries.txt
F
chmod a+rx /w/run_as_user.sh
runuser -u nobody -- env PROBE_COMMIT="$PROBE_COMMIT" DIRTY="$DIRTY" IMAGE_ID="$IMAGE_ID" PODMAN_VERSION="$PODMAN_VERSION" \
  sh /w/run_as_user.sh
