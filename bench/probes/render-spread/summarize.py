"""Summary of one render-spread run. Usage: python3 -I summarize.py OUT_DIR

For each executable: the within-process spread (over the 6 repetitions of
Google Benchmark, or over the Criterion samples of ramhorns), and the
between-process spread (over the 10 process medians). For each process: the
busy cpu time of the whole machine during the process, from /proc/stat, less
the one cpu that the process uses. For the mbedded arms: the rank correlation
of the process median with the low 12 bits and the page bits of each address.
"""
import glob
import json
import os
import re
import statistics
import sys

out = sys.argv[1]
TICKS_PER_SECOND = 100
PAGE_BITS = 12


def spread(xs):
    return (max(xs) / min(xs) - 1) * 100


def cv(xs):
    return statistics.stdev(xs) / statistics.mean(xs) * 100


def busy(stat_line):
    f = [int(x) for x in stat_line.split()[1:]]
    idle = f[3] + f[4]
    return sum(f[:8]) - idle


def seconds(stamp):
    m = re.match(r"\d+-\d+-\d+T(\d+):(\d+):(\d+\.\d+)Z", stamp)
    return int(m.group(1)) * 3600 + int(m.group(2)) * 60 + float(m.group(3))


def rank(xs):
    order = sorted(range(len(xs)), key=lambda i: xs[i])
    r = [0.0] * len(xs)
    for k, i in enumerate(order):
        r[i] = float(k)
    return r


def spearman(a, b):
    if len(set(a)) < 2 or len(set(b)) < 2:
        return float("nan")
    return statistics.correlation(rank(a), rank(b))


GB_REPETITIONS = 6


def median_of_chunks(times, iters, n):
    """Cuts the Criterion samples, in their order, into n parts of equal time and returns the median of the ns per
    iteration of the parts: the estimate that Google Benchmark makes from n repetitions."""
    total = sum(times)
    parts = [[0.0, 0] for _ in range(n)]
    elapsed = 0.0
    for t, i in zip(times, iters):
        k = min(int(elapsed / total * n), n - 1)
        parts[k][0] += t
        parts[k][1] += i
        elapsed += t
    return statistics.median(p[0] / p[1] for p in parts if p[1])


rows = {}
for d in sorted(glob.glob(os.path.join(out, "round*", "*"))):
    exe = os.path.basename(d)
    rnd = int(os.path.basename(os.path.dirname(d))[5:])
    rec = {"round": rnd}
    before = open(os.path.join(d, "stat-before.txt")).read()
    after = open(os.path.join(d, "stat-after.txt")).read()
    wall = seconds(open(os.path.join(d, "end.txt")).read().strip()) - seconds(open(os.path.join(d, "start.txt")).read().strip())
    rec["other_cpus"] = (busy(after) - busy(before)) / TICKS_PER_SECOND / wall - 1
    if exe == "ramhorns":
        sample = glob.glob(os.path.join(d, "criterion", "*", "w2", "new", "sample.json"))[0]
        s = json.load(open(sample))
        per = [t / i for t, i in zip(s["times"], s["iters"])]
        est = glob.glob(os.path.join(d, "criterion", "*", "w2", "new", "estimates.json"))[0]
        rec["median"] = json.load(open(est))["median"]["point_estimate"]
        rec["within_spread"] = spread(per)
        rec["within_cv"] = cv(per)
        rec["reps"] = per
        rec["minimum"] = min(per)
        rec["median_of_6_chunks"] = median_of_chunks(s["times"], s["iters"], GB_REPETITIONS)
    else:
        g = json.load(open(os.path.join(d, "gb.json")))
        reps = [b["real_time"] for b in g["benchmarks"] if b.get("run_type") == "iteration"]
        rec["median"] = statistics.median(reps)
        rec["within_spread"] = spread(reps)
        rec["within_cv"] = cv(reps)
        rec["reps"] = reps
        rec["minimum"] = min(reps)
        err = open(os.path.join(d, "err.txt")).read()
        m = re.search(r"stack=(0x[0-9a-f]+) malloc64=(0x[0-9a-f]+) data=(0x[0-9a-f]+) output=(0x[0-9a-f]+) text=(0x[0-9a-f]+)", err)
        rec["addr"] = {k: int(v, 16) for k, v in zip(["stack", "malloc64", "data", "output", "text"], m.groups())}
    rows.setdefault(exe, []).append(rec)

print("Stocks page, run-time render, ns per render. within: over the repetitions of one process (6 for Google Benchmark,")
print("the Criterion samples for ramhorns); between: over the medians of the 10 processes. spread = max/min - 1.")
print("other cpus: busy cpus of the machine during the process, from /proc/stat, less one.")
print()
print("| executable | median of medians ns | within spread median % | within cv median % | between spread % | between cv % | other cpus max |")
print("|---|---|---|---|---|---|---|")
for exe in sorted(rows):
    r = rows[exe]
    med = [x["median"] for x in r]
    print(f"| {exe} | {statistics.median(med):.0f} | {statistics.median([x['within_spread'] for x in r]):.1f} | "
          f"{statistics.median([x['within_cv'] for x in r]):.1f} | {spread(med):.1f} | {cv(med):.1f} | "
          f"{max(x['other_cpus'] for x in r):.2f} |")
print()
print("The same processes with two other estimates: the minimum of the process, and for ramhorns the median of its")
print("samples cut into 6 parts of equal time (the estimate of 6 Google Benchmark repetitions).")
print()
print("| executable | minimum: median ns | minimum: between spread % | 6 parts: between spread % |")
print("|---|---|---|---|")
for exe in sorted(rows):
    r = rows[exe]
    mins = [x["minimum"] for x in r]
    chunks = f"{spread([x['median_of_6_chunks'] for x in r]):.1f}" if exe == "ramhorns" else "-"
    print(f"| {exe} | {statistics.median(mins):.0f} | {spread(mins):.1f} | {chunks} |")
print()
print("Rank correlation (Spearman) of the process median with address bits, mbedded arms.")
print("low = address mod 4096; page = (address >> 12) mod 512.")
print()
print("| executable | stack low | stack page | output low | data low | malloc64 low | other cpus |")
print("|---|---|---|---|---|---|---|")
for exe in sorted(rows):
    if exe == "ramhorns":
        continue
    r = rows[exe]
    med = [x["median"] for x in r]

    def low(k):
        return [x["addr"][k] % (1 << PAGE_BITS) for x in r]

    def page(k):
        return [(x["addr"][k] >> PAGE_BITS) % 512 for x in r]

    print(f"| {exe} | {spearman(med, low('stack')):+.2f} | {spearman(med, page('stack')):+.2f} | "
          f"{spearman(med, low('output')):+.2f} | {spearman(med, low('data')):+.2f} | {spearman(med, low('malloc64')):+.2f} | "
          f"{spearman(med, [x['other_cpus'] for x in r]):+.2f} |")
print()
print("Per process: round, median ns, within spread %, other cpus, stack, output.")
for exe in sorted(rows):
    print(f"\n{exe}")
    for x in sorted(rows[exe], key=lambda x: x["round"]):
        a = x.get("addr")
        where = f" stack={a['stack']:#x} output={a['output']:#x}" if a else ""
        print(f"  round {x['round']:2d}: {x['median']:8.1f} ns, within {x['within_spread']:5.1f} %, other cpus {x['other_cpus']:+.2f}{where}")
