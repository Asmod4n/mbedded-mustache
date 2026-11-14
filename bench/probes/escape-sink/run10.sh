#!/bin/sh
mkdir -p /b/sink3
jobs=""
for cxx in g++-16 clang++-23; do for level in -Os -O2; do for a in 1 7; do jobs="$jobs $cxx:$level:$a"; done; done; done
echo $jobs | tr ' ' '\n' | xargs -P 4 -I{} sh -c '
  cxx=$(echo {} | cut -d: -f1); level=$(echo {} | cut -d: -f2); a=$(echo {} | cut -d: -f3)
  case $cxx in g++*) align="-falign-functions=64 -falign-loops=64 -falign-jumps=64";; *) align="-falign-functions=64 -falign-loops=64";; esac
  $cxx -std=c++26 $level -march=x86-64-v4 $align -DARM=$a -I/src/include /w/sink_final.cpp -lbenchmark -lpthread -o /b/sink3/s-$cxx$level-$a > /b/sink3/s-$cxx$level-$a.log 2>&1 || { echo "FAIL $cxx $level $a"; head -3 /b/sink3/s-$cxx$level-$a.log; }
'
chmod -R a+rx /b/sink3; chmod a+rwx /b/sink3
cat > /b/sink3/run_user.sh <<'F'
#!/bin/sh
for round in 1 2 3 4 5 6; do
  for cxx in g++-16 clang++-23; do for level in -Os -O2; do
    if [ $((round % 2)) = 1 ]; then order="1 7"; else order="7 1"; fi
    for a in $order; do
      /b/sink3/s-$cxx$level-$a --benchmark_format=json --benchmark_repetitions=3 --benchmark_report_aggregates_only=false --benchmark_min_time=0.05s 2>> /b/sink3/run.err > /b/sink3/k-$round-$cxx$level-$a.json
    done
  done; done
done
F
runuser -u nobody -- sh /b/sink3/run_user.sh
