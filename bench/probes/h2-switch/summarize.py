"""Reads one result directory of the h2-switch probe and prints the tables of the report.

Usage: python3 -I summarize.py RESULT_DIR
A cell is the median of real_time over the iteration rows of all rounds (ns per render) and the cv of the same rows
(sample standard deviation over the mean, in percent). The columns are the two compilers; the benchmark is run_time, the render of
a template compiled at run time on std::unordered_map data.
The p-value is the time p-value of the U test of tools/compare.py of Google Benchmark, arm against head, from compare/ARM.COMPILER.txt.
"""
import glob
import json
import os
import re
import statistics
import sys

TITLES = {
    "head": "head 6c50679",
    "twin": "head twin (A/A)",
    "h2bp": "H2b' Call the find of the map directly, with a pair {pointer, type index} and a switch",
}
ARMS = ["head", "twin", "h2bp"]
COMPILERS = ["g++-16", "clang++-23"]
BENCHMARKS = ["run_time"]


def cells(directory):
    values = {}
    for path in sorted(glob.glob(os.path.join(directory, "json", "round*", "*.json"))):
        arm, compiler = os.path.basename(path)[: -len(".json")].split(".", 1)
        with open(path) as handle:
            benchmarks = json.load(handle)["benchmarks"]
        for entry in benchmarks:
            if entry.get("run_type") != "iteration":
                continue
            assert entry["time_unit"] == "ns"
            values.setdefault((compiler, arm, entry["run_name"]), []).append(entry["real_time"])
    return values


def pvalue(directory, compiler, arm, benchmark):
    path = os.path.join(directory, "compare", "%s.%s.txt" % (arm, compiler))
    if not os.path.exists(path):
        return None
    with open(path) as handle:
        for line in handle:
            if line.startswith(benchmark + "_pvalue"):
                return float(line.split()[1])
    return None


def cell(values, compiler, arm):
    v = values.get((compiler, arm, "run_time"))
    if not v:
        return "error cell: not built"
    return "%.0f (%.2f %%)" % (statistics.median(v), statistics.stdev(v) / statistics.mean(v) * 100)


def change(values, directory, compiler, arm):
    v = values.get((compiler, arm, "run_time"))
    b = values.get((compiler, "head", "run_time"))
    if not v or not b:
        return "n/a"
    p = pvalue(directory, compiler, arm, "run_time")
    return "%+.2f %% (p %s)" % ((statistics.median(v) / statistics.median(b) - 1) * 100, "n/a" if p is None else "%.4f" % p)


def main():
    directory = sys.argv[1]
    values = cells(directory)
    print("### median ns per render (cv, 30 rows of 5 rounds x 6 repetitions)")
    print()
    print("| arm | g++-16 | clang++-23 |")
    print("|---|---|---|")
    for arm in ARMS:
        print("| %s | %s | %s |" % (TITLES[arm], cell(values, "g++-16", arm), cell(values, "clang++-23", arm)))
    print()
    print("### median of the arm over the median of head, minus 1, and the time p-value of the U test of compare.py against head")
    print()
    print("| arm | g++-16 | clang++-23 |")
    print("|---|---|---|")
    for arm in ARMS[1:]:
        print("| %s | %s | %s |" % (TITLES[arm], change(values, directory, "g++-16", arm), change(values, directory, "clang++-23", arm)))
    print()
    with open(os.path.join(directory, "context-common.txt")) as handle:
        for line in handle:
            if line.startswith("sysbench_cpu_events_per_second"):
                numbers = [float(x) for x in line.split("=", 1)[1].split()]
                print("noise floor, sysbench cpu events/s, 10 runs of 1 s: min %.1f max %.1f spread %.2f %%" % (min(numbers), max(numbers), (max(numbers) / min(numbers) - 1) * 100))
    print()
    sizes = os.path.join(directory, "sizes.txt")
    if os.path.exists(sizes):
        print("### .text size and compile time of each binary (four compiles ran at the same time on 4 processors)")
        print()
        print(open(sizes).read())


main()
