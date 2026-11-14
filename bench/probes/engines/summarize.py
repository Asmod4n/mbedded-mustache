"""Reads one result directory of the engines probe and prints the median time per render of every arm.

Usage: python3 -I summarize.py RESULT_DIR
Google Benchmark: the median aggregate of the 30 repetitions (real_time, ns) and its cv aggregate.
Criterion: median.point_estimate of new/estimates.json (ns) and its confidence interval.
"""
import glob
import json
import os
import sys

WORKLOADS = {"2": "workload 2 (template-benchmark stocks page)"}
COMPILERS = {"g++-16": "g++-16", "clang++-23": "clang++-23"}
CODE_ENGINES = ["maud", "markup"]


def google_rows(directory):
    rows = {}
    for path in sorted(glob.glob(os.path.join(directory, "json", "*.json"))):
        engine, workload, compiler = os.path.basename(path)[: -len(".json")].split(".")
        with open(path) as handle:
            benchmarks = json.load(handle)["benchmarks"]
        for entry in benchmarks:
            if entry.get("run_type") != "aggregate":
                continue
            name = entry["run_name"].split("/")[0]
            cell = rows.setdefault((workload, engine, name, compiler), {})
            if entry["aggregate_name"] == "median":
                assert entry["time_unit"] == "ns"
                cell["median"] = entry["real_time"]
            elif entry["aggregate_name"] == "cv":
                cell["cv"] = entry["real_time"] * 100
    return rows


def criterion_rows(directory):
    rows = {}
    for path in sorted(glob.glob(os.path.join(directory, "criterion", "*", "*", "w*", "new", "estimates.json"))):
        parts = path.split(os.sep)
        engine, workload = parts[-4], parts[-3][1:]
        with open(path) as handle:
            median = json.load(handle)["median"]
        rows[(workload, engine)] = (median["point_estimate"], median["confidence_interval"]["lower_bound"], median["confidence_interval"]["upper_bound"])
    return rows


def main():
    directory = sys.argv[1]
    google = google_rows(directory)
    criterion = criterion_rows(directory)
    print("Workload 2, the stocks page of mbosecke/template-benchmark. Median time per render, ns.")
    print()
    run_time = []
    compile_time = []
    for (workload, engine, name, compiler), cell in google.items():
        if "median" not in cell:
            continue
        row = (cell["median"], "%s %s (%s)" % (engine, name, compiler), "Google Benchmark", "%.2f %%" % cell.get("cv", float("nan")))
        (compile_time if name == "reflection" else run_time).append(row)
    for (workload, engine), (point, low, high) in criterion.items():
        row = (point, "%s (rustc)" % engine, "Criterion", "CI %.1f to %.1f ns" % (low, high))
        (run_time if engine == "ramhorns" else compile_time).append(row)
    for title, rows in (("Run time: the template is compiled at run time", run_time), ("Compile time: the template is compiled at build time", compile_time)):
        print("### " + title)
        print()
        print("| arm | median ns/render | tool | cv of 30 repetitions or confidence interval of the median |")
        print("|---|---|---|---|")
        for median, arm, tool, spread in sorted(rows):
            print("| %s | %.1f | %s | %s |" % (arm, median, tool, spread))
        print()
    a = google.get(("2", "mbedded", "run_time", "g++-16"), {}).get("median")
    b = google.get(("2", "mbedded-twin", "run_time", "g++-16"), {}).get("median")
    if a and b:
        print("A/A, run_time of g++-16 built twice: %.1f ns and %.1f ns, second over first minus 1: %+.2f %%" % (a, b, (b / a - 1) * 100))
        print()
    with open(os.path.join(directory, "context-common.txt")) as handle:
        for line in handle:
            if line.startswith("sysbench_cpu_events_per_second"):
                values = [float(x) for x in line.split("=", 1)[1].split()]
                print("noise floor, sysbench cpu events/s, 10 runs of 1 s: min %.1f max %.1f spread %.2f %%" % (min(values), max(values), (max(values) / min(values) - 1) * 100))
    try:
        start = open(os.path.join(directory, "timed-start.txt")).read().strip()
        end = open(os.path.join(directory, "timed-end.txt")).read().strip()
        print("timed part: %s to %s" % (start, end))
    except OSError:
        pass


main()
