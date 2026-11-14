#!/bin/sh
# Host side. Usage: bench/probes/output-targets/run.sh WORK
# Exports include/, bench/node.hpp and this probe of HEAD to WORK/src, then builds, checks, times and compares in one container.
set -eu
work=$1
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../../.." && pwd)
mkdir -p "$work/src" "$work/out"
git -C "$root" archive HEAD include bench/node.hpp bench/probes/output-targets | tar -x -C "$work/src"
chmod -R a+rwX "$work"
podman run --rm --network none \
  -e PROBE_COMMIT="$(git -C "$root" rev-parse HEAD)" -e DIRTY="$(git -C "$root" status --porcelain | wc -l)" \
  -e PODMAN_VERSION="$(podman --version | tr ' ' '_')" -e IMAGE_ID="$(podman image inspect localhost/mustache-bench --format '{{.Id}}')" \
  -v "$work":/w localhost/mustache-bench sh /w/src/bench/probes/output-targets/inside.sh
result="$root/bench/results/output-targets-$(date -u +%Y%m%dT%H%M%SZ).json"
cp "$work/out/result.json" "$result"
echo "result $result"
