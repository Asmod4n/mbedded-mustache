#!/bin/sh
cd /w
for cxx in g++-16 clang++-23; do
  $cxx -std=c++26 -O2 -Wall -Wextra -Werror small.cpp -o b/small-$cxx > b/small-$cxx.log 2>&1 && { b/small-$cxx; echo "$cxx optional<T&> and function_ref: compiled, exit $?"; } || { echo "$cxx small FAIL"; head -4 b/small-$cxx.log; }
  $cxx -std=c++26 -O2 -DVARIANT_OF_REFERENCES -fsyntax-only small.cpp > b/var-$cxx.log 2>&1 && echo "$cxx variant of references: compiled" || { echo "$cxx variant of references: does not compile"; grep -m1 'error' b/var-$cxx.log; }
done
apt-get update > res/apt3.log 2>&1
apt-get install -y g++-16-aarch64-linux-gnu >> res/apt3.log 2>&1 || { echo "apt cross failed"; tail -3 res/apt3.log; exit 0; }
for level in -O2 -Os; do
  aarch64-linux-gnu-g++-16 -std=c++26 $level -Wall -Wextra -Werror -S neon.cpp -o b/neon-g++$level.s > b/neon-g++$level.log 2>&1 || { echo "g++ aarch64 $level FAIL"; head -5 b/neon-g++$level.log; }
  clang++-23 --target=aarch64-linux-gnu -std=c++26 $level -Wall -Wextra -Werror -S neon.cpp -o b/neon-clang$level.s > b/neon-clang$level.log 2>&1 || { echo "clang aarch64 $level FAIL"; head -5 b/neon-clang$level.log; }
  for c in g++ clang; do echo "== $c $level"; grep -vE '^\s*\.|^\s*$|^//' b/neon-$c$level.s | head -30; done
done
chmod -R a+rX b
