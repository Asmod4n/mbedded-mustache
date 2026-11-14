#!/bin/sh
# Builds one test binary. Usage: test_one.sh ARM TEST. The binary is /w/tbin/ARM.TEST, with g++-16 over the headers of the arm.
arm=$1
t=$2
start=$(date +%s.%N)
g++-16 -std=c++26 -freflection -O1 -march=x86-64-v4 -Wall -Wextra -DMUSTACHE_SPEC_DIR='"/w/src/refs/mustache-spec"' \
  -I/w/arms/$arm/include /w/src/test/$t.cpp -lsimdjson -lgtest -lgtest_main -lpthread -o /w/tbin/$arm.$t > /w/out/tests/$arm.$t.build.log 2>&1
end=$(date +%s.%N)
echo "$end $start" | awk '{printf "%.1f\n", $1 - $2}' > /w/tbin/$arm.$t.seconds
