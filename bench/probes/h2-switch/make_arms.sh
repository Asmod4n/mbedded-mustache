#!/bin/sh
# Makes /w/arms/<arm>/include: a copy of include/ of the base for each arm, with the patch of the arm applied by git apply.
# head and twin are two copies of the base with no patch.
set -eu
here=$(cd "$(dirname "$0")" && pwd)
patch_of() {
  case $1 in
    head|twin) echo "" ;;
    *) echo "$1" ;;
  esac
}
for arm in head twin h2bp; do
  mkdir -p "/w/arms/$arm"
  cp -r /w/src/include "/w/arms/$arm/include"
  p=$(patch_of "$arm")
  if [ -n "$p" ]; then
    (cd "/w/arms/$arm" && git apply -p1 "$here/$p.patch") || { echo "patch $p on $arm failed"; exit 1; }
  fi
done
