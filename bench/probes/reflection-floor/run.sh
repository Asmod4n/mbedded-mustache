#!/bin/sh
# Host side: exports the tree of HEAD and runs inside.sh in a container without network. The results stay in WORK_DIR/out.
# Usage: bench/probes/reflection-floor/run.sh WORK_DIR [IMAGE]
set -eu
work=$1
image=${2:-localhost/mbedded-cbor-docs}
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../../.." && pwd)
mkdir -p "$work/out" "$work/src"
git -C "$root" archive HEAD include bench/probes/reflection-floor | tar -x -C "$work/src"
commit=$(git -C "$root" rev-parse HEAD)
dirty=$(git -C "$root" status --porcelain | wc -l)
digest=$(podman image inspect "$image" --format '{{.Digest}} {{.Id}}')
chmod -R a+rX "$work"
chmod a+w "$work/out"
date -u +%Y%m%dT%H%M%SZ > "$work/out/stamp.txt"
podman run --rm --network none \
  -e PROBE_COMMIT="$commit" -e DIRTY="$dirty" -e IMAGE_ID="$digest" -e PODMAN_VERSION="$(podman --version)" \
  -v "$work":/w "$image" sh /w/src/bench/probes/reflection-floor/inside.sh
