"""Reads one result directory of the fewer-calls probe and prints the tables of the report.

Usage: python3 -I summarize.py RESULT_DIR
A cell is the median of real_time over the iteration rows of all rounds (ns per render) and the cv of the same rows
(sample standard deviation over the mean, in percent). The columns are the two benchmarks of one binary: run_time, the render of
a template compiled at run time on std::unordered_map data, and reflection, the render of a template of the source on structs.
The p-value is the time p-value of the U test of tools/compare.py of Google Benchmark, arm against head, from compare/ARM.COMPILER.txt.
"""
import glob
import json
import os
import re
import statistics
import sys

TITLES = {
    "head": "head 4e036ed",
    "twin": "head twin (A/A)",
    "h1": "H1 Find the map of a dotted name once per row",
    "h2a": "H2a Call the find of the map directly, without function pointers (std::visit)",
    "h2b": "H2b Call the find of the map directly, without function pointers (switch)",
    "h5": "H5 Search the 16 marks inline with one load",
    "h6": "H6 Render a row of the batch path only once",
    "all": "all: H1, H2a, H5 and H6",
}
ARMS = ["head", "twin", "h1", "h2a", "h2b", "h5", "h6", "all"]
COMPILERS = ["g++-16", "clang++-23"]
BENCHMARKS = ["run_time", "reflection"]


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


def cell(values, compiler, arm, benchmark):
    v = values.get((compiler, arm, benchmark))
    if not v:
        if benchmark == "reflection" and compiler == "clang++-23":
            return "error cell: no reflection in clang++-23"
        return "error cell: not built for this arm"
    return "%.0f (%.2f %%)" % (statistics.median(v), statistics.stdev(v) / statistics.mean(v) * 100)


def main():
    directory = sys.argv[1]
    values = cells(directory)
    for compiler in COMPILERS:
        print("### %s: median ns per render (cv, 30 rows of 5 rounds x 6 repetitions)" % compiler)
        print()
        print("| arm | run_time | reflection |")
        print("|---|---|---|")
        for arm in ARMS:
            if (compiler, arm, "run_time") not in values and (compiler, arm, "reflection") not in values:
                print("| %s | error cell: no build for this arm on %s | error cell |" % (TITLES[arm], compiler))
                continue
            print("| %s | %s | %s |" % (TITLES[arm], cell(values, compiler, arm, "run_time"), cell(values, compiler, arm, "reflection")))
        print()
        print("Median of the arm over the median of head, minus 1, and the time p-value of the U test against head:")
        print()
        print("| arm | run_time | reflection |")
        print("|---|---|---|")
        for arm in ARMS[1:]:
            row = []
            for benchmark in BENCHMARKS:
                v = values.get((compiler, arm, benchmark))
                b = values.get((compiler, "head", benchmark))
                if not v or not b:
                    row.append("n/a")
                    continue
                p = pvalue(directory, compiler, arm, benchmark)
                row.append("%+.2f %% (p %s)" % ((statistics.median(v) / statistics.median(b) - 1) * 100, "n/a" if p is None else "%.4f" % p))
            print("| %s | %s | %s |" % (TITLES[arm], row[0], row[1]))
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
