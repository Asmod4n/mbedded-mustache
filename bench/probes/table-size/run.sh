#!/bin/sh
# Host side: exports include/ of each state with git archive, gives the lambda indent_at of compile.hpp the
# return type size_t where the exported header does not have it yet, and builds probe.cpp for 32-bit ARM in a
# container. Prints one line for each state, form and section, and writes the context to WORK_DIR/out.
# STATES in the environment chooses the commits (default 4dac08e 738ff29 7c3f670 d08686b). A state <commit>+<name>
# is the commit with <name>.patch of this directory applied to the exported include/ (class-tables.patch and
# class-fields.patch: escape.hpp of f331cd1 with the class forms of arms 4 and 5 of the probe scalar-writers).
# Usage: bench/probes/table-size/run.sh WORK_DIR [IMAGE]
set -eu
work=$1
image=${2:-localhost/mbedded-cbor-docs}
here=$(cd "$(dirname "$0")" && pwd)
top=$(git -C "$here" rev-parse --show-toplevel)
states=${STATES:-4dac08e 738ff29 7c3f670 d08686b}
mkdir -p "$work/out"
cp "$here/probe.cpp" "$here/inside.sh" "$work/"
repaired=""
for s in $states; do
  rm -rf "$work/h/$s"
  mkdir -p "$work/h/$s"
  git -C "$top" archive "${s%%+*}" include | tar -x -C "$work/h/$s"
  case $s in *+*) patch -p1 -d "$work/h/$s" < "$here/${s#*+}.patch" > "$work/out/patch-$s.log" 2>&1 || { echo "patch $s failed"; exit 1; };; esac
  f="$work/h/$s/include/mustache/compile.hpp"
  if grep -q '^  auto indent_at = \[&\](const Token &t) {$' "$f"; then
    sed -i 's/^  auto indent_at = \[&\](const Token &t) {$/  auto indent_at = [\&](const Token \&t) -> size_t {/' "$f"
    repaired="$repaired $s"
  fi
done
commit=$(git -C "$here" rev-parse HEAD)
dirty=$(git -C "$here" status --porcelain | wc -l)
digest=$(podman image inspect "$image" --format '{{.Digest}} {{.Id}}')
chmod -R a+rX "$work"
chmod a+w "$work/out"
date -u +%Y%m%dT%H%M%SZ > "$work/out/stamp.txt"
podman run --rm -v "$work":/w -e STATES="$states" -e REPAIRED="$repaired" -e PROBE_COMMIT="$commit" -e DIRTY="$dirty" \
  -e IMAGE_ID="$digest" -e PODMAN_VERSION="$(podman --version)" "$image" sh /w/inside.sh
