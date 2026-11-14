#!/bin/sh
# Host side. Usage: bench/probes/engine-work/run.sh WORK build|tests|timed|compare [IMAGE]
# build: exports include/, test/, bench/, the spec files and the probe of HEAD, builds every binary, checks the bytes.
# tests: builds and runs the test binaries with both compilers, as nobody.
# timed: the noise floor and 10 rounds, as nobody. PILOT=1 shortens it; a pilot result is not a result.
# compare: merges the rounds and runs tools/compare.py of Google Benchmark for each arm against main.
# SRC_TREE=1 exports the files of the working tree instead of HEAD, for a trial; the result of a trial is not a result.
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
    if [ "${SRC_TREE:-0}" = 1 ]; then
      mkdir -p "$work/src/refs" "$work/src/bench/probes"
      cp -r "$root/include" "$root/test" "$work/src/"
      cp -r "$root/refs/mustache-spec" "$work/src/refs/"
      cp "$root/bench/render.cpp" "$root/bench/node.hpp" "$work/src/bench/"
      cp -r "$here" "$work/src/bench/probes/"
    else
      git -C "$root" archive HEAD include test refs/mustache-spec bench/render.cpp bench/node.hpp bench/probes/engine-work | tar -x -C "$work/src"
    fi
    chmod -R a+rX "$work"
    podman run --rm --network none -v "$work":/w "$image" sh /w/src/bench/probes/engine-work/inside_build.sh ;;
  tests)
    podman run --rm --network none -v "$work":/w "$image" sh /w/src/bench/probes/engine-work/inside_tests.sh ;;
  timed)
    digest=$(podman image inspect "$image" --format '{{.Digest}} {{.Id}}' | tr ' ' '_')
    podman run --rm --network none $env_args -e IMAGE_ID="$digest" -e PILOT="${PILOT:-0}" -v "$work":/w "$image" sh /w/src/bench/probes/engine-work/inside_timed.sh ;;
  compare)
    podman run --rm --network host \
      -e HTTP_PROXY -e HTTPS_PROXY -e http_proxy -e https_proxy -e NO_PROXY -e no_proxy \
      -v /root/.ccr/ca-bundle.crt:/ca.crt:ro -e GIT_SSL_CAINFO=/ca.crt -e SSL_CERT_FILE=/ca.crt \
      -v "$work":/w "$image" sh /w/src/bench/probes/engine-work/inside_compare.sh ;;
esac
