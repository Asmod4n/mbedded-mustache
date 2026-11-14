#!/bin/sh
# Container side, phase tests. Builds the test binaries spec and typed of test/ for the variant e3a with g++-16 only, four at a time,
# and runs each as nobody. Warnings are counted for each binary. Stderr goes to log files under /w/out/tests.
set -u
. /w/src/bench/probes/engine-work/flags.sh
mkdir -p /w/out/tests /w/tbin /w/out/testrun
chmod a+rwx /w/out/tests /w/tbin /w/out/testrun
[ -d /w/arms/main ] || sh /w/src/bench/probes/engine-work/make_arms.sh > /w/out/arms-tests.log 2>&1
GCC_TESTS="spec typed"
CLANG_TESTS=""
TEST_VARIANTS="e3a"
TEST_COMPILERS="g++-16"
jobs=""
for v in $TEST_VARIANTS; do
  for t in $GCC_TESTS; do jobs="${jobs}$v $t g++-16\n"; done
  for t in $CLANG_TESTS; do jobs="${jobs}$v $t clang++-23\n"; done
done
printf "$jobs" | xargs -P 4 -L 1 sh /w/src/bench/probes/engine-work/test_one.sh
chmod -R a+rX /w/tbin
: > /w/out/tests/summary.txt
for v in $TEST_VARIANTS; do
  for cxx in $TEST_COMPILERS; do
    case $cxx in g++-16) tests=$GCC_TESTS ;; *) tests=$CLANG_TESTS ;; esac
    for t in $tests; do
      b=/w/tbin/$v.$t.$cxx
      if [ ! -x "$b" ]; then echo "$v $t $cxx BUILD-FAILED warnings=$(grep -c 'warning:' /w/out/tests/$v.$t.$cxx.build.log)" >> /w/out/tests/summary.txt; continue; fi
      (cd /w/out/testrun && runuser -u nobody -- "$b" > /w/out/tests/$v.$t.$cxx.run.log 2> /w/out/tests/$v.$t.$cxx.run.err < /dev/null)
      code=$?
      echo "$v $t $cxx exit=$code $(grep -E '^\[  (PASSED|FAILED)  \]' /w/out/tests/$v.$t.$cxx.run.log | head -2 | tr '\n' ' ') warnings=$(grep -c 'warning:' /w/out/tests/$v.$t.$cxx.build.log) build_seconds=$(cat /w/tbin/$v.$t.$cxx.seconds)" >> /w/out/tests/summary.txt
    done
  done
done
