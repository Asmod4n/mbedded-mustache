#!/bin/sh
# Host side: exports include/, test/ and bench/render.cpp of HEAD and runs inside.sh in a container. The result is the JSON that Google Benchmark writes.
# Usage: bench/probes/readme-render/run.sh WORK_DIR [IMAGE]
set -eu
work=$1
image=${2:-localhost/mbedded-cbor-docs}
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../../.." && pwd)
mkdir -p "$work/out" "$work/src"
git -C "$root" archive HEAD include test bench/render.cpp bench/node.hpp | tar -x -C "$work/src"
cp "$here/inside.sh" "$work/"
commit=$(git -C "$root" rev-parse HEAD)
dirty=$(git -C "$root" status --porcelain | wc -l)
digest=$(podman image inspect "$image" --format '{{.Digest}} {{.Id}}')
chmod -R a+rX "$work"
chmod a+w "$work/out"
date -u +%Y%m%dT%H%M%SZ > "$work/out/stamp.txt"
podman run --rm -v "$work":/w -e PROBE_COMMIT="$commit" -e DIRTY="$dirty" -e IMAGE_ID="$digest" \
  -e PODMAN_VERSION="$(podman --version)" "$image" sh /w/inside.sh
