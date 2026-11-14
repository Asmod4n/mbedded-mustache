#!/bin/sh
apt-get update > /w/res/apt2.log 2>&1
apt-get install -y libbenchmark-dev >> /w/res/apt2.log 2>&1 || { echo "apt failed"; exit 1; }
ARMS="51 63" sh /w/build.sh
sh /w/meas.sh run2 6 "g++-16 clang++-23" "-Os -O2" "1 50 51 60 63"
python3 /w/med.py /w/res/run2 1 > /w/res/run2/table.txt 2>&1
# std::optional<T &> and the preconditions of resize_and_overwrite, read in the library of the image
grep -n '__cpp_lib_optional\b\|__cpp_lib_optional ' /usr/include/c++/16/bits/version.h | head -5 > /w/res/optref.txt 2>&1
grep -n 'class optional<_Tp&>\|optional<_Tp&>' /usr/include/c++/16/optional | head -5 >> /w/res/optref.txt 2>&1
grep -n -B3 -A45 'resize_and_overwrite(const size_type __n, _Operation __op)' /usr/include/c++/16/bits/basic_string.tcc > /w/res/rao.txt 2>&1
grep -n -B12 -A6 'resize_and_overwrite' /usr/include/c++/16/bits/basic_string.h | head -80 >> /w/res/rao.txt 2>&1
