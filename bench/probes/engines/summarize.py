"""Reads one result directory of the engines probe and prints the median time per render of every arm.

Usage: python3 -I summarize.py RESULT_DIR
Google Benchmark: the median aggregate of the 30 repetitions (real_time, ns) and its cv aggregate.
Criterion: median.point_estimate of new/estimates.json (ns) and its confidence interval.
"""
import glob
import json
import os
import sys

WORKLOADS = {"1": "workload 1 (ramhorns simple)", "2": "workload 2 (template-benchmark stocks page)", "3": "workload 3 (news page)"}
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
    for workload, title in WORKLOADS.items():
        print("## " + title + ": median time per render, ns, one table of Mustache-form and Jinja-form engines")
        print()
        print("| arm | engine | compiler | median ns/render | cv % of 30 repetitions |")
        print("|---|---|---|---|---|")
        for (w, engine, name, compiler), cell in sorted(google.items(), key=lambda item: item[1].get("median", 0)):
            if w == workload and "median" in cell:
                print("| %s | %s | %s | %.1f | %.2f |" % (name, engine, compiler, cell["median"], cell.get("cv", float("nan"))))
        for (w, engine), (point, low, high) in sorted(criterion.items(), key=lambda item: item[1][0]):
            if w == workload and engine not in CODE_ENGINES:
                print("| criterion | %s | rustc, opt-level 3, lto fat, target-cpu x86-64-v4 | %.1f | CI %.1f to %.1f |" % (engine, point, low, high))
        print()
        print("Code-built HTML (maud, markup), Criterion median.point_estimate, ns/render:")
        print()
        print("| engine | median ns/render | confidence interval of the median |")
        print("|---|---|---|")
        for (w, engine), (point, low, high) in sorted(criterion.items(), key=lambda item: item[1][0]):
            if w == workload and engine in CODE_ENGINES:
                print("| %s | %.1f | %.1f to %.1f |" % (engine, point, low, high))
        print()
    print("## A/A: the run_time arm of g++-16 built twice")
    print()
    print("| workload | first binary ns | second binary ns | second over first, minus 1 |")
    print("|---|---|---|---|")
    for workload in WORKLOADS:
        a = google.get((workload, "mbedded", "run_time", "g++-16"), {}).get("median")
        b = google.get((workload, "mbedded-twin", "run_time", "g++-16"), {}).get("median")
        if a and b:
            print("| %s | %.1f | %.1f | %+.2f %% |" % (workload, a, b, (b / a - 1) * 100))
    print()
    with open(os.path.join(directory, "context-common.txt")) as handle:
        for line in handle:
            if line.startswith("sysbench_cpu_events_per_second"):
                values = [float(x) for x in line.split("=", 1)[1].split()]
                print("noise floor, sysbench cpu events/s, 10 runs of 1 s: min %.1f max %.1f spread %.2f %%" % (min(values), max(values), (max(values) / min(values) - 1) * 100))


main()
