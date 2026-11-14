#!/bin/bash
# Container side, as user 65534. Step build: one build (g++-16, the flags below), the binaries a, aa and b in parallel, the
# byte check. Step measure: 10 rounds; each round runs each binary once in a rotated order (3 repetitions of at least 0.05 s);
# a round counts when its steal is under 3 % of its cpu time. Then tools/compare.py of Google Benchmark and the result file.
set -u
step=$1
src=/w/src/bench/probes/output-targets
out=/w/out
flags="-std=c++26 -freflection -O3 -DNDEBUG -march=x86-64-v4"
cxx=g++-16
rounds=10
repetitions=3
min_time=0.05s
steal_limit_percent=3
attempts_per_round=2
arms=(a aa b)

build() {
  mkdir -p "$out/bin" "$out/log" "$out/check"
  : > "$out/failed.txt"
  for arm in "${arms[@]}"; do
    define=""
    [ "$arm" = b ] && define="-DARM_REUSED"
    { $cxx $flags $define -I/w/src/include -I/w/src/bench "$src/output_targets.cpp" -lbenchmark -lpthread -o "$out/bin/$arm" \
        > "$out/log/$arm" 2>&1 || echo "build $arm" >> "$out/failed.txt"; } &
  done
  wait
  for arm in "${arms[@]}"; do
    "$out/bin/$arm" --output_file="$out/check/$arm.html" 2>> "$out/log/check" || echo "output $arm" >> "$out/failed.txt"
  done
  for arm in "${arms[@]}"; do
    echo "$arm $(readelf -h "$out/bin/$arm" | grep -o 'Type: *[A-Z]*' | tr -s ' ')"
  done > "$out/elf_type"
  cat "$out/failed.txt"
}

read_steal() {
  local user nice system idle iowait irq softirq stolen rest
  read -r _ user nice system idle iowait irq softirq stolen rest < /proc/stat
  echo "$stolen $((user + nice + system + idle + iowait + irq + softirq + stolen))"
}

