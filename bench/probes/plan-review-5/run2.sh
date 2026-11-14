for cxx in g++-16 clang++-23; do
  $cxx -std=c++26 -O2 -Wall -Wextra -Werror /w/retry.cpp -o /w/retry-$cxx && /w/retry-$cxx
  $cxx -std=c++26 -O2 -D_GLIBCXX_ASSERTIONS -Wall -Wextra -Werror /w/retry.cpp -o /w/retrya-$cxx && /w/retrya-$cxx | tail -1
  for level in -O2 -Os; do for a in 0 1 2; do
    $cxx -std=c++26 $level -DARM=$a /w/cost.cpp -lbenchmark -lpthread -o /w/cost$cxx$level-$a || echo FAIL $cxx $level $a
  done; done
done
chmod a+rx /w/cost* ; mkdir -p /w/res; chmod a+rwx /w/res
cat > /w/res/user.sh <<'F'
for round in 1 2 3 4 5 6; do for b in g++-16-O2 g++-16-Os clang++-23-O2 clang++-23-Os; do
  if [ $((round % 2)) = 1 ]; then order="0 1 2"; else order="2 1 0"; fi
  for a in $order; do /w/cost$b-$a --benchmark_format=csv --benchmark_repetitions=3 --benchmark_min_time=0.1s 2>>/w/res/err | grep '^"arm"' | cut -d, -f4 | sed "s/^/$b $a /" >> /w/res/cost.txt; done
done; done
F
rm -f /w/res/cost.txt
runuser -u nobody -- sh /w/res/user.sh
grep -n "resize_and_overwrite" -A32 /usr/include/c++/16/bits/basic_string.tcc | grep -n "_Terminator\|noexcept\|_M_set_length\|catch" 
