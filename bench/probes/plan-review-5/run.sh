set -e
g++-16 -std=c++26 -O2 -march=x86-64-v3 -Wall -Wextra -I/src/include /w/bound.cpp -o /w/bound-avx2
g++-16 -std=c++26 -O2 -Wall -Wextra -I/src/include /w/bound.cpp -o /w/bound-scalar
echo AVX2; /w/bound-avx2
echo SCALAR; /w/bound-scalar | tail -7
