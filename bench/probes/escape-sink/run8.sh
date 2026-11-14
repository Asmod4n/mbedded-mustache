#!/bin/sh
jobs=""
for cxx in g++-16 clang++-23; do for level in -Os -O2; do jobs="$jobs $cxx:$level:7 $cxx:$level:8"; done; done
echo $jobs | tr ' ' '\n' | xargs -P 4 -I{} sh -c '
  cxx=$(echo {} | cut -d: -f1); level=$(echo {} | cut -d: -f2); a=$(echo {} | cut -d: -f3)
  $cxx -std=c++26 $level -march=x86-64-v4 -DARM=$a -I/src/include /w/sink.cpp -lbenchmark -lpthread -o /b/sink/s-$cxx$level-$a > /b/sink/s-$cxx$level-$a.log 2>&1 || { echo "FAIL $cxx $level $a"; head -5 /b/sink/s-$cxx$level-$a.log; }
'
chmod -R a+rx /b/sink; chmod a+rwx /b/sink
cat > /b/sink/run_user8.sh <<'F'
#!/bin/sh
for round in 1 2 3 4 5 6; do
  for cxx in g++-16 clang++-23; do for level in -Os -O2; do
    if [ $((round % 2)) = 1 ]; then order="1 7 8"; else order="8 7 1"; fi
    for a in $order; do
      /b/sink/s-$cxx$level-$a --benchmark_format=json --benchmark_repetitions=3 --benchmark_report_aggregates_only=false --benchmark_min_time=0.05s 2>> /b/sink/run.err > /b/sink/i-$round-$cxx$level-$a.json
    done
  done; done
done
F
runuser -u nobody -- sh /b/sink/run_user8.sh
