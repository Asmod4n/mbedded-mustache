# Reads the results directory of a run and prints, for each round, the median of A, Ap and M and the load and MHz records.
# Usage: python3 -I summarize.py RESULT_DIR
import json, os, statistics, sys

directory = sys.argv[1]


def criterion(path):
    estimates = json.load(open(os.path.join(path, "criterion", "ramhorns", "w2", "new", "estimates.json")))
    sample = json.load(open(os.path.join(path, "criterion", "ramhorns", "w2", "new", "sample.json")))
    per_iteration = [t / n for n, t in zip(sample["iters"], sample["times"])]
    half = len(per_iteration) // 2
    return {
        "median": estimates["median"]["point_estimate"],
        "mean": estimates["mean"]["point_estimate"],
        "std_dev": estimates["std_dev"]["point_estimate"],
        "first_half": statistics.median(per_iteration[:half]),
        "second_half": statistics.median(per_iteration[half:]),
    }


def google(path):
    data = json.load(open(os.path.join(path, "gb.json")))
    for b in data["benchmarks"]:
        if b["name"] == "run_time_median":
            median = b["real_time"]
        if b["name"] == "run_time_cv":
            cv = b["real_time"]
    repetitions = [b["real_time"] for b in data["benchmarks"] if b["name"] == "run_time" and b["run_type"] == "iteration"]
    return {"median": median, "cv": cv * 100, "first_half": statistics.median(repetitions[:3]), "second_half": statistics.median(repetitions[3:])}


def mhz(path):
    values = [float(line.split(":")[1]) for line in open(path)]
    return "%.0f" % statistics.mean(values)


results = {}
for round_name in sorted(d for d in os.listdir(directory) if d.startswith("round")):
    for exe in ("A", "Ap", "M"):
        path = os.path.join(directory, round_name, exe)
        r = google(path) if exe == "M" else criterion(path)
        r["load_before"] = open(os.path.join(path, "loadavg-before.txt")).read().split()[0]
        r["load_after"] = open(os.path.join(path, "loadavg-after.txt")).read().split()[0]
        r["mhz_before"] = mhz(os.path.join(path, "mhz-before.txt"))
        r["mhz_after"] = mhz(os.path.join(path, "mhz-after.txt"))
        results[(round_name, exe)] = r

print("Median ns per render of workload 2. A and Ap: ramhorns Criterion. M: mbedded run_time clang++-23, Google Benchmark.")
print()
print("| round | exe | median ns | first half ns | second half ns | load before, after | MHz before, after |")
print("|---|---|---|---|---|---|---|")
for (round_name, exe), r in results.items():
    print("| %s | %s | %.1f | %.1f | %.1f | %s, %s | %s, %s |" % (round_name, exe, r["median"], r["first_half"], r["second_half"], r["load_before"], r["load_after"], r["mhz_before"], r["mhz_after"]))
print()
for exe in ("A", "Ap", "M"):
    medians = [results[(r, exe)]["median"] for r in sorted({k[0] for k in results})]
    spread = (max(medians) / min(medians) - 1) * 100
    print("%s: medians %s, max over min minus 1 = %.2f %%, cv of the 5 medians = %.2f %%" % (exe, " ".join("%.1f" % m for m in medians), spread, statistics.stdev(medians) / statistics.mean(medians) * 100))
