"""Reads one result directory of the runtime-speed probe and prints one table for each compiler.

Usage: python3 -I summarize.py RESULT_DIR
Rows are the arms, columns are the data forms. A cell is the median of real_time over the iteration rows of all rounds (ns per render)
and the cv of the same rows (sample standard deviation over the mean, in percent), with the number of rows.
"""
import glob
import json
import os
import statistics
import sys

ARMS = ["H", "Htwin", "C1", "C2p", "C3", "C4", "C5", "All"]
COMPILERS = ["g++-16", "clang++-23"]
FORMS = ["ordered_map", "flat_map", "unordered_map"]


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
            form = entry["run_name"][len("stocks_"):]
            values.setdefault((compiler, arm, form), []).append(entry["real_time"])
    return values


def main():
    directory = sys.argv[1]
    values = cells(directory)
    for compiler in COMPILERS:
        print("### %s: median ns per render (cv %%, rows)" % compiler)
        print()
        print("| arm | " + " | ".join(FORMS) + " |")
        print("|---|" + "---|" * len(FORMS))
        base = {form: statistics.median(values[(compiler, "H", form)]) for form in FORMS}
        for arm in ARMS:
            row = []
            for form in FORMS:
                v = values[(compiler, arm, form)]
                median = statistics.median(v)
                cv = statistics.stdev(v) / statistics.mean(v) * 100
                row.append("%.0f (%.2f %%, %d)" % (median, cv, len(v)))
            print("| %s | %s |" % (arm, " | ".join(row)))
        print()
        print("Median over the median of H, minus 1:")
        print()
        print("| arm | " + " | ".join(FORMS) + " |")
        print("|---|" + "---|" * len(FORMS))
        for arm in ARMS[1:]:
            row = ["%+.2f %%" % ((statistics.median(values[(compiler, arm, form)]) / base[form] - 1) * 100) for form in FORMS]
            print("| %s | %s |" % (arm, " | ".join(row)))
        print()
    with open(os.path.join(directory, "context-common.txt")) as handle:
        for line in handle:
            if line.startswith("sysbench_cpu_events_per_second"):
                numbers = [float(x) for x in line.split("=", 1)[1].split()]
                print("noise floor, sysbench cpu events/s, 10 runs of 1 s: min %.1f max %.1f spread %.2f %%" % (min(numbers), max(numbers), (max(numbers) / min(numbers) - 1) * 100))


main()
