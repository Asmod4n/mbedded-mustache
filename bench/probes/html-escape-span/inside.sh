#!/bin/sh
# Container side: builds one binary for each arm and build with its assembly (by default 4 arms, 2 compilers,
# 2 levels, 2 instruction sets), measures the noise floor, runs the arms alternately as nobody, writes /w/out.
# ARMS and BUILDS choose a part of the arms and builds.
set -u
rounds=${ROUNDS:-6}
arms=${ARMS:-0 1 2 3}
builds=${BUILDS:-g++-16:-O2:v3 g++-16:-Os:v3 g++-16:-O2:v4 g++-16:-Os:v4 clang++-23:-O2:v3 clang++-23:-Os:v3 clang++-23:-O2:v4 clang++-23:-Os:v4}
apt-get update -q > /w/out/apt.log 2>&1
apt-get install -y -q sysbench >> /w/out/apt.log 2>&1
mkdir -p /w/bin
jobs=""
for b in $builds; do for a in $arms; do jobs="$jobs $b:$a"; done; done
echo $jobs | tr ' ' '\n' | xargs -P 4 -I{} sh -c '
  cxx=$(echo {} | cut -d: -f1); level=$(echo {} | cut -d: -f2); isa=$(echo {} | cut -d: -f3); a=$(echo {} | cut -d: -f4)
  case $cxx in
    g++*) align="-falign-functions=64 -falign-loops=64 -falign-jumps=64";;
    *)    align="-falign-functions=64 -falign-loops=64";;
  esac
  f="-std=c++26 $level -march=x86-64-$isa $align -DNDEBUG -Wall -Wextra -Werror -DARM=$a -I/w/old/include -I/w"
  name=/w/bin/probe-$cxx$level-$isa-$a
  { $cxx $f /w/probe.cpp -o $name && $cxx $f /w/probe.cpp -S -masm=intel -fno-asynchronous-unwind-tables -o $name.s; } > $name.log 2>&1 ||
    echo "$name: $(grep -m1 error $name.log)"
'
chmod -R a+rX /w/bin
cat > /w/run_as_user.sh <<F
#!/bin/sh
set -u
cat /proc/loadavg > /w/out/loadavg-before.txt
for i in 1 2 3 4 5 6 7 8 9 10; do sysbench cpu --threads=1 --time=1 run | awk '/events per second/ {printf "%s ", \$4}'; done > /w/out/noise.txt
round=1
while [ \$round -le $rounds ]; do
  for b in $builds; do
    if [ \$((round % 2)) = 1 ]; then order="$arms"; else order="\$(echo $arms | tr ' ' '\n' | tac | tr '\n' ' ')"; fi
    for a in \$order; do
      /w/bin/probe-\$(echo \$b | cut -d: -f1)\$(echo \$b | cut -d: -f2)-\$(echo \$b | cut -d: -f3)-\$a \$round "arm=\$a build=\$b" >> /w/out/runs.txt 2>> /w/out/runs.err ||
        echo "round \$round build \$b arm \$a exit \$?" >> /w/out/failed.txt
    done
  done
  round=\$((round + 1))
done
cat /proc/loadavg > /w/out/loadavg-after.txt
F
chmod a+rx /w/run_as_user.sh
runuser -u nobody -- sh /w/run_as_user.sh
{
echo "probe commit: $PROBE_COMMIT (dirty files: $DIRTY); headers of the arm before: $BEFORE_COMMIT"
echo "run: $rounds rounds, arm order alternates by round, each process 5 repetitions of 2^22 input bytes for each input, one thread, as $(id -un nobody), nice $(runuser -u nobody -- nice)"
echo "image: $IMAGE_ID; $PODMAN_VERSION"
echo "instruction set v3: $(g++-16 -march=x86-64-v3 -dM -E -x c++ /dev/null | grep -E '__AVX|__SSE4|__BMI' | awk '{print $2}' | sort | tr '\n' ' ')"
echo "instruction set v4: $(g++-16 -march=x86-64-v4 -dM -E -x c++ /dev/null | grep -E '__AVX|__SSE4|__BMI' | awk '{print $2}' | sort | tr '\n' ' ')"
echo "arms: $arms"
echo "builds: $builds"
echo "compiler of the binaries: $(readelf -p .comment $(ls /w/bin/probe-g++-16-* | grep -v '\.' | head -1) | grep GCC | head -1 | sed 's/^ *\[ *[0-9a-f]*\] *//'); $(readelf -p .comment $(ls /w/bin/probe-clang++-23-* | grep -v '\.' | head -1) | grep -i clang | head -1 | sed 's/^ *\[ *[0-9a-f]*\] *//')"
echo "libstdc++ and libc: $(ls -l $(ldconfig -p | awk '/libstdc\+\+.so.6 /{print $NF; exit}') | awk '{print $NF}'); $(ldd --version | head -1)"
echo "kernel: $(uname -srv); sched_autogroup_enabled=$(cat /proc/sys/kernel/sched_autogroup_enabled)"
echo "machine: $(lscpu | grep -E 'Hypervisor vendor|Virtualization type' | tr -s ' ' | tr '\n' ';'); container: podman, cpus visible $(nproc)"
echo "processor: $(grep -m1 'model name' /proc/cpuinfo | cut -d: -f2); flags digest sha256:$(grep -m1 '^flags' /proc/cpuinfo | sha256sum | cut -c1-16)"
echo "avx-512 flags of the processor: $(grep -m1 '^flags' /proc/cpuinfo | tr ' ' '\n' | grep avx512 | sort | tr '\n' ' ')"
echo "load average before and after: $(cat /w/out/loadavg-before.txt) / $(cat /w/out/loadavg-after.txt)"
} > /w/out/context.txt
