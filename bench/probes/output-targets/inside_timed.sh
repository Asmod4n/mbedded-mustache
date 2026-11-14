#!/bin/sh
# Container side, phase timed. Refuses to start when the check found a difference. It records the load average and does not wait for it.
# Runs as nobody: the noise floor of the machine, then 10 rounds of the A/A pair alone, then 10 rounds of every arm.
# Each round runs every binary once, 30 repetitions of at least 0.05 s, in a new random order.
set -u
. /w/src/bench/probes/output-targets/flags.sh
cd /w/out
if [ "$(cat check-diffs.txt)" != 0 ]; then echo "the check found a difference" > refused.txt; exit 2; fi
chmod a+rwx /w/out
mkdir -p json json-noise
chmod a+rwx json json-noise
chmod a+w failed.txt
runuser -u nobody -- env PROBE_COMMIT="$PROBE_COMMIT" DIRTY="$DIRTY" IMAGE_ID="$IMAGE_ID" PODMAN_VERSION="$PODMAN_VERSION" PILOT="${PILOT:-0}" \
  sh /w/src/bench/probes/output-targets/timed_as_user.sh
