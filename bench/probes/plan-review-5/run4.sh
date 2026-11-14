g++-16 -std=c++26 -freflection -O2 -I/src/include /w/twice.cpp -o /w/twice 2>&1 | grep -E "error" | head -5; /w/twice
(timeout 240 sh -c 'apt-get update >/w/res/apt.log 2>&1 && apt-get install -y g++-16-aarch64-linux-gnu qemu-user >>/w/res/apt.log 2>&1') || { echo "apt failed"; tail -3 /w/res/apt.log; exit 0; }
aarch64-linux-gnu-g++-16 -std=c++26 -O2 -static -Wall -Wextra -I/src/include /w/bound.cpp -o /w/bound-neon && qemu-aarch64 /w/bound-neon | awk '{print $2, $5, $9}' | sort | uniq -c | sort -rn | head -40
