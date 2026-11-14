#!/bin/sh
set -u
mkdir -p /b/sink
jobs=""
for cxx in g++-16 clang++-23; do for level in -Os -O2; do for a in 1 2 3 4 5; do jobs="$jobs $cxx:$level:$a"; done; done; done
echo $jobs | tr ' ' '\n' | xargs -P 4 -I{} sh -c '
  cxx=$(echo {} | cut -d: -f1); level=$(echo {} | cut -d: -f2); a=$(echo {} | cut -d: -f3)
  case $cxx in g++*) std="-std=c++26";; *) std="-std=c++26";; esac
  $cxx $std $level -march=x86-64-v4 -DARM=$a -I/src/include /w/sink.cpp -lbenchmark -lpthread -o /b/sink/s-$cxx$level-$a > /b/sink/s-$cxx$level-$a.log 2>&1 || echo "FAIL $cxx $level $a: $(grep -m1 error /b/sink/s-$cxx$level-$a.log)"
'
chmod -R a+rx /b/sink
cat > /b/sink/run_user.sh <<'F'
#!/bin/sh
for round in 1 2 3 4 5 6; do
  for cxx in g++-16 clang++-23; do for level in -Os -O2; do
    if [ $((round % 2)) = 1 ]; then order="1 2 3 4 5"; else order="5 4 3 2 1"; fi
    for a in $order; do
      /b/sink/s-$cxx$level-$a --benchmark_format=json --benchmark_repetitions=3 --benchmark_report_aggregates_only=false --benchmark_min_time=0.05s 2>> /b/sink/run.err > /b/sink/out-$round-$cxx$level-$a.json
    done
  done; done
done
F
runuser -u nobody -- sh /b/sink/run_user.sh
