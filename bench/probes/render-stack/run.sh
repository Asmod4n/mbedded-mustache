#!/bin/sh
g++-16 -std=c++26 -freflection -O2 -march=x86-64-v4 -falign-functions=64 -falign-loops=64 -falign-jumps=64 -DNDEBUG -DWORKLOAD=1 -I/src/include /w/render_stack.cpp -lbenchmark -lpthread -o /b/stk > /b/stk.log 2>&1 || { head -20 /b/stk.log; exit 1; }
for r in 1 2 3 4 5 6 7 8 9 10 11 12; do /b/stk --benchmark_format=json --benchmark_repetitions=5 --benchmark_report_aggregates_only=false --benchmark_min_time=0.05s 2>> /b/stk.err | python3 -c '
import json,sys
d={}
for b in json.load(sys.stdin)["benchmarks"]:
    if b.get("run_type")=="iteration": d.setdefault(b["name"].split("/")[0],[]).append(round(b["real_time"],1))
for k,v in d.items(): print(k," ".join(map(str,v)))'; done
