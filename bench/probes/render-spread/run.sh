#!/bin/sh
# Host side: exports the tree of HEAD and runs inside.sh in a container without network and without the seccomp filter,
# so that setarch -R can turn off address space randomization for the arms that ask for it.
# Usage: bench/probes/render-spread/run.sh WORK_DIR RAMHORNS_BINARY [IMAGE]
set -eu
work=$1
ramhorns=$2
image=${3:-localhost/mbedded-cbor-docs}
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../../.." && pwd)
mkdir -p "$work/out" "$work/src" "$work/rbin"
git -C "$root" archive HEAD include bench/node.hpp bench/probes/engines/flags.sh bench/probes/render-spread | tar -x -C "$work/src"
cp "$ramhorns" "$work/rbin/render"
commit=$(git -C "$root" rev-parse HEAD)
dirty=$(git -C "$root" status --porcelain | wc -l)
digest=$(podman image inspect "$image" --format '{{.Digest}} {{.Id}}')
chmod -R a+rX "$work"
chmod a+w "$work/out"
date -u +%Y%m%dT%H%M%SZ > "$work/out/stamp.txt"
podman run --rm --network none --security-opt seccomp=unconfined \
  -e PROBE_COMMIT="$commit" -e DIRTY="$dirty" -e IMAGE_ID="$digest" \
  -e PODMAN_VERSION="$(podman --version)" \
  -v "$work":/w "$image" sh /w/src/bench/probes/render-spread/inside.sh
