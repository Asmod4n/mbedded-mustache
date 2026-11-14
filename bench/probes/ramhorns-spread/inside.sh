#!/bin/sh
# Container side. Builds the ramhorns bench twice (A and Ap, two target directories, one flag line as in the engines probe)
# and the mbedded run-time arm with clang++-23 once (M). Reads the load average once and, when it is 0.5 or more, waits once
# for the time that the one-minute load average needs to fall below 0.4. Then runs run_as_nobody.sh as nobody.
# Stderr goes to log files under /w/out.
set -u
. /w/flags.sh
mkdir -p /w/out /w/bin /w/rbin
: > /w/out/failed.txt
chmod a+rwx /w/out /w/bin
apt-get update -q > /w/out/apt.log 2>&1
apt-get install -y -q libbenchmark-dev rustc cargo ca-certificates python3 coreutils >> /w/out/apt.log 2>&1 || echo "apt install failed" >> /w/out/failed.txt

cp -r /w/src/bench/probes/engines/rust /w/rwork
export CARGO_HOME=/w/cargo-home RUSTFLAGS="-C target-cpu=x86-64-v4"
(cd /w/rwork && cargo fetch --locked > /w/out/cargo-fetch.log 2>&1) || echo "cargo fetch failed" >> /w/out/failed.txt

build_rust() {
  (cd /w/rwork && CARGO_TARGET_DIR=/w/rt-$1 cargo bench --no-run --locked -p ramhorns-bench --message-format=json > /w/out/cargo-$1.json 2> /w/out/cargo-$1.log) ||
    echo "cargo bench --no-run $1 failed" >> /w/out/failed.txt
  mkdir -p /w/rbin/$1
  python3 -I -c '
import json, sys
for line in open(sys.argv[1]):
    if not line.startswith("{"):
        continue
    message = json.loads(line)
    if message.get("reason") == "compiler-artifact" and message.get("executable") and "bench" in message["target"]["kind"]:
        print(message["executable"])
' /w/out/cargo-$1.json > /w/out/exe-$1.txt 2>> /w/out/failed.txt
  cp "$(cat /w/out/exe-$1.txt)" /w/rbin/$1/render
}
build_rust A &
build_rust Ap &
sh /w/build_one.sh mbedded clang++-23 2 &
wait
chmod -R a+rX /w/bin /w/rbin
sha256sum /w/rbin/A/render /w/rbin/Ap/render /w/bin/mbedded.2.clang++-23 > /w/out/binaries.sha256
ls -l /w/rbin/A/render /w/rbin/Ap/render /w/bin/mbedded.2.clang++-23 > /w/out/binaries.ls

load=$(cut -d' ' -f1 /proc/loadavg)
echo "load before the wait: $(cat /proc/loadavg)" > /w/out/wait.txt
wait_seconds=$(awk -v l="$load" 'BEGIN { if (l >= 0.5) printf "%d", 60 * log(l / 0.4) + 10; else print 0 }')
echo "one wait of $wait_seconds s" >> /w/out/wait.txt
[ "$wait_seconds" -gt 0 ] && sleep "$wait_seconds"
echo "load after the wait: $(cat /proc/loadavg)" >> /w/out/wait.txt
if ! awk -v l="$(cut -d' ' -f1 /proc/loadavg)" 'BEGIN { exit !(l < 0.5) }'; then
  echo "load is not under 0.5, no timing" >> /w/out/failed.txt
  exit 1
fi
{
echo "rustc=$(rustc --version)"
echo "cargo=$(cargo --version)"
echo "debian_packages=$(dpkg-query -W -f='${Package} ${Version}; ' libbenchmark-dev rustc cargo)"
echo "clang=$(clang++-23 --version | head -1)"
echo "rust_crates=$(awk '/^name = "(ramhorns|criterion)"$/ {n=$3; getline; printf "%s %s; ", n, $3}' /w/rwork/Cargo.lock)"
echo "rustflags=$RUSTFLAGS, profile bench: opt-level 3, codegen-units 1, lto fat"
echo "mbedded_flags=$(cat /w/bin/mbedded.2.clang++-23.flags)"
echo "commit=$PROBE_COMMIT"
echo "dirty_files=$DIRTY"
echo "image=$IMAGE_ID"
echo "podman=$PODMAN_VERSION"
echo "kernel=$(uname -srv)"
echo "machine=$(lscpu | grep -E 'Hypervisor vendor|Virtualization type' | tr -s ' ' | tr '\n' ';')"
echo "container=podman, cpus visible $(nproc)"
echo "processor=$(grep -m1 'model name' /proc/cpuinfo | cut -d: -f2)"
echo "processor_flags_digest=sha256:$(grep -m1 '^flags' /proc/cpuinfo | sha256sum | cut -c1-16)"
} > /w/out/context.txt
chmod -R a+rX /w/out
chmod a+w /w/out/failed.txt
runuser -u nobody -- sh /w/run_as_nobody.sh
