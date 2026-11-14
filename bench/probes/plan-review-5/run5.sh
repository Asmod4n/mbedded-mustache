(apt-get update >/w/res/apt.log 2>&1 && apt-get install -y qemu-user >>/w/res/apt.log 2>&1) || { echo "apt failed"; exit 0; }
qemu-aarch64 /w/bound-neon > /w/res/neon.txt; grep -E "^n=( 0| 1|15|16|17|24|31|32|33|47|48|96) |ALL|context" /w/res/neon.txt
