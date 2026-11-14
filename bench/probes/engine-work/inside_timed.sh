#!/bin/sh
# Container side, phase timed. Refuses to start when the check found a difference. Runs the noise floor and the rounds as nobody.
set -u
. /w/src/bench/probes/engine-work/flags.sh
cd /w/out
if [ "$(cat check-diffs.txt)" != 0 ]; then echo "the check found a difference" > refused.txt; exit 2; fi
chmod a+rwx /w/out
mkdir -p json
chmod a+rwx json
chmod a+w failed.txt
runuser -u nobody -- env PROBE_COMMIT="$PROBE_COMMIT" DIRTY="$DIRTY" IMAGE_ID="$IMAGE_ID" PILOT="${PILOT:-0}" PODMAN_VERSION="$PODMAN_VERSION" \
  sh /w/src/bench/probes/engine-work/timed_as_user.sh
