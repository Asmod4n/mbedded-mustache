# One flag line for every arm of one compiler: the flags of bench/build.sh at -O2.
flags_for() {
  case $1 in
    g++-16) echo "g++-16 -std=c++26 -freflection -O2 -march=x86-64-v4 -falign-functions=64 -falign-loops=64 -falign-jumps=64 -DNDEBUG" ;;
    clang++-23) echo "clang++-23 -std=c++26 -O2 -march=x86-64-v4 -falign-functions=64 -falign-loops=64 -DNDEBUG" ;;
  esac
}
# Arm names. H5 and H6 change what only the reflection render reads; clang++-23 has no reflection, so their cell there is an error cell.
ARMS="head h1 h2a twin h2b h5 h6 all"
COMPILERS="g++-16 clang++-23"
cells_of() {
  case $1 in
    g++-16) echo "$ARMS" ;;
    clang++-23) echo "head h1 h2a twin h2b all" ;;
  esac
}
