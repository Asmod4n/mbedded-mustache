# compare.sh

The script measures the render time of two commits of the headers of mbedded-mustache in a container.

## Usage

    bench/compare.sh <old-commit> <new-commit> [image]

The default image is localhost/mbedded-cbor-docs. The run has 6 rounds.

## What it measures

- Workloads 1 to 3 of bench/render.cpp: the run-time render and, with g++, the reflection render.
- Compilers: g++-16 and clang++-23, each with -Os and -O2.
- Flags: -std=c++26 -march=x86-64-v4 -DNDEBUG -falign-functions=64 -falign-loops=64.
  g++ also gets -freflection and -falign-jumps=64.

## Method

1. The script exports include/ of each commit with `git archive`.
   bench/render.cpp comes from the new commit.
2. Each arm, workload, compiler and level is its own binary. Four builds run at a time.
3. sysbench cpu runs 10 times for 1 s with one thread before the first round.
4. Each round runs every binary once. The order of the arms alternates by round.
   Each process makes 5 repetitions of at least 0.05 s. The runs are made as the user nobody.

There is no gate, no record of steal and no binary built twice.

## Result

One file: `bench/results/compare-<UTC time>.txt`, written by summarize.py.
bench/record.py turns the same raw files into one JSON file of the form below.

# record.py

The script turns the raw output of one bench run into one result file. It also checks result files.

## Usage

    python3 -I bench/record.py write <spec.json> <result.json>
    python3 -I bench/record.py check bench/results/*.json

The spec holds the text fields of the result under `meta` and the raw inputs under `inputs`.
Each input names one reader:

- `google_benchmark_json`: a directory of JSON files of Google Benchmark. A regular expression on the
  file name gives the round and the keys of the cell.
- `google_benchmark_console`: the console output of Google Benchmark after a line `== round=<n> <key>=<value>...`.
- `labelled_lines`: lines of labels and numbers, for example `run_time 184.3 186.2 188.4`.
- `key_value_lines`: lines of `<key>=<value>` pairs with one round field.
- `number_line`: the numbers of one line, for example the output of sysbench.

`check` computes the count, the median, the minimum, the maximum and the spread of each cell again from
its values. It prints `ok` for each file that agrees, and exits with code 1 when a file does not agree.

## Result

One file for each run: `bench/results/<name>-<UTC time>.json`. The file holds:

- what was measured, the arms, the builds and the headers;
- the method of the run, and how it differs from the method of mbedded-cbor;
- the commit that holds the probe sources and scripts, and the names of these files;
- the context of the run that is known, and where each fact was read;
- `not_recorded`: each fact of the context that no file of the run holds;
- the facts read off the raw files: the dates of the first and the last process, the version and build type
  of the Google Benchmark library, the range of the load average, the errors;
- for each measurement: the unit, what one value is, and one cell for each key.
  A cell holds every value by round, the count, the median, the minimum, the maximum and the spread.

The spread is the maximum over the minimum, minus one. A value of Google Benchmark is rounded to 0.001 ns.
A file of Google Benchmark JSON gives two measurements: real_time and cpu_time.

# Probes

The sources and scripts of the probes stand in bench/probes/<name>/ as they ran.
Their paths (/w, /b, /src, /arms) are the mounts of the container.
One comment in plan-review-4/arms.hpp names the path of final.cpp in this tree in place of the scratch path.

| Probe | What it is |
|---|---|
| compare-first | The first comparison of 0f04731 and 7b9bd9e, with aggregates only. |
| run-length | One render binary, built twice, at four run lengths. |
| malloc-tunables | The same binaries with and without GLIBC_TUNABLES. |
| compute-loop | A loop of xorshift steps in a process of the same shape. |
| render-stack | Five targets of the render of workload 1. |
| render-pool | Five ways to get the output buffer, one of them a pool. |
| escape-sink | Eight targets and forms of the HTML escape. |
| escape-form-h | Forms of escaped_h against mustache::escape_into. |
| plan-review-4 | Escape forms that check the room, and checks of the library. |
| plan-review-5 | The cost of the zero fill of a string (cost.cpp), and checks. |
| plan-review-7 | A check: the writers in a room of the exact size with guard bytes. |
| plan-review-8 | A check: two plan forms of the headers (patches against 7564b32). |

plan-review-5/bound.cpp, retry.cpp and twice.cpp, plan-review-7 and plan-review-8 are checks.
They find the stores past the written count with guard bytes, and give no time.
No result file stands for them.

# Runs kept

| File | Probe |
|---|---|
| compare-20261112T190806Z.json | compare-first |
| compare-20261112T194930Z.json | compare.sh at 7b9bd9e |
| run-length-20261112T194840Z.json | run-length |
| malloc-tunables-20261112T195247Z.json | malloc-tunables |
| compute-loop-20261112T195312Z.json | compute-loop |
| render-stack-20261112T195536Z.json | render-stack, first form |
| render-stack-20261112T195802Z.json | render-stack |
| render-pool-20261112T212325Z.json | render-pool |
| escape-sink-20261112T202359Z.json and five more | escape-sink |
| escape-form-h-20261112T204259Z.json and four more | escape-form-h |
| plan-review-4-20261113T050118Z.json | plan-review-4, run 1 |
| plan-review-4-20261113T050436Z.json | plan-review-4, run 2 |
| plan-review-5-cost-20261113T064347Z.json | plan-review-5, cost.cpp |
| compare-20261114T063020Z.json | compare.sh, 93683bc against fee69a6 |
| compare-20261114T064610Z.json | compare.sh, dbd4cac against d13b6ca |

These runs did not follow the method of mbedded-cbor. Each file says what is not recorded.
