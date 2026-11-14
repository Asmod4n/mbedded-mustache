"""Reads one result directory of the typed-mechanism probe and prints the tables of the report.

Usage: python3 -I summarize.py RESULT_DIR
A cell is the median of real_time over the iteration rows of all processes (ns per render) and the cv of the same rows (sample
standard deviation over the mean, in percent). Then the median and cv of the 10 process medians. The p-value is the time p-value
of the U test of tools/compare.py of Google Benchmark, arm against R, from compare/ARM.txt.
"""
import glob
import json
import os
import statistics
import sys

TITLES = {
    "R": "R typed, members from C++26 reflection (head)",
    "R2": "R2 twin of R (A/A)",
    "P": "P typed, members from Boost.PFR",
}
ARMS = ["R", "R2", "P"]


def cv(xs):
    return statistics.stdev(xs) / statistics.mean(xs) * 100


def pvalue(directory, arm):
    path = os.path.join(directory, "compare", arm + ".txt")
    if not os.path.exists(path):
        return None
    with open(path) as handle:
        for line in handle:
            if line.startswith("render_pvalue"):
                return float(line.split()[1])
    return None


def main():
    directory = sys.argv[1]
    rows = {}
    process_medians = {}
    for path in sorted(glob.glob(os.path.join(directory, "json", "round*", "*.json"))):
        arm = os.path.basename(path)[: -len(".json")]
        with open(path) as handle:
            benchmarks = json.load(handle)["benchmarks"]
        reps = [b["real_time"] for b in benchmarks if b.get("run_type") == "iteration"]
        assert all(b["time_unit"] == "ns" for b in benchmarks)
        rows.setdefault(arm, []).extend(reps)
        process_medians.setdefault(arm, []).append(statistics.median(reps))
    print("Stocks page (workload 2), typed render, g++-16 -O2, real time in ns per render.")
    print()
    print("| arm | median ns | cv % (all rows) | median of 10 process medians ns | cv % of process medians | change against R | p |")
    print("|---|---|---|---|---|---|---|")
    base = statistics.median(rows["R"])
    for arm in ARMS:
        v = rows[arm]
        m = statistics.median(v)
        pm = process_medians[arm]
        if arm == "R":
            print("| %s | %.0f | %.2f | %.0f | %.2f | | |" % (TITLES[arm], m, cv(v), statistics.median(pm), cv(pm)))
        else:
            p = pvalue(directory, arm)
            print("| %s | %.0f | %.2f | %.0f | %.2f | %+.2f %% | %s |" % (TITLES[arm], m, cv(v), statistics.median(pm), cv(pm),
                  (m / base - 1) * 100, "n/a" if p is None else "%.4f" % p))
    print()
    print("rows per arm: " + ", ".join("%s %d" % (a, len(rows[a])) for a in ARMS))
    noise = [float(x) for x in open(os.path.join(directory, "noise.txt")).read().split()]
    print("noise floor, sysbench cpu events/s, 10 runs of 1 s: min %.1f max %.1f spread %.2f %% (max/min - 1)" % (min(noise), max(noise), (max(noise) / min(noise) - 1) * 100))
    print("P against R2 is not computed; R2 is the A/A of R.")


main()
