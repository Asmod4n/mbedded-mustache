#!/bin/sh
# Container side, phase tests. Builds the test binaries of test/ for every arm with g++-16, four at a time, and runs each as nobody.
# The tests that render through the walk H2b' replaces are spec, limits, media_type and typed (typed renders structs through Table, and maps and vectors of structs through the walk).
# Warnings are counted for each binary. Stderr goes to log files under /w/out/tests.
set -u
. /w/src/bench/probes/h2-switch/flags.sh
mkdir -p /w/out/tests /w/tbin /w/out/testrun
chmod a+rwx /w/out/tests /w/tbin /w/out/testrun
[ -d /w/arms/head ] || sh /w/src/bench/probes/h2-switch/make_arms.sh > /w/out/arms-tests.log 2>&1
TESTS="spec limits media_type typed"
jobs=""
for arm in $(cells_of g++-16); do
  [ "$arm" = twin ] && continue
  for t in $TESTS; do jobs="${jobs}$arm $t\n"; done
done
printf "$jobs" | xargs -P 4 -L 1 sh /w/src/bench/probes/h2-switch/test_one.sh
chmod -R a+rX /w/tbin
: > /w/out/tests/summary.txt
for arm in $(cells_of g++-16); do
  [ "$arm" = twin ] && continue
  for t in $TESTS; do
    b=/w/tbin/$arm.$t
    if [ ! -x "$b" ]; then echo "$arm $t BUILD-FAILED warnings=$(grep -c 'warning:' /w/out/tests/$arm.$t.build.log)" >> /w/out/tests/summary.txt; continue; fi
    (cd /w/out/testrun && runuser -u nobody -- "$b" > /w/out/tests/$arm.$t.run.log 2> /w/out/tests/$arm.$t.run.err < /dev/null)
    code=$?
    echo "$arm $t exit=$code $(grep -E '^\[  (PASSED|FAILED)  \]' /w/out/tests/$arm.$t.run.log | head -2 | tr '\n' ' ') warnings=$(grep -c 'warning:' /w/out/tests/$arm.$t.build.log) build_seconds=$(cat /w/tbin/$arm.$t.seconds)" >> /w/out/tests/summary.txt
  done
done
