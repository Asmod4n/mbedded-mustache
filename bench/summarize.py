import collections
import glob
import json
import re
import statistics
import sys

cells = collections.defaultdict(lambda: collections.defaultdict(list))
for path in sorted(glob.glob(sys.argv[1] + "/r*.json")):
    m = re.match(r".*/r(\d+)-(old|new)-(\d)-(\S+)\.json", path)
    round_number, arm, workload, build = m.groups()
    for b in json.load(open(path))["benchmarks"]:
        if b["run_type"] != "iteration":
            continue
        name = b["name"].split("/")[0]
        cells[(workload, build, name)][arm].append(round(b["real_time"], 2))

print("each cell: every repetition in ns (rounds in order), median, spread (min to max); change = median of new over median of old")
for key in sorted(cells):
    arms = cells[key]
    old, new = arms["old"], arms["new"]
    mo, mn = statistics.median(old), statistics.median(new)
    print(f"workload={key[0]} build={key[1]} benchmark={key[2]}")
    for arm, v in (("old", old), ("new", new)):
        print(f"  {arm}: median={statistics.median(v):.2f} min={min(v):.2f} max={max(v):.2f} n={len(v)} values={v}")
    print(f"  change new over old: {100 * (mn / mo - 1):+.1f} %")
