#!/bin/sh
# Builds one test binary. Usage: test_one.sh VARIANT TEST COMPILER. The binary is /w/tbin/VARIANT.TEST.COMPILER, over the include of the
# variant. g++-16 has reflection; clang++-23 builds typed over Boost.PFR.
v=$1
t=$2
cxx=$3
case $cxx in
  g++-16) refl=-freflection ;;
  *) refl= ;;
esac
start=$(date +%s.%N)
$cxx -std=c++26 $refl -O1 -march=x86-64-v4 -Wall -Wextra -DMUSTACHE_SPEC_DIR='"/w/src/refs/mustache-spec"' \
  -I/w/arms/$v/include /w/src/test/$t.cpp -lsimdjson -lgtest -lgtest_main -lpthread -o /w/tbin/$v.$t.$cxx > /w/out/tests/$v.$t.$cxx.build.log 2>&1
end=$(date +%s.%N)
echo "$end $start" | awk '{printf "%.1f\n", $1 - $2}' > /w/tbin/$v.$t.$cxx.seconds
