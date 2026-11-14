for cxx in g++-16 clang++-23; do for level in -O2 -Os; do for a in 0 1 2; do
  $cxx -std=c++26 $level -Wall -Wextra -Werror -DARM=$a /w/cost.cpp -o /w/cost$cxx$level-$a || echo FAIL $cxx $level $a
done; done; done
chmod a+rx /w/cost* ; mkdir -p /w/res; chmod a+rwx /w/res
cat > /w/res/user.sh <<'F'
for round in 1 2 3 4 5 6; do for b in g++-16-O2 g++-16-Os clang++-23-O2 clang++-23-Os; do
  if [ $((round % 2)) = 1 ]; then order="0 1 2"; else order="2 1 0"; fi
  for a in $order; do /w/cost$b-$a | cut -d' ' -f1 | sed "s/^/$b $a /" >> /w/res/cost.txt; done
done; done
F
rm -f /w/res/cost.txt
runuser -u nobody -- sh /w/res/user.sh
cat /proc/loadavg
