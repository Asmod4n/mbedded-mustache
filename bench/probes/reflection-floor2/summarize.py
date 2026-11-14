"""Summary of one reflection-floor run. Usage: python3 -I summarize.py OUT_DIR

For each arm: the median of the 10 process medians (each process: the median of 30 Google Benchmark repetitions, real time,
ns per render), the spread (max/min - 1) and cv of the process medians, and the median cv within a process. Then the A/A of
R against R2, the noise floor from sysbench, and R minus (C + S2), with C a constant from another run.
"""
import glob
import json
import os
import re
import statistics
import sys

out = sys.argv[1]
TICKS_PER_SECOND = 100
# Median ns of the copy of the 7737 output bytes, arm C of bench/results/reflection-floor-20261114T165119Z. It is not measured in this run.
COPY_NS_FROM_ANOTHER_RUN = 68.0


def spread(xs):
    return (max(xs) / min(xs) - 1) * 100


def cv(xs):
    return statistics.stdev(xs) / statistics.mean(xs) * 100


def busy(line):
    f = [int(x) for x in line.split()[1:]]
    return sum(f[:8]) - f[3] - f[4]


def seconds(stamp):
    m = re.match(r"\d+-\d+-\d+T(\d+):(\d+):(\d+(?:\.\d+)?)Z", stamp)
    return int(m.group(1)) * 3600 + int(m.group(2)) * 60 + float(m.group(3))


rows = {}
for d in sorted(glob.glob(os.path.join(out, "round*", "*"))):
    exe = os.path.basename(d)
    g = json.load(open(os.path.join(d, "gb.json")))
    reps = [b["real_time"] for b in g["benchmarks"] if b.get("run_type") == "iteration"]
    wall = seconds(open(os.path.join(d, "end.txt")).read().strip()) - seconds(open(os.path.join(d, "start.txt")).read().strip())
    other = (busy(open(os.path.join(d, "stat-after.txt")).read()) - busy(open(os.path.join(d, "stat-before.txt")).read())) / TICKS_PER_SECOND / wall - 1
    rows.setdefault(exe, []).append({"median": statistics.median(reps), "cv": cv(reps), "n": len(reps), "other": other})

noise = [float(x) for x in open(os.path.join(out, "noise.txt")).read().split()]
print("Stocks page, reflection render and its floors, g++-16 -O2, real time in ns per operation.")
print("Each process: the median of its 30 repetitions. Each arm: 10 processes, a new random order of the processes per round.")
print()
print("| arm | median of 10 process medians ns | min ns | max ns | between spread % | between cv % | within cv median % | other cpus max |")
print("|---|---|---|---|---|---|---|---|")
med = {}
for exe in ["R", "R2", "S2"]:
    r = rows[exe]
    m = [x["median"] for x in r]
    med[exe] = statistics.median(m)
    print(f"| {exe} | {med[exe]:.1f} | {min(m):.1f} | {max(m):.1f} | {spread(m):.1f} | {cv(m):.1f} | "
          f"{statistics.median([x['cv'] for x in r]):.1f} | {max(x['other'] for x in r):.2f} |")
print()
print(f"repetitions per process: {sorted(set(x['n'] for r in rows.values() for x in r))}")
print(f"noise floor, sysbench cpu events/s, 10 runs of 1 s: min {min(noise):.1f} max {max(noise):.1f} spread {spread(noise):.2f} %")
print(f"A/A, R2 against R: {(med['R2'] / med['R'] - 1) * 100:+.2f} % (medians of the process medians)")
print()
floor = COPY_NS_FROM_ANOTHER_RUN + med["S2"]
print(f"C = {COPY_NS_FROM_ANOTHER_RUN:.1f} ns, from another run; S2 = {med['S2']:.1f} ns, C + S2 = {floor:.1f} ns")
print(f"R - (C + S2) = {med['R'] - floor:.1f} ns ({(med['R'] - floor) / med['R'] * 100:.1f} % of R)")
print(f"R / S2 = {med['R'] / med['S2']:.2f}, R / (C + S2) = {med['R'] / floor:.2f}")
print()
print("Per process: round, median ns, within cv %, other cpus.")
for exe in ["R", "R2", "S2"]:
    print(f"\n{exe}")
    for i, x in enumerate(rows[exe], 1):
        print(f"  process {i:2d}: {x['median']:9.2f} ns, within cv {x['cv']:5.2f} %, other cpus {x['other']:+.2f}")
