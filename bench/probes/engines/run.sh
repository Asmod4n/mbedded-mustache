#!/bin/sh
# Host side: exports include/, test/, bench/render.cpp and bench/probes/engines of HEAD and runs inside.sh in a container with network access.
# Usage: bench/probes/engines/run.sh WORK_DIR [IMAGE]
# The environment variable PILOT=1 shortens every measurement; a pilot result is not a result.
set -eu
work=$1
image=${2:-localhost/mbedded-cbor-docs}
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../../.." && pwd)
mkdir -p "$work/out" "$work/src"
git -C "$root" archive HEAD include test bench/render.cpp bench/node.hpp bench/probes/engines | tar -x -C "$work/src"
cp "$here/inside.sh" "$here/build_one.sh" "$here/flags.sh" "$here/run_as_user.sh" "$work/"
commit=$(git -C "$root" rev-parse HEAD)
dirty=$(git -C "$root" status --porcelain | wc -l)
digest=$(podman image inspect "$image" --format '{{.Digest}} {{.Id}}')
chmod -R a+rX "$work"
chmod a+w "$work/out"
date -u +%Y%m%dT%H%M%SZ > "$work/out/stamp.txt"
podman run --rm --network host \
  -e HTTP_PROXY -e HTTPS_PROXY -e http_proxy -e https_proxy -e NO_PROXY -e no_proxy \
  -v /root/.ccr/ca-bundle.crt:/ca.crt:ro \
  -e CARGO_HTTP_CAINFO=/ca.crt -e GIT_SSL_CAINFO=/ca.crt -e SSL_CERT_FILE=/ca.crt \
  -e PROBE_COMMIT="$commit" -e DIRTY="$dirty" -e IMAGE_ID="$digest" -e PILOT="${PILOT:-0}" \
  -e PODMAN_VERSION="$(podman --version)" \
  -v "$work":/w "$image" sh /w/inside.sh
