#!/bin/sh
# Makes /w/arms/<variant>/include: a copy of include/ of the base for each variant, with the patch of the variant applied by git apply.
# main and twin are two copies of the base with no patch.
set -eu
here=$(cd "$(dirname "$0")" && pwd)
. "$here/flags.sh"
for v in $VARIANTS; do
  mkdir -p "/w/arms/$v"
  cp -r /w/src/include "/w/arms/$v/include"
  case $v in
    main|twin) ;;
    *) (cd "/w/arms/$v" && git apply -p1 "$here/$v.patch") || { echo "patch $v failed"; exit 1; } ;;
  esac
done
