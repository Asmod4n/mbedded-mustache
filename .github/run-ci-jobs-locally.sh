#!/bin/sh
# Runs every x86-64 job of .github/workflows/ci.yml in a local container,
# one after the other, and exits non-zero if one of them fails.
# The aarch64 jobs run in the CI only.
set -u
root=$(cd "$(dirname "$0")/.." && pwd)
image=${CI_IMAGE:-localhost/gcc16-gtest}
jobs=$(python3 -I -c '
import sys, yaml
for j in yaml.safe_load(open(sys.argv[1]))["jobs"]["test"]["strategy"]["matrix"]["include"]:
    if j["runner"] == "ubuntu-24.04":
        print("\t".join([j["name"], j["cxx"], j["cc"], str(j["std"]), j["flags"]]))
' "$root/.github/workflows/ci.yml") || exit 2
failed=0
tab=$(printf '\t')
while IFS="$tab" read -r name cxx cc std flags; do
  if podman run --rm -v "$root:/w:ro" -w /w "$image" sh -c "
      cmake -S . -B /tmp/build -G Ninja -DCMAKE_CXX_COMPILER=$cxx -DCMAKE_C_COMPILER=$cc \
        -DCMAKE_CXX_STANDARD=$std '-DCMAKE_CXX_FLAGS=$flags' >/tmp/configure.log 2>&1 || { cat /tmp/configure.log; exit 1; }
      cmake --build /tmp/build -j3 && ctest --test-dir /tmp/build --output-on-failure" >"/tmp/ci-local-$$.log" 2>&1; then
    echo "pass  $name ($(grep -o '[0-9]*% tests passed.*' "/tmp/ci-local-$$.log" | tail -1 || true))"
  else
    failed=1
    echo "FAIL  $name"
    tail -30 "/tmp/ci-local-$$.log"
  fi
  rm -f "/tmp/ci-local-$$.log"
done <<EOF
$jobs
EOF
exit $failed
