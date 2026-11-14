#!/bin/sh
# Makes /w/arms/<arm>/include: a copy of include/ of the base for each arm, with the patches of the arm applied in order by git apply.
# H and Htwin are two copies of the base with no patch.
set -eu
here=$(cd "$(dirname "$0")" && pwd)
patches_of() {
  case $1 in
    H|Htwin) echo "" ;;
    C1) echo "c1-output-start" ;;
    C2p) echo "c2p-buffer-for-overwrite" ;;
    C3) echo "c3-short-values" ;;
    C4) echo "c4-op-range" ;;
    C5) echo "c5-one-visit" ;;
    All) echo "c1-output-start c2p-buffer-for-overwrite c3-short-values c4-op-range c5-one-visit" ;;
  esac
}
for arm in H C1 C2p C3 Htwin C4 C5 All; do
  mkdir -p "/w/arms/$arm"
  cp -r /w/src/include "/w/arms/$arm/include"
  for p in $(patches_of "$arm"); do
    (cd "/w/arms/$arm" && git apply -p1 "$here/$p.patch") || { echo "patch $p on $arm failed"; exit 1; }
  done
done
