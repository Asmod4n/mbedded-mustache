"""Writes the order of the binaries of one round: the lines of FILE in a random order that SEED and ROUND fix.

Usage: python3 -I shuffle.py FILE SEED ROUND
"""
import random
import sys

names = open(sys.argv[1]).read().split()
random.Random(int(sys.argv[2]) * 1000 + int(sys.argv[3])).shuffle(names)
print("\n".join(names))
