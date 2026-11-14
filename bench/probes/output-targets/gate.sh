#!/bin/bash
# The gate of mbedded-cbor bench/gate.sh: what runs now, from vmstat and ps --sort=-pcpu. The first row of vmstat (the means
# since boot) is not read. The load average is not read. Exit status 3 when the machine is busy.
GATE_MAX_BUSY_PERCENT=${GATE_MAX_BUSY_PERCENT:-10}
GATE_MAX_STEAL_PERCENT=${GATE_MAX_STEAL_PERCENT:-0}
GATE_SECONDS=${GATE_SECONDS:-5}
GATE_TOP_PROCESSES=7
GATE_BUSY_PROCESS_PERCENT=5

gate() {
  local vm ps_out
  vm=$(vmstat 1 "$GATE_SECONDS")
  ps_out=$(ps -eo pid,user,pcpu,ni,comm --sort=-pcpu | head -n $((GATE_TOP_PROCESSES + 1)))
  printf 'gate: vmstat 1 %s\n%s\ngate: ps --sort=-pcpu (top %s)\n%s\n' "$GATE_SECONDS" "$vm" "$GATE_TOP_PROCESSES" "$ps_out"
  printf '%s\n' "$ps_out" | awk -v limit="$GATE_BUSY_PROCESS_PERCENT" 'NR > 1 && $3 >= limit && $5 != "ps" && $5 != "head" && $5 != "vmstat" {
    printf "gate: busy process %s user %s pcpu %s ni %s %s\n", $1, $2, $3, $4, $5 }'
  printf '%s\n' "$vm" | awk -v busy_limit="$GATE_MAX_BUSY_PERCENT" -v steal_limit="$GATE_MAX_STEAL_PERCENT" '
    NR == 2 { for (i = 1; i <= NF; i++) col[$i] = i }
    NR >= 4 { n++; idle += $col["id"]; wa += $col["wa"]; if ("st" in col) st += $col["st"] }
    END { busy = 100 - idle / n; steal = st / n
          ok = busy <= busy_limit && steal <= steal_limit
          printf "gate: busy %.1f%% of all cpus (limit %s%%), steal %.1f%%, iowait %.1f%%: %s\n", busy, busy_limit, steal, wa / n, ok ? "idle enough" : "NOT idle enough"
          exit ok ? 0 : 3 }'
}
