"""Reads one result directory of the output-targets probe and prints the tables of the report.

Usage: python3 -I summarize.py RESULT_DIR
A cell is the median of real_time over the iteration rows of all rounds of the phase (ns per render) and the cv of the same rows
(sample standard deviation over the mean, in percent). The rounds column is the median and the range of the 10 medians of the rounds
(one process each). The p-value is the time p-value of the U test of tools/compare.py of Google Benchmark, against fresh, from
compare/ARM.COMPILER.txt (main phase) and compare/noise-twin.COMPILER.txt (noise phase).
"""
import glob
import json
import os
import statistics
import sys

TITLES = {
    "fresh": "fresh: form 1, render into a new std::string",
    "twin": "twin: fresh built again (A/A)",
    "reused": "reused: form 2, render_to into one std::string kept over the renders",
}
ARMS = ["fresh", "twin", "reused"]
COMPILERS = ["g++-16", "clang++-23"]


def rows_of(directory, json_dir):
    rows = {}
    for path in sorted(glob.glob(os.path.join(directory, json_dir, "round*", "*.json"))):
        arm, compiler = os.path.basename(path)[: -len(".json")].split(".", 1)
        with open(path) as handle:
            benchmarks = json.load(handle)["benchmarks"]
        values = [e["real_time"] for e in benchmarks if e.get("run_type") == "iteration"]
        for e in benchmarks:
            assert e["time_unit"] == "ns"
        rows.setdefault((compiler, arm), []).append(values)
    return rows


def pvalue(directory, name):
    path = os.path.join(directory, "compare", name + ".txt")
    if not os.path.exists(path):
        return None
    with open(path) as handle:
        for line in handle:
            if line.startswith("stocks_pvalue"):
                return float(line.split()[1])
    return None


def flat(rounds):
    return [v for r in rounds for v in r]


def cell(rounds):
    v = flat(rounds)
    medians = [statistics.median(r) for r in rounds]
    return "%.1f (%.2f %%) | %.1f (%.1f to %.1f)" % (
        statistics.median(v), statistics.stdev(v) / statistics.mean(v) * 100, statistics.median(medians), min(medians), max(medians))


def ratio(rounds, base):
    return (statistics.median(flat(rounds)) / statistics.median(flat(base)) - 1) * 100


def main():
    directory = sys.argv[1]
    main_rows = rows_of(directory, "json")
    noise_rows = rows_of(directory, "json-noise")
    for compiler in COMPILERS:
        print("### %s: ns per render of the stocks page" % compiler)
        print()
        print("| arm | median (cv) of all rows | median (range) of the round medians |")
        print("|---|---|---|")
        for arm in ARMS:
            if (compiler, arm) not in main_rows:
                print("| %s | error cell: not built | |" % TITLES[arm])
                continue
            print("| %s | %s |" % (TITLES[arm], cell(main_rows[(compiler, arm)])))
        print()
        print("| comparison | median over median, minus 1 | time p-value (U test) |")
        print("|---|---|---|")
        base = main_rows[(compiler, "fresh")]
        for arm in ("twin", "reused"):
            p = pvalue(directory, "%s.%s" % (arm, compiler))
            print("| main phase, %s against fresh | %+.2f %% | %s |" % (arm, ratio(main_rows[(compiler, arm)], base), "n/a" if p is None else "%.4f" % p))
        nb = noise_rows[(compiler, "fresh")]
        p = pvalue(directory, "noise-twin.%s" % compiler)
        print("| noise phase (before the main phase), twin against fresh | %+.2f %% | %s |" % (ratio(noise_rows[(compiler, "twin")], nb), "n/a" if p is None else "%.4f" % p))
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
