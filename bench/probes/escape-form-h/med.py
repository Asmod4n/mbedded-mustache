import json, glob, sys, statistics, collections, os
d = sys.argv[1]; base = sys.argv[2] if len(sys.argv) > 2 else "1"
v = collections.defaultdict(list)
for f in glob.glob(d + "/j-*.json"):
    _, rnd, rest = os.path.basename(f)[:-5].split("-", 2)
    build, a = rest.rsplit("-", 1)
    try: j = json.load(open(f))
    except Exception as e: print("bad", f, e); continue
    for b in j["benchmarks"]:
        if b.get("run_type") != "iteration": continue
        if b.get("error_occurred"): print("ERROR", f, b.get("error_message")); continue
        v[(build, b["name"].split("/", 1)[1], a)].append(b["cpu_time"])
builds = sorted({k[0] for k in v}); args = ["64/0", "64/5", "1024/0", "1024/5"]; arms = sorted({k[2] for k in v}, key=int)
print("cpu time, ns per call: median (min..max, count); x = median over median of arm", base)
for bl in builds:
    for ar in args:
        row = []
        bm = statistics.median(v[(bl, ar, base)]) if v.get((bl, ar, base)) else None
        for a in arms:
            x = v.get((bl, ar, a))
            if not x: continue
            m = statistics.median(x)
            row.append(f"{a}: {m:.1f} ({min(x):.1f}..{max(x):.1f}, {len(x)})" + (f" {m/bm:.2f}x" if bm else ""))
        print(f"{bl:14s} {ar:7s} | " + " | ".join(row))
