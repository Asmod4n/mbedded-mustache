#!/bin/sh
# Container side, as root, with network. Installs scipy, merges the rounds of each arm, and runs tools/compare.py of the release
# of Google Benchmark (the version that the JSON names) for each arm against R.
set -u
cd /w/out
date -u +%Y-%m-%dT%H:%M:%SZ > phase-compare-start.txt
apt-get update -q > apt-compare.log 2>&1
apt-get install -y -q python3-scipy python3-numpy git ca-certificates >> apt-compare.log 2>&1 || echo "apt install (compare) failed" >> failed.txt
version=$(python3 -I -c 'import json; print(json.load(open("/w/out/json/round1/R.json"))["context"]["library_version"])' 2>> compare.err)
echo "$version" > gb-version-json.txt
rm -rf /w/gb
git clone -q --depth 1 --branch "v${version#v}" https://github.com/google/benchmark /w/gb > git-gb.log 2>&1 || echo "clone google benchmark failed" >> failed.txt
git -C /w/gb rev-parse HEAD > gb-commit.txt 2>> compare.err
python3 -I /w/src/bench/probes/typed-mechanism/merge.py /w/out /w/merged 2>> compare.err || echo "merge failed" >> failed.txt
mkdir -p compare
for arm in R2 P; do
  python3 /w/gb/tools/compare.py --no-color benchmarks /w/merged/R.json /w/merged/$arm.json > compare/$arm.txt 2> compare/$arm.err || echo "compare $arm failed" >> failed.txt
done
date -u +%Y-%m-%dT%H:%M:%SZ > phase-compare-end.txt
python3 -I /w/src/bench/probes/typed-mechanism/summarize.py /w/out > summary.txt 2>> compare.err || echo "summary failed" >> failed.txt
