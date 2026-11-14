#!/bin/sh
# Runs as nobody. Checks that every arm renders the same bytes, measures the noise floor with sysbench, and then runs ten
# rounds. Each round starts every executable once as its own process, in a new random order. The executables are the ten
# arms (five arms, two compilers), the heap and cap arms of clang++-23 a second time under setarch -R (no address space
# randomization), and the ramhorns bench. Before and after each process the cpu line of /proc/stat is recorded, so that
# work of other processes during the timing can be seen. EXECUTABLES and REPETITIONS replace the list and the 6 repetitions.
set -u
cd /w/out
echo "user=$(id -un) nice=$(nice) threads=1 not pinned; nobody owns no other process, so the run has the default priority" > run-context.txt
mkdir -p check
for b in /w/bin/*.g++-16 /w/bin/*.clang++-23; do
  case $b in *.flags) continue ;; esac
  n=$(basename "$b")
  "$b" --output_file=check/$n.html 2>> check/err.txt < /dev/null || echo "output of $n failed" >> failed.txt
done
sha256sum check/*.html > check/sha256.txt
setarch x86_64 -R sh -c 'cat /proc/self/personality' > setarch-check.txt 2>&1
for i in 1 2 3 4 5 6 7 8 9 10; do sysbench cpu --threads=1 --time=1 run 2>> sysbench.err | awk '/events per second/ {printf "%s ", $4}'; done > noise.txt
executables=${EXECUTABLES:-"heap.g++-16 cap.g++-16 warm.g++-16 pmr.g++-16 typed.g++-16 heap.clang++-23 cap.clang++-23 warm.clang++-23 pmr.clang++-23 typed.clang++-23 heapNR.clang++-23 capNR.clang++-23 ramhorns"}
repetitions=${REPETITIONS:-6}
echo "executables=$executables repetitions=$repetitions" >> run-context.txt
date -u +%Y-%m-%dT%H:%M:%SZ > timed-start.txt
for round in 1 2 3 4 5 6 7 8 9 10; do
  order=$(printf '%s\n' $executables | shuf | tr '\n' ' ')
  echo "round $round: $order" >> order.txt
  for exe in $order; do
    d=round$round/$exe
    mkdir -p $d
    grep '^cpu ' /proc/stat > $d/stat-before.txt
    date -u +%Y-%m-%dT%H:%M:%S.%NZ > $d/start.txt
    case $exe in
      ramhorns)
        CRITERION_HOME=/w/out/$d/criterion /w/rbin/render --bench w2 --warm-up-time 0.1 --measurement-time 1.5 \
          > $d/console.txt 2> $d/err.txt < /dev/null || echo "round $round $exe exit $?" >> failed.txt ;;
      heapNR.*|capNR.*)
        b=$(echo "$exe" | sed 's/NR//')
        setarch x86_64 -R /w/bin/$b --benchmark_repetitions=$repetitions --benchmark_min_time=0.05s \
          --benchmark_out=/w/out/$d/gb.json --benchmark_out_format=json > $d/console.txt 2> $d/err.txt < /dev/null ||
          echo "round $round $exe exit $?" >> failed.txt ;;
      *)
        /w/bin/$exe --benchmark_repetitions=$repetitions --benchmark_min_time=0.05s \
          --benchmark_out=/w/out/$d/gb.json --benchmark_out_format=json > $d/console.txt 2> $d/err.txt < /dev/null ||
          echo "round $round $exe exit $?" >> failed.txt ;;
    esac
    date -u +%Y-%m-%dT%H:%M:%S.%NZ > $d/end.txt
    grep '^cpu ' /proc/stat > $d/stat-after.txt
  done
done
date -u +%Y-%m-%dT%H:%M:%SZ > timed-end.txt
