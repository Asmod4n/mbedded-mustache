#!/bin/sh
# Host side. Usage: bench/probes/typed-mechanism/run.sh WORK build|timed|compare [IMAGE]
# build: exports include/ and the probe of HEAD, builds the arms with g++-16, checks the bytes against bench/render.cpp workload 2.
# timed: the noise floor and 10 rounds, as nobody, without network.
# compare: merges the rounds and runs tools/compare.py of Google Benchmark for each arm against R.
set -eu
work=$1
phase=$2
image=${3:-localhost/mbedded-cbor-docs}
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../../.." && pwd)
env_args="-e PROBE_COMMIT=$(git -C "$root" rev-parse HEAD) -e DIRTY=$(git -C "$root" status --porcelain | wc -l) -e PODMAN_VERSION=$(podman --version | tr ' ' '_')"
net_args="--network host -e HTTP_PROXY -e HTTPS_PROXY -e http_proxy -e https_proxy -e NO_PROXY -e no_proxy -v /root/.ccr/ca-bundle.crt:/ca.crt:ro -e GIT_SSL_CAINFO=/ca.crt -e SSL_CERT_FILE=/ca.crt"
case $phase in
  build)
    mkdir -p "$work/out" "$work/src"
    git -C "$root" archive HEAD include bench/render.cpp bench/node.hpp bench/probes/typed-mechanism | tar -x -C "$work/src"
    chmod -R a+rX "$work"
    chmod a+w "$work/out"
    podman run --rm $net_args $env_args -v "$work":/w "$image" sh /w/src/bench/probes/typed-mechanism/inside_build.sh ;;
  timed)
    digest=$(podman image inspect "$image" --format '{{.Digest}} {{.Id}}' | tr ' ' '_')
    podman run --rm --network none $env_args -e IMAGE_ID="$digest" -v "$work":/w "$image" sh /w/src/bench/probes/typed-mechanism/inside_timed.sh ;;
  compare)
    podman run --rm $net_args -v "$work":/w "$image" sh /w/src/bench/probes/typed-mechanism/inside_compare.sh ;;
esac
