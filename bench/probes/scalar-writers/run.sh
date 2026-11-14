#!/bin/sh
# Host side: builds every arm of probe.cpp in four builds and runs them in a container, 6 rounds.
# ARMS and BUILDS in the environment choose a part of them (see inside.sh).
# Usage: bench/probes/scalar-writers/run.sh WORK_DIR [IMAGE]
set -eu
work=$1
image=${2:-localhost/mbedded-cbor-docs}
here=$(cd "$(dirname "$0")" && pwd)
mkdir -p "$work/out"
cp "$here/probe.cpp" "$here/inside.sh" "$work/"
commit=$(git -C "$here" rev-parse HEAD)
dirty=$(git -C "$here" status --porcelain | wc -l)
digest=$(podman image inspect "$image" --format '{{.Digest}} {{.Id}}')
chmod -R a+rX "$work"
chmod a+w "$work/out"
date -u +%Y%m%dT%H%M%SZ > "$work/out/stamp.txt"
podman run --rm -v "$work":/w -e ARMS -e BUILDS -e PROBE_COMMIT="$commit" -e DIRTY="$dirty" -e IMAGE_ID="$digest" \
  -e PODMAN_VERSION="$(podman --version)" "$image" sh /w/inside.sh
