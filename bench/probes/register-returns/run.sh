#!/bin/sh
# Host side. Usage: bench/probes/register-returns/run.sh WORK build|tests|timed|compare [IMAGE]
# build: exports include/, test/, the spec files and the probe of HEAD and bench/render.cpp of 3e49a1c, builds every binary, checks the bytes.
# tests: builds and runs the test binaries of every arm with g++-16, as nobody.
# timed: the noise floor and 5 rounds, as nobody. PILOT=1 shortens it; a pilot result is not a result. It starts only when the load average is under 0.5.
# compare: merges the rounds and runs tools/compare.py of Google Benchmark for each arm against head.
set -eu
work=$1
phase=$2
image=${3:-localhost/mbedded-cbor-docs}
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../../.." && pwd)
env_args="-e PROBE_COMMIT=$(git -C "$root" rev-parse HEAD) -e DIRTY=$(git -C "$root" status --porcelain | wc -l) -e PODMAN_VERSION=$(podman --version | tr ' ' '_')"
case $phase in
  build)
    mkdir -p "$work/out" "$work/src" "$work/ref/bench" "$work/ref/test"
    git -C "$root" archive HEAD include test refs/mustache-spec bench/probes/register-returns | tar -x -C "$work/src"
    git -C "$root" show 3e49a1c:bench/render.cpp > "$work/ref/bench/render.cpp"
    cp "$here/node.hpp" "$work/ref/test/node.hpp"
    chmod -R a+rX "$work"
    podman run --rm --network none -v "$work":/w "$image" sh /w/src/bench/probes/register-returns/inside_build.sh ;;
  tests)
    podman run --rm --network none -v "$work":/w "$image" sh /w/src/bench/probes/register-returns/inside_tests.sh ;;
  timed)
    digest=$(podman image inspect "$image" --format '{{.Digest}} {{.Id}}' | tr ' ' '_')
    podman run --rm --network none $env_args -e IMAGE_ID="$digest" -e PILOT="${PILOT:-0}" -v "$work":/w "$image" sh /w/src/bench/probes/register-returns/inside_timed.sh ;;
  compare)
    podman run --rm --network host \
      -e HTTP_PROXY -e HTTPS_PROXY -e http_proxy -e https_proxy -e NO_PROXY -e no_proxy \
      -v /root/.ccr/ca-bundle.crt:/ca.crt:ro -e GIT_SSL_CAINFO=/ca.crt -e SSL_CERT_FILE=/ca.crt \
      -v "$work":/w "$image" sh /w/src/bench/probes/register-returns/inside_compare.sh ;;
esac
