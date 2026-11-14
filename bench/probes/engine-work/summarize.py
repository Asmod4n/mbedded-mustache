"""Reads one result directory of the engine-work probe and prints the tables of the report.

Usage: python3 -I summarize.py RESULT_DIR
A cell is the median of real_time over the iteration rows of all processes (ns per render) and the cv of the same rows (sample
standard deviation over the mean, in percent). The benchmark is run_time, the render of a template compiled at run time on
std::unordered_map data. The change is the median of the arm over the median of main, minus 1. The p-value is the time p-value of
the U test of tools/compare.py of Google Benchmark, arm against main, from compare/ARM.PAGE.COMPILER.txt.
"""
import glob
import json
import os
import re
import statistics
import sys

TITLES = {
    "main": "main 5d3c154",
    "twin": "main twin (A/A)",
    "main8192": "main with initial_capacity 8192 (the argument of render)",
    "e1": "E1 Test the kept first part inside run",
    "e2": "E2 Run a text op and the tag op after it in one dispatch",
    "e3a1024": "E3a Grow the output without a zero fill, initial capacity 1024",
    "e3a8192": "E3a Grow the output without a zero fill, initial capacity 8192",
    "all": "all: E1, E2 and E3a with 1024",
}
ARMS = ["main", "twin", "main8192", "e1", "e2", "e3a1024", "e3a8192", "all"]
COMPILERS = ["g++-16", "clang++-23"]
PAGES = {"p20": "stocks page, 20 rows (workload 2)", "p200": "stocks page, 200 rows"}


def cells(directory):
    values = {}
    for path in sorted(glob.glob(os.path.join(directory, "json", "round*", "*.json"))):
        arm, page, compiler = os.path.basename(path)[: -len(".json")].split(".", 2)
        with open(path) as handle:
            benchmarks = json.load(handle)["benchmarks"]
        for entry in benchmarks:
            if entry.get("run_type") != "iteration":
                continue
            assert entry["time_unit"] == "ns"
            values.setdefault((compiler, page, arm), []).append(entry["real_time"])
    return values


def pvalue(directory, compiler, page, arm):
    path = os.path.join(directory, "compare", "%s.%s.%s.txt" % (arm, page, compiler))
    if not os.path.exists(path):
        return None
    with open(path) as handle:
        for line in handle:
            if line.startswith("run_time_pvalue"):
                return float(line.split()[1])
    return None


def main():
    directory = sys.argv[1]
    values = cells(directory)
    arms_here = [a for a in ARMS if any(k[2] == a for k in values)]
    for compiler in COMPILERS:
        for page, page_title in PAGES.items():
            if not any(k[1] == page for k in values):
                continue
            base = values.get((compiler, page, "main"))
            print("### %s, %s: median ns per render, cv, change against main, p-value" % (compiler, page_title))
            print()
            print("| arm | median ns | cv % | change | p |")
            print("|---|---|---|---|---|")
            for arm in arms_here:
                v = values.get((compiler, page, arm))
                if not v:
                    print("| %s | error cell: not built | | | |" % TITLES[arm])
                    continue
                med = statistics.median(v)
                cv = statistics.stdev(v) / statistics.mean(v) * 100
                if arm == "main" or not base:
                    print("| %s | %.0f | %.2f | | |" % (TITLES[arm], med, cv))
                    continue
                p = pvalue(directory, compiler, page, arm)
                print("| %s | %.0f | %.2f | %+.2f %% | %s |" % (TITLES[arm], med, cv, (med / statistics.median(base) - 1) * 100,
                                                              "n/a" if p is None else "%.4f" % p))
            print()
            print("rows per arm: %s" % ", ".join("%s %d" % (a, len(values.get((compiler, page, a), []))) for a in arms_here))
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
