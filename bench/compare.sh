#!/bin/sh
# Host side: builds two arms of the headers from two commits and runs them in a container.
# Usage: bench/compare.sh OLD_COMMIT NEW_COMMIT [IMAGE]
set -eu
old=$1
new=$2
image=${3:-localhost/mbedded-cbor-docs}
root=$(cd "$(dirname "$0")/.." && pwd)
cd "$root"
dirty=$(git status --porcelain | wc -l)
work=$(mktemp -d)
mkdir "$work/old" "$work/new" "$work/out"
git archive "$old" | tar -x -C "$work/old"
git archive "$new" | tar -x -C "$work/new"
cp bench/compare_in_container.sh bench/summarize.py "$work/"
stamp=$(date -u +%Y%m%dT%H%M%SZ)
digest=$(podman image inspect "$image" --format '{{.Digest}} {{.Id}}')
chmod -R a+rX "$work"
chmod a+w "$work/out"
podman run --rm -v "$work":/w -e OLD="$old" -e NEW="$new" -e SCRIPT_COMMIT="$(git rev-parse HEAD)" -e DIRTY="$dirty" \
  -e IMAGE_ID="$digest" -e PODMAN_VERSION="$(podman --version)" "$image" sh /w/compare_in_container.sh
cp "$work/out/result.txt" "bench/results/compare-$stamp.txt"
echo "bench/results/compare-$stamp.txt"
