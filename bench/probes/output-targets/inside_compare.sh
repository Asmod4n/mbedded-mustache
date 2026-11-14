#!/bin/sh
# Container side, phase compare, with network. Installs scipy from the distribution, fetches tools/compare.py of the release of Google
# Benchmark that the JSON names, and compares twin and reused with fresh for each compiler, in the noise phase and in the main phase.
# Stderr goes to log files under /w/out.
set -u
. /w/src/bench/probes/output-targets/flags.sh
cd /w/out
apt-get update -q > apt.log 2>&1
apt-get install -y -q python3-scipy python3-numpy git ca-certificates >> apt.log 2>&1 || echo "apt install failed" >> failed.txt
version=$(python3 -I -c 'import json,sys; print(json.load(open("/w/out/json/round1/fresh.g++-16.json"))["context"]["library_version"])' 2>> compare.err)
echo "$version" > gb-version.txt
tag=v${version#v}
git clone -q --depth 1 --branch "$tag" https://github.com/google/benchmark /w/gb > git-gb.log 2>&1 || echo "clone google benchmark $tag failed" >> failed.txt
git -C /w/gb rev-parse HEAD > gb-commit.txt 2>> compare.err
python3 -I /w/src/bench/probes/output-targets/merge.py /w/out /w/merged json 2>> compare.err || echo "merge failed" >> failed.txt
python3 -I /w/src/bench/probes/output-targets/merge.py /w/out /w/merged-noise json-noise 2>> compare.err || echo "merge of the noise phase failed" >> failed.txt
mkdir -p compare
for cxx in $COMPILERS; do
  for arm in twin reused; do
    python3 /w/gb/tools/compare.py --no-color benchmarks /w/merged/fresh.$cxx.json /w/merged/$arm.$cxx.json > compare/$arm.$cxx.txt 2> compare/$arm.$cxx.err ||
      echo "compare $arm.$cxx failed" >> failed.txt
  done
  python3 /w/gb/tools/compare.py --no-color benchmarks /w/merged-noise/fresh.$cxx.json /w/merged-noise/twin.$cxx.json > compare/noise-twin.$cxx.txt 2> compare/noise-twin.$cxx.err ||
    echo "compare noise-twin.$cxx failed" >> failed.txt
done
