#include "node.hpp"

#include <mustache/mustache.hpp>

#include <gtest/gtest.h>

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#ifndef MUSTACHE_MANDOC
#error the path of mandoc is set by CMake
#endif
#ifndef MUSTACHE_TROFF_DIR
#error the directory of the files of this test is set by CMake
#endif

namespace {

constexpr size_t kIndent = 7;
constexpr std::string_view kNoBreakSpace = "\xc2\xa0";

std::string
file_text(const std::string &path)
{
  std::ifstream in(path, std::ios::binary);
  std::stringstream s;
  s << in.rdbuf();
  return s.str();
}

// The text that mandoc shows for a man page whose section D holds the body.
// The bold of the headings is removed; the diagnostics go to a file.
struct Shown {
  std::vector<std::string> lines;
  std::string              diagnostics;
};

Shown
shown_by_mandoc(const std::string &body)
{
  const std::string dir = MUSTACHE_TROFF_DIR;
  {
    std::ofstream out(dir + "/page.7", std::ios::binary);
    out << ".TH T 7 2026-11-12\n.SH NAME\nt \\- t\n.SH D\n" << body;
  }
  const std::string command = std::string(MUSTACHE_MANDOC) + " -T utf8 -W all " + dir + "/page.7 > " + dir +
                              "/page.txt 2> " + dir + "/page.err";
  EXPECT_EQ(std::system(command.c_str()), 0);
  std::string text;
  const std::string raw = file_text(dir + "/page.txt");
  for (size_t i = 0; i < raw.size(); i++) {
    if (i + 1 < raw.size() && raw[i + 1] == '\b') {
      i += 1;
      continue;
    }
    text.push_back(raw[i]);
  }
  Shown shown;
  shown.diagnostics = file_text(dir + "/page.err");
  std::stringstream s(text);
  std::string line;
  bool in_section = false;
  while (std::getline(s, line)) {
    if (line == "D") {
      in_section = true;
      continue;
    }
    if (!in_section) continue;
    if (line.find("T(7)") != std::string::npos) break;
    shown.lines.push_back(line.starts_with(std::string(kIndent, ' ')) ? line.substr(kIndent) : line);
  }
  while (!shown.lines.empty() && shown.lines.back().find_first_not_of(' ') == std::string::npos) shown.lines.pop_back();
  return shown;
}

std::string
troff_of(const std::string &value)
{
  const std::expected<mustache::Template, mustache::Error> t = mustache::compile("{{x}}\n", "text/troff");
  EXPECT_TRUE(t);
  const NodeMap data{{"x", Node{value}}};
  const std::expected<std::string, mustache::Error> r = mustache::render(*t, data);
  EXPECT_TRUE(r);
  return r.value_or(std::string());
}

// mandoc is the second reader of the pages of refs/mandoc. For a value
// of printable ASCII it shows the value, with no diagnostic. A space at
// the end of a line is forced by a backslash, and shows as a no-break
// space.
TEST(TroffMandoc, ShowsEveryPrintableByteAtEachPosition)
{
  for (int b = 0x20; b < 0x7f; b++) {
    const std::string c(1, (char)b);
    for (const std::string &value : {c + "x", "x" + c + "x", "x" + c}) {
      SCOPED_TRACE(value);
      const Shown shown = shown_by_mandoc(troff_of(value));
      EXPECT_EQ(shown.diagnostics, "");
      ASSERT_EQ(shown.lines.size(), 1u);
      std::string want = value;
      if (want.back() == ' ') want.replace(want.size() - 1, 1, kNoBreakSpace);
      EXPECT_EQ(shown.lines.front(), want);
    }
  }
}

// A newline in a value is a space in the filled text; a sentence that
// ends at the end of a line does not get the wide space.
TEST(TroffMandoc, ShowsALineEndAsOneSpace)
{
  for (const std::string_view value_view : {"a\nb", "a.\nb", "a!\nb", "a?\nb", "a.)\nb", "a.]\nb", "a.'\nb", "a.\"\nb", "a\n.b"}) {
    const std::string value(value_view);
    SCOPED_TRACE(value);
    const Shown shown = shown_by_mandoc(troff_of(value));
    EXPECT_EQ(shown.diagnostics, "");
    ASSERT_EQ(shown.lines.size(), 1u);
    std::string want = value;
    want.replace(want.find('\n'), 1, " ");
    EXPECT_EQ(shown.lines.front(), want);
  }
}

}
