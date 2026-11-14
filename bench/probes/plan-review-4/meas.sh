#!/bin/sh
# usage: meas.sh <tag> <rounds> "<compilers>" "<levels>" "<arms>"; arms run alternately, the order turns each round
tag=$1; rounds=$2
d=/w/res/$tag; mkdir -p $d; chmod a+rwx $d
cat > $d/run_user.sh <<F
#!/bin/sh
arms="$5"
rev=\$(echo \$arms | tr ' ' '\n' | tac | tr '\n' ' ')
for round in \$(seq 1 $rounds); do
  for cxx in $3; do for level in $4; do
    if [ \$((round % 2)) = 1 ]; then order="\$arms"; else order="\$rev"; fi
    for a in \$order; do
      /w/b/v-\$cxx\$level-\$a --benchmark_format=json --benchmark_repetitions=3 --benchmark_report_aggregates_only=false --benchmark_min_time=0.05s 2>> $d/run.err > $d/j-\$round-\$cxx\$level-\$a.json
    done
  done; done
done
F
runuser -u nobody -- sh $d/run_user.sh
cat /proc/loadavg > $d/loadavg.txt
