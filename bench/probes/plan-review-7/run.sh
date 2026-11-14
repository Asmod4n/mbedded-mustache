#!/bin/sh
set -e
cd /work
I=/src/include
g++-16 -std=c++26 -O2 -Wall -Wextra -march=x86-64-v3 -I$I room.cpp -o room_gcc_avx2 &
clang++-23 -std=c++26 -O2 -Wall -Wextra -march=x86-64-v3 -I$I room.cpp -o room_clang_avx2 &
g++-16 -std=c++26 -O2 -Wall -Wextra -I$I room.cpp -o room_gcc_scalar &
clang++-23 -std=c++26 -O2 -Wall -Wextra -I$I room.cpp -o room_clang_scalar &
wait
for b in room_gcc_avx2 room_clang_avx2 room_gcc_scalar room_clang_scalar; do echo "== $b"; ./$b || echo "exit $?"; done
