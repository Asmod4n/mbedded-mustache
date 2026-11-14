# Turns the raw output of one bench run into one result file, and checks the medians and spreads of result files.
import json
import re
import statistics
import sys
from pathlib import Path

DECIMALS_OF_A_TIME = 3
DECIMALS_OF_A_SPREAD = 4
DECIMALS_OF_A_MEDIAN = 6
LOAD_AVERAGE_OF_ONE_MINUTE = 0
EXIT_CODE_OF_A_MISMATCH = 1
EXIT_CODE_OF_A_WRONG_USE = 2
FIELDS_OF_GOOGLE_BENCHMARK = ("real_time", "cpu_time")
NUMBER = re.compile(r"-?[0-9]+(?:\.[0-9]+)?(?:e[-+]?[0-9]+)?")
CONSOLE_HEADER = re.compile(r"^== (?P<pairs>.*)$")
CONSOLE_ROW = re.compile(
    r"^(?P<benchmark>\S+)_(?P<statistic>median|cv|mean|stddev)\s+(?P<real>\S+) (?P<real_unit>\S+)\s+(?P<cpu>\S+) (?P<cpu_unit>\S+)\s+\d+$"
)


def summarize(rounds):
    values = [v for r in sorted(rounds, key=int) for v in rounds[r]]
    if not values:
        return {"n": 0}
    low, high = min(values), max(values)
    spread = round(high / low - 1, DECIMALS_OF_A_SPREAD) if low > 0 else None
    return {"n": len(values), "median": round(statistics.median(values), DECIMALS_OF_A_MEDIAN), "min": low, "max": high, "spread": spread}


def make_cells(collected):
    cells = []
    for key in sorted(collected, key=lambda k: json.dumps(k)):
        rounds = collected[key]
        cells.append({"key": dict(key), "rounds": rounds} | summarize(rounds))
    return cells


def add_value(collected, key, round_number, value):
    collected.setdefault(tuple(sorted(key.items())), {}).setdefault(str(round_number), []).append(value)


def read_google_benchmark_json(documents, spec):
    pattern = re.compile(spec["pattern"])
    collected = {field: {} for field in FIELDS_OF_GOOGLE_BENCHMARK}
    dates, loads, versions, build_types, errors, unmatched = [], [], set(), set(), [], []
    machine = {}
    for name, document in documents:
        found = pattern.fullmatch(name)
        if found is None:
            unmatched.append(name)
            continue
        groups = found.groupdict()
        round_number = groups.pop("round")
        context = document.get("context", {})
        dates.append(context.get("date"))
        loads.append(context.get("load_avg", [None])[LOAD_AVERAGE_OF_ONE_MINUTE])
        versions.add(context.get("library_version"))
        build_types.add(context.get("library_build_type"))
        machine = {k: context.get(k) for k in ("num_cpus", "mhz_per_cpu", "cpu_scaling_enabled", "caches")}
        for row in document.get("benchmarks", []):
            if row.get("error_occurred"):
                errors.append({"file": name, "benchmark": row.get("name"), "message": row.get("error_message")})
                continue
            if row.get("run_type") != "iteration":
                continue
            key = groups | {"benchmark": row["name"]}
            for field in FIELDS_OF_GOOGLE_BENCHMARK:
                add_value(collected[field], key, round_number, round(row[field], DECIMALS_OF_A_TIME))
    known_dates = sorted(d for d in dates if d)
    known_loads = [x for x in loads if x is not None]
    facts = {
        "files": len(documents),
        "files_not_matched": unmatched,
        "first_process_date": known_dates[0] if known_dates else None,
        "last_process_date": known_dates[-1] if known_dates else None,
        "google_benchmark_library_version": sorted(v for v in versions if v),
        "google_benchmark_library_build_type": sorted(v for v in build_types if v),
        "load_average_1_min_range": [min(known_loads), max(known_loads)] if known_loads else None,
        "machine_seen_by_google_benchmark": machine,
        "errors": errors,
    }
    measurements = [
        {"name": spec["name"] + " " + field, "unit": "ns per iteration (google benchmark " + field + ")", "cells": make_cells(collected[field])}
        for field in FIELDS_OF_GOOGLE_BENCHMARK
    ]
    return measurements, facts


def read_google_benchmark_console(text, spec):
    collected = {}
    header = None
    for line in text.splitlines():
        top = CONSOLE_HEADER.match(line)
        if top:
            header = dict(pair.split("=", 1) for pair in top["pairs"].split())
            continue
        row = CONSOLE_ROW.match(line.strip())
        if row is None or header is None:
            continue
        round_number = header["round"]
        base = {k: v for k, v in header.items() if k != "round"} | {"benchmark": row["benchmark"], "statistic": row["statistic"]}
        add_value(collected, base | {"field": "real_time", "unit": row["real_unit"]}, round_number, float(row["real"]))
        add_value(collected, base | {"field": "cpu_time", "unit": row["cpu_unit"]}, round_number, float(row["cpu"]))
    return [{"name": spec["name"], "unit": spec["unit"], "cells": make_cells(collected)}], {}


def read_labelled_lines(text, spec):
    accepted = re.compile(spec["match"])
    names = spec["label_names"]
    take = spec["take"]
    per_round = spec.get("lines_per_round", 1)
    seen, collected, failed = {}, {}, []
    for line in text.splitlines():
        if not accepted.match(line):
            continue
        words = line.split()
        key = dict(zip(names, words[: len(names)]))
        numbers = [float(w) for w in words[len(names):] if NUMBER.fullmatch(w)]
        index = seen.get(tuple(key.items()), 0)
        seen[tuple(key.items())] = index + 1
        round_number = index // per_round + 1
        if len(numbers) < take:
            failed.append({"key": key, "round": round_number, "line": line.strip()})
            continue
        for value in numbers[:take]:
            add_value(collected, key, round_number, value)
    return [{"name": spec["name"], "unit": spec["unit"], "cells": make_cells(collected)}], {"lines_without_values": failed}


