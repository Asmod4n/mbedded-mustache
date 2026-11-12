# The specifications this tree implements

Verbatim copies. They are here so that a claim about a rule is read
rather than remembered, and so that a session with no network can still
read one.

Nothing here is edited. A file that differs from its source is a bug in
this directory, not a local decision.

    grep -n "Null is falsey" refs/mustache-spec/sections.yml

| directory | source | what it decides here |
|---|---|---|
| mustache-spec | `https://github.com/mustache/spec`, `specs/` and `LICENSE` at e8ec001 | the Mustache language: interpolation, sections, inverted sections, comments, partials, Set Delimiter, blocks and parents |
| mandoc | `https://mandoc.bsd.lv`, mandoc 1.14.6: `roff.7`, `man.7`, `mandoc_char.7` and `LICENSE` | the text lines of a man page: which characters mean something, how they are escaped, and the sentence spacing |
| rfc4263.txt | `https://www.rfc-editor.org/rfc/rfc4263.txt` | the registered media type `text/troff` |
