"""Prints how an engine wrote each special character, read from its output of workloads 2 and 3.

Usage: python3 -I escapes.py OUTPUT_OF_WORKLOAD_2 OUTPUT_OF_WORKLOAD_3
The data holds & in "what happened & why", ' in "Today's news", < and > in "<b>markup</b>",
/ in "http://www.adobe.com" and " in the minus class of workload 2.
A character that no engine text carries is printed as "not in the output".
"""
import re
import sys


def found(pattern, text):
    match = re.search(pattern, text)
    return match.group(1) if match else "not in the output"


def main():
    two = open(sys.argv[1], encoding="utf-8").read()
    three = open(sys.argv[2], encoding="utf-8").read()
    print("&=" + found(r"what happened (\S+?) why", three))
    print("'=" + found(r"Today(\S+?)s news", three))
    print("<=" + found(r"with (\S+?)b", three))
    print(">=" + found(r"with \S+?b(\S+?)markup", three))
    print("/=" + found(r"http:(/|&#x2F;|&#47;)(?:/|&#x2F;|&#47;)www\.adobe\.com", two))
    quote = found(r"<td(?: )?class=(\S+?)minus", two)
    print('"=' + ("not in the output" if quote == '"' else quote))


main()
