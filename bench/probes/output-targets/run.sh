#!/bin/bash
# Host side. Usage: bench/probes/output-targets/run.sh WORK
# The result form of mbedded-cbor bench/run-desktop.sh: a = render into a new std::string, aa = a built again, b = render_to
# into a kept std::string. The gate (vmstat and ps) runs before the build and before the runs; a busy machine ends the run with
# exit 3 and no result. Writes bench/results/output-targets-<UTC time>.json.
set -eu
work=$1
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../../.." && pwd)
image=localhost/mustache-bench
. "$here/gate.sh"
mkdir -p "$work/src" "$work/out"
exec 9> "$work/run.lock"
flock -n 9 || { echo "another run holds $work/run.lock" >&2; exit 2; }
git -C "$root" archive HEAD include bench/node.hpp bench/probes/output-targets | tar -x -C "$work/src"
commit=$(git -C "$root" rev-parse HEAD)
[ -z "$(git -C "$root" status --porcelain -- include bench)" ] || commit="$commit-dirty"
echo "$commit" > "$work/out/commit"
podman image inspect "$image" --format '{{.Id}}' > "$work/out/image"
{
  echo "kernel=$(uname -r)"
  echo "virtualization=$(systemd-detect-virt 2>&1 | tr '\n' ' ' || true)"
  echo "host_container=$([ -e /run/.containerenv ] || [ -e /.dockerenv ] && echo yes || echo no)"
  if [ -r /sys/kernel/sched_ext/state ]; then echo "scheduler=sched_ext $(cat /sys/kernel/sched_ext/state)"
  else echo "scheduler=no sched_ext state (kernel default)"; fi
  echo "host_user=$(id -un) $(id -u)"
  echo "cpus=$(nproc)"
  echo "level_two=$(cat /sys/devices/system/cpu/cpu0/cache/index2/size)"
} > "$work/out/host"
in_container() {
  podman run --rm --network none --user 65534:65534 --cap-add SYS_NICE --init -e IMAGE="$image" \
    -v "$work":/w:U "$image" bash /w/src/bench/probes/output-targets/inside.sh "$1"
}
gate > "$work/out/gate-before-build" 2>&1 || { cat "$work/out/gate-before-build"; echo "the machine is busy; no run was made" >&2; exit 3; }
in_container build
gate > "$work/out/gate-before-runs" 2>&1 || { cat "$work/out/gate-before-runs"; echo "the machine is busy after the build; no run was made" >&2; exit 3; }
in_container measure
result="$root/bench/results/output-targets-$(cat "$work/out/stamp").json"
cp "$work/out/result.json" "$result"
echo "wrote $result"