def read_key_value_lines(text, spec):
    collected = {}
    for line in text.splitlines():
        pairs = dict(p.split("=", 1) for p in line.split() if "=" in p)
        if spec["round_field"] not in pairs:
            continue
        key = {k: pairs[k] for k in spec["key_fields"]}
        for field, value in pairs.items():
            if field == spec["round_field"] or field in spec["key_fields"]:
                continue
            add_value(collected, key | {"benchmark": field}, pairs[spec["round_field"]], float(value))
    return [{"name": spec["name"], "unit": spec["unit"], "cells": make_cells(collected)}], {}


def read_number_line(text, spec):
    accepted = re.compile(spec["match"])
    collected = {}
    for line in text.splitlines():
        found = accepted.match(line)
        if found:
            for value in NUMBER.findall(found["numbers"]):
                add_value(collected, spec.get("key", {}), 1, float(value))
            break
    return [{"name": spec["name"], "unit": spec["unit"], "cells": make_cells(collected)}], {}


def read_labelled_values(text, spec):
    accepted = re.compile(spec["match"])
    names = spec["label_names"]
    cells, seen_twice, failed = [], [], []
    for line in text.splitlines():
        if not accepted.match(line):
            continue
        words = line.split()
        key = dict(zip(names, words[spec.get("skip", 0) : spec.get("skip", 0) + len(names)]))
        rest = words[spec.get("skip", 0) + len(names) :]
        if len(rest) != 1 or not NUMBER.fullmatch(rest[0]):
            failed.append({"key": key, "line": line.strip()})
            continue
        if any(c["key"] == key for c in cells):
            seen_twice.append(key)
            continue
        cells.append({"key": key, "value": float(rest[0]) if "." in rest[0] else int(rest[0])})
    cells.sort(key=lambda c: json.dumps(c["key"]))
    return [{"name": spec["name"], "unit": spec["unit"], "cells": cells}], {"lines_without_one_value": failed, "keys_seen_twice": seen_twice}


READERS = {
    "google_benchmark_console": read_google_benchmark_console,
    "labelled_lines": read_labelled_lines,
    "key_value_lines": read_key_value_lines,
    "number_line": read_number_line,
    "labelled_values": read_labelled_values,
}


def load_input(spec):
    if spec["reader"] == "google_benchmark_json":
        paths = sorted(Path(spec["directory"]).glob(spec.get("glob", "*.json")))
        return [(p.name, json.loads(p.read_text())) for p in paths]
    return Path(spec["file"]).read_text()


def build_result(meta, inputs, loaded):
    measurements, facts = [], []
    for spec, data in zip(inputs, loaded):
        reader = read_google_benchmark_json if spec["reader"] == "google_benchmark_json" else READERS[spec["reader"]]
        found, known = reader(data, spec)
        for m in found:
            m["raw"] = spec["raw"]
            m["reader"] = spec["reader"]
            m["values_are"] = spec["values_are"]
        measurements += found
        facts.append({"raw": spec["raw"]} | known)
    return meta | {"facts_read_off_the_raw_files": facts, "measurements": measurements}


def format_result(result):
    lines = ["{"]
    keys = list(result)
    for i, key in enumerate(keys):
        end = "," if i + 1 < len(keys) else ""
        if key != "measurements":
            lines.append(" " + json.dumps(key) + ": " + json.dumps(result[key], ensure_ascii=False) + end)
            continue
        lines.append(' "measurements": [')
        for j, m in enumerate(result[key]):
            head = {k: v for k, v in m.items() if k != "cells"}
            lines.append("  " + json.dumps(head, ensure_ascii=False)[:-1] + ', "cells": [')
            cells = [("   " + json.dumps(c, ensure_ascii=False)) for c in m["cells"]]
            lines.append(",\n".join(cells))
            lines.append("  ]}" + ("," if j + 1 < len(result[key]) else ""))
        lines.append(" ]" + end)
    lines.append("}")
    return "\n".join(lines) + "\n"


def find_mismatches(result):
    mismatches = []
    for m in result.get("measurements", []):
        for cell in m["cells"]:
            if "rounds" not in cell:
                if set(cell) != {"key", "value"} or not isinstance(cell["value"], (int, float)):
                    mismatches.append({"measurement": m["name"], "key": cell.get("key"), "stored": cell, "recomputed": "a cell without rounds holds a key and one number"})
                continue
            again = summarize(cell["rounds"])
            stored = {k: cell.get(k) for k in again}
            if stored != again:
                mismatches.append({"measurement": m["name"], "key": cell["key"], "stored": stored, "recomputed": again})
    return mismatches


def main(arguments):
    if len(arguments) == 3 and arguments[0] == "write":
        spec = json.loads(Path(arguments[1]).read_text())
        loaded = [load_input(s) for s in spec["inputs"]]
        Path(arguments[2]).write_text(format_result(build_result(spec["meta"], spec["inputs"], loaded)))
        return 0
    if len(arguments) >= 2 and arguments[0] == "check":
        status = 0
        for name in arguments[1:]:
            mismatches = find_mismatches(json.loads(Path(name).read_text()))
            print(name + ": " + ("ok" if not mismatches else json.dumps(mismatches)))
            status = status if not mismatches else EXIT_CODE_OF_A_MISMATCH
        return status
    print("usage: record.py write SPEC.json RESULT.json | record.py check RESULT.json...", file=sys.stderr)
    return EXIT_CODE_OF_A_WRONG_USE


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