measure() {
  if [ "$(nice -n -10 nice 2>> "$out/nice.log")" = -10 ]; then
    echo "nice -10 granted" > "$out/priority"; runner="nice -n -10"
  else
    echo "nice 0 (nice -10 refused: $(tr '\n' ' ' < "$out/nice.log"))" > "$out/priority"; runner=""
  fi
  : > "$out/rounds"
  : > "$out/err.txt"
  local i=1 attempt=0 k s0 s1 t0 t1 try
  while [ $i -le $rounds ] && [ $attempt -lt $((attempts_per_round * rounds)) ]; do
    attempt=$((attempt + 1))
    try=$out/try$attempt
    mkdir -p "$try"
    s0=$(read_steal); t0=$EPOCHREALTIME
    for k in 0 1 2; do
      arm=${arms[$(((k + attempt) % 3))]}
      $runner "$out/bin/$arm" --benchmark_repetitions=$repetitions --benchmark_min_time=$min_time \
        --benchmark_out_format=json --benchmark_out="$try/$arm.json" < /dev/null > /dev/null 2>> "$out/err.txt" ||
        echo "run $arm" >> "$out/failed.txt"
    done
    s1=$(read_steal); t1=$EPOCHREALTIME
    set -- $s0 $s1
    if [ $((100 * ($3 - $1))) -lt $((steal_limit_percent * ($4 - $2 > 0 ? $4 - $2 : 1))) ]; then
      mv "$try" "$out/round$i"; echo "$i $attempt $t0 $t1 $s0 $s1 kept" >> "$out/rounds"; i=$((i + 1))
    else
      echo "$i $attempt $t0 $t1 $s0 $s1 discarded" >> "$out/rounds"
    fi
  done
  mkdir -p "$out/merged" "$out/compare"
  for arm in "${arms[@]}"; do
    jq -s '{context: .[0].context, benchmarks: [.[].benchmarks[] | select(.run_type == "iteration")]}' "$out"/round*/"$arm".json \
      > "$out/merged/$arm.json"
  done
  for arm in aa b; do
    python3 /opt/gb/tools/compare.py --no-color -d "$out/compare/$arm.json" benchmarks "$out/merged/a.json" "$out/merged/$arm.json" \
      > "$out/compare/$arm.txt" 2> "$out/compare/$arm.err" || echo "compare $arm" >> "$out/failed.txt"
  done
  date -u +%Y%m%dT%H%M%SZ > "$out/stamp"
  dpkg-query -W -f '${Package} ${Version}\n' g++-16 libstdc++6 libc6 libbenchmark1.9.1 > "$out/packages" 2>&1
  same_check=$(cmp -s "$out/check/a.html" "$out/check/b.html" && cmp -s "$out/check/a.html" "$out/check/aa.html" && echo true || echo false)
  libs() { ldd "$1" | awk '$1 ~ /^(libstdc\+\+|libc\.so|libm|libbenchmark)/ && $2 == "=>" {print $1, $3}' |
    while read -r n p; do echo "$n $(readlink -f "$p")"; done | jq -R -s 'split("\n") | map(select(length > 0) | split(" ") | {(.[0]): .[1]}) | add'; }
  jq -n \
    --arg time "$(cat "$out/stamp")" --arg commit "$(cat "$out/commit")" --arg image "$(cat "$out/image")" \
    --arg flags "$cxx $flags" --arg min_time "$min_time" --argjson rounds $rounds --argjson repetitions $repetitions \
    --arg compiler "$($cxx --version | head -n 1)" --arg comment "$(readelf -p .comment "$out/bin/a")" --argjson libraries "$(libs "$out/bin/a")" \
    --arg libc "$(ldd --version | head -n 1)" --arg model "$(grep -m1 'model name' /proc/cpuinfo | cut -d: -f2- | sed 's/^ //')" \
    --arg flags_sha256 "$(grep -m1 '^flags' /proc/cpuinfo | cut -d: -f2- | sed 's/^ //' | tr -d '\n' | sha256sum | cut -d' ' -f1)" \
    --argjson cpus "$(nproc)" --arg l3 "$(cat /sys/devices/system/cpu/cpu0/cache/index3/size)" \
    --argjson uid "$(id -u)" --arg priority "$(cat "$out/priority")" --argjson steal_limit $steal_limit_percent \
    --argjson same_check "$same_check" --arg resolved_march "$($cxx $flags -Q --help=target | awk '$1 == "-march=" {print $2}')" \
    --rawfile host "$out/host" --rawfile elf "$out/elf_type" --rawfile gate_build "$out/gate-before-build" \
    --rawfile gate_runs "$out/gate-before-runs" --rawfile rounds_text "$out/rounds" --rawfile failed "$out/failed.txt" \
    --rawfile err "$out/err.txt" --rawfile packages "$out/packages" \
    --slurpfile ma "$out/merged/a.json" --slurpfile maa "$out/merged/aa.json" --slurpfile mb "$out/merged/b.json" \
    --slurpfile caa "$out/compare/aa.json" --slurpfile cb "$out/compare/b.json" '
    def lines: split("\n") | map(select(length > 0));
    def median: sort | if length % 2 == 1 then .[length / 2 | floor] else (.[length / 2 - 1] + .[length / 2]) / 2 end;
    def arm: [.benchmarks[].real_time] as $t | {repetitions_ns: $t, median_ns: ($t | median), spread: (($t | max) / ($t | min) - 1)};
    ($host | lines | map(capture("^(?<k>[^=]+)=(?<v>.*)$") | {(.k): .v}) | add) as $h
    | ($ma[0] | arm) as $a | ($maa[0] | arm) as $aa | ($mb[0] | arm) as $b
    | ($aa.median_ns / $a.median_ns) as $aar | ($b.median_ns / $a.median_ns) as $bar
    | ($rounds_text | lines | map(split(" ") | {round: (.[0] | tonumber), attempt: (.[1] | tonumber),
        wall_s: ((.[3] | tonumber) - (.[2] | tonumber)), kept: (.[8] == "kept"),
        steal_percent_of_cpu_time: (100 * ((.[6] | tonumber) - (.[4] | tonumber)) / ([1, (.[7] | tonumber) - (.[5] | tonumber)] | max))})) as $rd
    | {
      time: $time,
      commit_a: {given: "HEAD", id: $commit},
      commit_b: {given: "HEAD", id: $commit},
      bench_sources_commit: $commit,
      what: "the stocks page of mbosecke/template-benchmark, rendered at run time from std::unordered_map data; a = mustache::render into a new std::string, aa = a built again, b = mustache::render_to into a kept std::string",
      documents: "stocks",
      flags: $flags,
      pie: "default of the compiler",
      elf_type_count: ($elf | lines | map(sub("^[^ ]+ "; "")) | group_by(.) | map({(.[0]): length}) | add),
      rounds: $rounds,
      repetitions_per_process: $repetitions,
      min_time: $min_time,
      gxx_binary: {binary: "a", comment: $comment, libraries: $libraries},
      resolved_march: $resolved_march,
      compilers: [$compiler],
      libc: $libc,
      level_two_cache: $h.level_two,
      kernel: $h.kernel,
      scheduler: $h.scheduler,
      virtualization: $h.virtualization,
      host_in_container: $h.host_container,
      host_user: $h.host_user,
      container: ("podman " + env.IMAGE + " " + $image + ", user 65534, network none"),
      cpu: {model: $model, flags_sha256: $flags_sha256, cpus: $cpus, l3: $l3},
      uid: $uid,
      priority: $priority,
      threads: 1,
      pinned: false,
      cpu_left_to_the_machine: ("each bench process is single-threaded on " + $h.cpus + " cpus"),
      gate_before_build: ($gate_build | lines),
      gate_before_runs: ($gate_runs | lines),
      steal_limit_percent: $steal_limit,
      rounds_detail: $rd,
      failed: ($failed | lines),
      stderr_of_runs: ($err | .[0:4000]),
      packages: ($packages | lines),
      noise_floor: {a_over_a_max_deviation_percent: (100 * (($aar - 1) | fabs))},
      steal_percent_of_cpu_time_max: ($rd | map(.steal_percent_of_cpu_time) | max),
      cells: {stocks: {a: $a, aa: $aa, b: $b, a_over_a_ratio: $aar, b_over_a_ratio: $bar, same_check: $same_check,
                       difference_over_noise: ((($bar - 1) | fabs) > (($aar - 1) | fabs))}},
      compare_against_a: {aa: $caa[0], b: $cb[0]}
    }' > "$out/result.json" || echo "result" >> "$out/failed.txt"
  jq -r '"noise floor (A/A): \(.noise_floor.a_over_a_max_deviation_percent * 100 | round / 100) %",
    "stocks: b/a \(.cells.stocks.b_over_a_ratio * 1000 | round / 1000), aa/a \(.cells.stocks.a_over_a_ratio * 1000 | round / 1000), over noise \(.cells.stocks.difference_over_noise), same bytes \(.cells.stocks.same_check)",
    "rounds kept \([.rounds_detail[] | select(.kept)] | length) of \(.rounds_detail | length), steal max \(.steal_percent_of_cpu_time_max * 100 | round / 100) %"' "$out/result.json"
  for arm in aa b; do echo "== compare.py $arm against a"; grep -E "pvalue|_median" "$out/compare/$arm.txt"; done
  cat "$out/failed.txt"
}

$step
