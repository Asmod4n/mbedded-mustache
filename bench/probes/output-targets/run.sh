#!/bin/sh
# Host side. Usage: bench/probes/output-targets/run.sh WORK build|timed|compare [IMAGE]
# build: exports include/, bench/node.hpp and this probe of HEAD, builds every binary, checks the bytes.
# timed: the noise floor (sysbench and the A/A pair alone), then 10 rounds of every arm, as nobody. PILOT=1 shortens it; a pilot result is not a result.
# compare: merges the rounds and runs tools/compare.py of Google Benchmark for twin and reused against fresh.
set -eu
work=$1
phase=$2
image=${3:-localhost/mbedded-cbor-docs}
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../../.." && pwd)
env_args="-e PROBE_COMMIT=$(git -C "$root" rev-parse HEAD) -e DIRTY=$(git -C "$root" status --porcelain | wc -l) -e PODMAN_VERSION=$(podman --version | tr ' ' '_')"
case $phase in
  build)
    mkdir -p "$work/out" "$work/src"
    git -C "$root" archive HEAD include bench/node.hpp bench/probes/output-targets | tar -x -C "$work/src"
    chmod -R a+rX "$work"
    podman run --rm --network none -v "$work":/w "$image" sh /w/src/bench/probes/output-targets/inside_build.sh ;;
  timed)
    digest=$(podman image inspect "$image" --format '{{.Digest}} {{.Id}}' | tr ' ' '_')
    podman run --rm --network none $env_args -e IMAGE_ID="$digest" -e PILOT="${PILOT:-0}" -v "$work":/w "$image" sh /w/src/bench/probes/output-targets/inside_timed.sh ;;
  compare)
    podman run --rm --network host \
      -e HTTP_PROXY -e HTTPS_PROXY -e http_proxy -e https_proxy -e NO_PROXY -e no_proxy \
      -v /root/.ccr/ca-bundle.crt:/ca.crt:ro -e GIT_SSL_CAINFO=/ca.crt -e SSL_CERT_FILE=/ca.crt \
      -v "$work":/w "$image" sh /w/src/bench/probes/output-targets/inside_compare.sh ;;
esac
