"""Merges the iteration rows of the rounds of one binary into one Google Benchmark JSON file for compare.py.

Usage: python3 -I merge.py RESULT_DIR OUT_DIR
Reads RESULT_DIR/json/round*/ARM.COMPILER.json. Writes OUT_DIR/ARM.COMPILER.json: the context of round 1 and every row with
run_type iteration of every round. The aggregate rows of one round do not stand for the rounds together and are left out.
The files of RESULT_DIR are not changed.
"""
import glob
import json
import os
import sys

result_dir, out_dir = sys.argv[1], sys.argv[2]
os.makedirs(out_dir, exist_ok=True)
names = sorted({os.path.basename(p) for p in glob.glob(os.path.join(result_dir, "json", "round*", "*.json"))})
for name in names:
    merged = None
    for path in sorted(glob.glob(os.path.join(result_dir, "json", "round*", name))):
        with open(path) as handle:
            document = json.load(handle)
        rows = [b for b in document["benchmarks"] if b.get("run_type") == "iteration"]
        if merged is None:
            merged = {"context": document["context"], "benchmarks": []}
        merged["benchmarks"].extend(rows)
    with open(os.path.join(out_dir, name), "w") as handle:
        json.dump(merged, handle)
