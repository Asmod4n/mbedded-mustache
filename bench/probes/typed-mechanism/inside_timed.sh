#!/bin/sh
# Container side, as root, without network. Runs timed_as_user.sh as nobody.
set -u
mkdir -p /w/out/json
chmod a+rwx /w/out /w/out/json
{ echo "image=$IMAGE_ID"; echo "podman_timed=$PODMAN_VERSION"; echo "g++_runtime_check=$(readelf -p .comment /w/bin/R | grep -i GCC | head -1 | sed 's/^ *\[ *[0-9a-f]*\] *//')"; } > /w/out/context-timed.txt
runuser -u nobody -- sh /w/src/bench/probes/typed-mechanism/timed_as_user.sh
