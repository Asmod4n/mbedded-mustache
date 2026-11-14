# One flag line for every arm of one compiler: the flags of bench/build.sh at -O2.
flags_for() {
  case $1 in
    g++-16) echo "g++-16 -std=c++26 -freflection -O2 -march=x86-64-v4 -falign-functions=64 -falign-loops=64 -falign-jumps=64 -DNDEBUG" ;;
    clang++-23) echo "clang++-23 -std=c++26 -O2 -march=x86-64-v4 -falign-functions=64 -falign-loops=64 -DNDEBUG" ;;
  esac
}
# Arm names. head is include/ of c5864bf; twin is a second copy of it (A/A); h1p, h2ap and bothp are H1', H2a' and both (patch files of this directory).
# Every arm builds with both compilers; only the run-time render is timed.
ARMS="head twin h1p h2ap bothp"
COMPILERS="g++-16 clang++-23"
cells_of() {
  echo "$ARMS"
}
