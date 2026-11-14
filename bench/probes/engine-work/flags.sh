# Shared settings of every script of the probe.
# One flag line for each compiler: the flags of bench/build.sh at -O2.
flags_for() {
  case $1 in
    g++-16) echo "g++-16 -std=c++26 -freflection -O2 -march=x86-64-v4 -falign-functions=64 -falign-loops=64 -falign-jumps=64 -DNDEBUG" ;;
    clang++-23) echo "clang++-23 -std=c++26 -O2 -march=x86-64-v4 -falign-functions=64 -falign-loops=64 -DNDEBUG" ;;
  esac
}
# Arms. main is include/ of 5d3c154. twin is a second copy of it (A/A). main8192 is main with 8192 as the initial_capacity argument
# of render. e1, e2 and e3a are the patch files of this directory. e3a1024 and e3a8192 are e3a with 1024 and 8192 as the argument.
# all is e1, e2 and e3a with 1024.
ARMS="main twin main8192 e1 e2 e3a1024 e3a8192 all"
VARIANTS="main twin e1 e2 e3a all"
EXTRA_VARIANTS="main e1 e2 e3a all"
COMPILERS="g++-16 clang++-23"
PAGES="p20 p200"
include_of() {
  case $1 in
    main8192) echo main ;;
    e3a1024|e3a8192) echo e3a ;;
    *) echo "$1" ;;
  esac
}
capacity_of() {
  case $1 in
    main8192|e3a8192) echo 8192 ;;
    *) echo 1024 ;;
  esac
}
rows_of() {
  case $1 in
    p20) echo 20 ;;
    p200) echo 200 ;;
  esac
}
