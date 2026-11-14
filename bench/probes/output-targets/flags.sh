# One flag line for every compiler: the flags of bench/build.sh at -O2.
flags_for() {
  case $1 in
    g++-16) echo "g++-16 -std=c++26 -freflection -O2 -march=x86-64-v4 -falign-functions=64 -falign-loops=64 -falign-jumps=64 -DNDEBUG" ;;
    clang++-23) echo "clang++-23 -std=c++26 -O2 -march=x86-64-v4 -falign-functions=64 -falign-loops=64 -DNDEBUG" ;;
  esac
}
# fresh is form 1, reused is form 2, twin is fresh built again (the A/A arm).
ARMS="fresh twin reused"
NOISE_ARMS="fresh twin"
COMPILERS="g++-16 clang++-23"
