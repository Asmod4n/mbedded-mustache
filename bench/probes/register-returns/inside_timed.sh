#!/bin/sh
# Container side, phase timed. Refuses to start when the load average is 0.5 or more or the check found a difference.
# Measures the noise floor, then runs 5 rounds. Each round runs every binary once in a new random order.
set -u
. /w/src/bench/probes/register-returns/flags.sh
cd /w/out
if [ "$(cat check-diffs.txt)" != 0 ]; then echo "the check found a difference" > refused.txt; exit 2; fi
if [ "${PILOT:-0}" != 1 ] && ! awk '{exit !($1 < 0.5)}' /proc/loadavg; then cp /proc/loadavg refused.txt; exit 3; fi
chmod a+rwx /w/out
mkdir -p json
chmod a+rwx json
chmod a+w failed.txt
runuser -u nobody -- env PROBE_COMMIT="$PROBE_COMMIT" DIRTY="$DIRTY" IMAGE_ID="$IMAGE_ID" PILOT="${PILOT:-0}" PODMAN_VERSION="$PODMAN_VERSION" \
  sh /w/src/bench/probes/register-returns/timed_as_user.sh
