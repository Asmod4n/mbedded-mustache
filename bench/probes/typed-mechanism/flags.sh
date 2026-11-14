# The flag line of each arm. R and R2 take the members from C++26 reflection, P from Boost.PFR.
align="-O2 -march=x86-64-v4 -falign-functions=64 -falign-loops=64 -falign-jumps=64 -DNDEBUG -Wall -Wextra -Werror"
flags_reflection="g++-16 -std=c++26 -freflection $align"
flags_pfr="g++-16 -std=c++23 $align"
