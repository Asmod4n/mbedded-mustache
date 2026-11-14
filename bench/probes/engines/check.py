"""Compares the output of an engine with the output of bench/render.cpp.

Usage: python3 -I check.py EXPECTED ACTUAL
Three levels, each stricter than the next one is loose:
  raw      the bytes agree.
  decoded  the bytes agree after both sides are decoded from HTML entities.
  tags     the decoded texts agree after the whitespace between two tags is removed,
           " />" is read as ">", "<tdclass=" is read as "<td class=", and trailing whitespace is removed.
Prints one line with the three results and the first difference of each level that differs.
Exit code 0 when the decoded texts agree, 1 otherwise.
"""
import html
import re
import sys


def first_difference(a, b):
    limit = min(len(a), len(b))
    offset = next((i for i in range(limit) if a[i] != b[i]), limit)
    line = a.count("\n", 0, offset) + 1
    return "offset %d line %d: expected %r actual %r" % (offset, line, a[max(0, offset - 20):offset + 40], b[max(0, offset - 20):offset + 40])


def tags_form(text):
    text = re.sub(r">\s+<", "><", text)
    text = text.replace(" />", ">")
    return text.replace("<tdclass=", "<td class=").rstrip()


def main():
    expected_path, actual_path = sys.argv[1:3]
    expected = open(expected_path, encoding="utf-8").read()
    actual = open(actual_path, encoding="utf-8").read()
    ex, ac = html.unescape(expected), html.unescape(actual)
    raw = expected == actual
    decoded = ex == ac
    tags = tags_form(ex) == tags_form(ac)
    word = lambda same: "same" if same else "different"
    print("raw=%s decoded=%s tags=%s" % (word(raw), word(decoded), word(tags)))
    if not raw:
        print("  raw first difference: " + first_difference(expected, actual))
    if not decoded:
        print("  decoded first difference: " + first_difference(ex, ac))
    if not tags:
        print("  tags first difference: " + first_difference(tags_form(ex), tags_form(ac)))
    return 0 if decoded else 1


sys.exit(main())
