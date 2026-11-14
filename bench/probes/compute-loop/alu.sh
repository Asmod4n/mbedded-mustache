#!/bin/sh
g++-16 -std=c++26 -O2 /arms/alu.cpp -lbenchmark -lpthread -o /b/alu
for r in 1 2 3 4 5 6 7 8 9 10 11 12; do /b/alu --benchmark_format=json --benchmark_repetitions=5 --benchmark_report_aggregates_only=false --benchmark_min_time=0.05s 2>> /b/alu.err | python3 -c 'import json,sys; print(" ".join(str(round(b["real_time"],1)) for b in json.load(sys.stdin)["benchmarks"] if b.get("run_type")=="iteration"))'; done
