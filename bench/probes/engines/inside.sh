#!/bin/sh
# Container side. Installs the tools, takes inja and mstch from their origin at a named release, builds every C++ binary and
# every Rust bench with one flag line, checks the output of every engine, measures the noise floor, and runs every binary once
# as nobody in one rotated order. Stderr goes to log files under /w/out.
set -u
. /w/flags.sh
mkdir -p /w/out/json /w/out/check /w/out/criterion /w/bin /w/obj /w/rbin
: > /w/out/failed.txt
chmod a+rwx /w/out/json /w/out/check /w/out/criterion /w/bin
apt-get update -q > /w/out/apt.log 2>&1
apt-get install -y -q libbenchmark-dev sysbench git rustc cargo nlohmann-json3-dev libboost-dev libkainjow-mustache-dev ca-certificates python3 \
  >> /w/out/apt.log 2>&1 || echo "apt install failed" >> /w/out/failed.txt
git clone -q --depth 1 --branch v3.5.0 https://github.com/pantor/inja /w/src/inja > /w/out/git-inja.log 2>&1 || echo "clone inja failed" >> /w/out/failed.txt
git clone -q --depth 1 --branch 1.0.2 https://github.com/no1msd/mstch /w/src/mstch > /w/out/git-mstch.log 2>&1 || echo "clone mstch failed" >> /w/out/failed.txt

# mstch: its src/*.cpp with the common flag line, one static library for each compiler.
for cxx in g++-16; do
  mkdir -p /w/obj/$cxx
  flags=$(flags_for $cxx)
  find /w/src/mstch/src -name '*.cpp' | xargs -P 4 -I{} sh -c \
    "$flags -I/w/src/mstch/include -I/w/src/mstch/src -c {} -o /w/obj/$cxx/\$(basename {} .cpp).o 2>> /w/out/mstch-$cxx.log || echo 'build mstch object {} with $cxx failed' >> /w/out/failed.txt"
  ar rcs /w/obj/$cxx/libmstch.a /w/obj/$cxx/*.o > /w/out/ar-$cxx.log 2>&1
done

jobs=""
for cxx in g++-16; do for workload in 2; do for engine in mbedded inja mstch kainjow; do
  jobs="$jobs$engine $cxx $workload\n"
done; done; done
jobs="${jobs}mbedded-twin g++-16 2\n"
printf "$jobs" | xargs -P 4 -L 1 sh /w/build_one.sh

# Rust: one workspace, built once as root; each bench executable is copied for nobody.
cp -r /w/src/bench/probes/engines/rust /w/rwork
export CARGO_HOME=/w/cargo-home CARGO_TARGET_DIR=/w/rtarget RUSTFLAGS="-C target-cpu=x86-64-v4"
(cd /w/rwork && cargo bench --no-run --locked --message-format=json > /w/out/cargo-bench-build.json 2> /w/out/cargo-bench-build.log) ||
  echo "cargo bench --no-run failed" >> /w/out/failed.txt
python3 -I - <<'P' > /w/out/rust-executables.txt 2>> /w/out/failed.txt
import json
for line in open("/w/out/cargo-bench-build.json"):
    if not line.startswith("{"):
        continue
    message = json.loads(line)
    if message.get("reason") == "compiler-artifact" and message.get("executable") and "bench" in message["target"]["kind"]:
        print(message["package_id"].split("#")[0].rsplit("/", 1)[-1].replace("-bench", ""), message["executable"])
P
while read -r crate exe; do mkdir -p /w/rbin/$crate; cp "$exe" /w/rbin/$crate/render; done < /w/out/rust-executables.txt
chmod -R a+rX /w/bin /w/rbin /w/obj

ls /w/bin | grep -v '\.log$' | grep -v '\.flags$' | sort > /w/out/cpp-binaries.txt
{ cat /w/out/cpp-binaries.txt; while read -r crate exe; do echo "rust:$crate"; done < /w/out/rust-executables.txt; } > /w/out/fixed-order.txt
count=$(wc -l < /w/out/fixed-order.txt)
seed=$(od -An -N4 -tu4 /dev/urandom | tr -d ' ')
echo "$seed" > /w/out/seed.txt
start=$((seed % count))
{ tail -n +$((start + 1)) /w/out/fixed-order.txt; head -n $start /w/out/fixed-order.txt; } > /w/out/order.txt
chmod -R a+rX /w/out
{
echo "inja=3.5.0 $(git -C /w/src/inja rev-parse HEAD) from https://github.com/pantor/inja tag v3.5.0"
echo "mstch=1.0.2 $(git -C /w/src/mstch rev-parse HEAD) from https://github.com/no1msd/mstch tag 1.0.2, src/*.cpp built with the common flag line into a static library, Boost headers $(dpkg-query -W -f='${Version}' libboost-dev)"
echo "debian_packages=$(dpkg-query -W -f='${Package} ${Version}; ' nlohmann-json3-dev libkainjow-mustache-dev libbenchmark-dev rustc cargo)"
echo "rustc=$(rustc --version)"
echo "cargo=$(cargo --version)"
echo "rust_crates=$(awk '/^name = "(sailfish|markup|maud|ramhorns|criterion)"$/ {n=$3; getline; printf "%s %s; ", n, $3}' /w/rwork/Cargo.lock)"
} > /w/out/context-tools.txt
chmod a+w /w/out/failed.txt
chmod a+r /w/out/context-tools.txt

runuser -u nobody -- env PROBE_COMMIT="$PROBE_COMMIT" DIRTY="$DIRTY" IMAGE_ID="$IMAGE_ID" PODMAN_VERSION="$PODMAN_VERSION" PILOT="$PILOT" \
  sh /w/run_as_user.sh
