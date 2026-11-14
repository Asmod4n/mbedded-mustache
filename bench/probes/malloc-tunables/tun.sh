#!/bin/sh
set -u
T="glibc.malloc.tcache_count=64:glibc.malloc.top_pad=268435456:glibc.malloc.trim_threshold=4294967295:glibc.malloc.mmap_threshold=1073741824:glibc.malloc.arena_max=1"
for round in 1 2 3 4 5 6; do
  for arm in plain tunables noaslr; do
    for n in a b; do
      case $arm in
        plain)    cmd="/b/rl-$n";;
        tunables) cmd="env GLIBC_TUNABLES=$T /b/rl-$n";;
        noaslr)   cmd="setarch -R /b/rl-$n";;
      esac
      v=$($cmd --benchmark_format=json --benchmark_repetitions=5 --benchmark_report_aggregates_only=false --benchmark_min_time=0.05s 2>> /b/tun.err | python3 -c 'import json,sys; print(" ".join(str(round(b["real_time"],1)) for b in json.load(sys.stdin)["benchmarks"] if b["name"].startswith("run_time")))')
      echo "$arm $n $v"
    done
  done
done
