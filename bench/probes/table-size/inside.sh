#!/bin/sh
# Container side: installs arm-linux-gnueabihf-g++-16, builds probe.cpp for each state in /w/h and each form
# (no media type, text/html, application/json, a media type known at run time), four builds at a time,
# and prints "size <state> <form> <section> <bytes>" for .text, .rodata and their sum. Writes /w/out.
set -u
apt-get update -q > /w/out/apt.log 2>&1
apt-get install -y -q --no-install-recommends g++-16-arm-linux-gnueabihf >> /w/out/apt.log 2>&1 || { echo "apt failed"; tail -5 /w/out/apt.log; exit 1; }
mkdir -p /w/bin
jobs=""
for s in $STATES; do for f in none html json runtime; do jobs="$jobs $s:$f"; done; done
echo $jobs | tr ' ' '\n' | xargs -P 4 -I{} sh -c '
  s=$(echo {} | cut -d: -f1); f=$(echo {} | cut -d: -f2)
  case $f in
    none)    d="";;
    html)    d="-DMEDIA=\"text/html\"";;
    json)    d="-DMEDIA=\"application/json\"";;
    runtime) d="-DMEDIA_RUN_TIME";;
  esac
  name=/w/bin/probe-$s-$f
  arm-linux-gnueabihf-g++-16 -std=c++23 -Os -mthumb -ffunction-sections -fdata-sections -Wall -Wextra -Werror \
    -I/w/h/$s/include $d /w/probe.cpp -Wl,--gc-sections -o $name.elf > $name.log 2>&1 ||
    echo "$name: build failed: $(grep -m1 error $name.log)"
'
for s in $STATES; do for f in none html json runtime; do
  e=/w/bin/probe-$s-$f.elf
  [ -f $e ] || continue
  arm-linux-gnueabihf-size -A $e | awk -v s=$s -v f=$f '
    $1 == ".text" { t = $2 } $1 == ".rodata" { r = $2 }
    END { print "size", s, f, ".text", t; print "size", s, f, ".rodata", r; print "size", s, f, ".text+.rodata", t + r }'
done; done | tee /w/out/sizes.txt
e=$(ls /w/bin/*.elf | head -1)
{
echo "probe commit: $PROBE_COMMIT (dirty files: $DIRTY)"
echo "states: $STATES; indent_at repaired in the exported copy of:$REPAIRED"
echo "image: $IMAGE_ID; $PODMAN_VERSION"
echo "compiler: $(arm-linux-gnueabihf-g++-16 --version | head -1); read off a binary: $(arm-linux-gnueabihf-readelf -p .comment $e | grep GCC | head -1 | sed 's/^ *\[ *[0-9a-f]*\] *//')"
echo "binutils: $(arm-linux-gnueabihf-ld --version | head -1)"
echo "packages: $(dpkg-query -W -f '${Package} ${Version}; ' g++-16-arm-linux-gnueabihf libstdc++6-armhf-cross libc6-armhf-cross 2>&1)"
echo "shared libraries the binary needs: $(arm-linux-gnueabihf-readelf -d $e | awk '/NEEDED/ {print $NF}' | tr '\n' ' ')"
echo "target of the build: $(arm-linux-gnueabihf-g++-16 -mthumb -Q --help=target | grep -E '^ *-m(arch|cpu|float-abi|fpu|thumb)[= ]' | tr -s ' \t' ' ' | tr '\n' ';')"
echo "kernel: $(uname -srv); machine: $(lscpu | grep -E 'Hypervisor vendor' | tr -s ' '); container: podman, cpus visible $(nproc)"
} > /w/out/context.txt
