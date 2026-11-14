#!/bin/sh
cd /w
for t in base c3 c3b; do
  echo "== $t"
  g++-16 -std=c++26 -freflection -O2 -march=x86-64-v3 -Wall -Wextra -Werror -I $t/include -I $t/test p1.cpp -o p1_$t > p1_$t.log 2>&1 && ./p1_$t || tail -15 p1_$t.log
done
