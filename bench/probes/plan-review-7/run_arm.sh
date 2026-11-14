#!/bin/sh
set -e
apt-get update > /work/apt.log 2>&1
apt-get install -y g++-16-aarch64-linux-gnu qemu-user >> /work/apt.log 2>&1
cd /work
aarch64-linux-gnu-g++-16 -std=c++26 -O2 -Wall -Wextra -static -I/src/include room.cpp -o room_gcc_neon
echo "== room_gcc_neon"; qemu-aarch64 ./room_gcc_neon || echo "exit $?"
