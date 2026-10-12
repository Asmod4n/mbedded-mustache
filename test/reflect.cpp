#include <mustache/reflect.hpp>
#include <mustache/std.hpp>

#include <gtest/gtest.h>

#include <string>
#include <string_view>
#include <vector>

namespace {

struct Row {
  std::string a;
  std::string b;
};

struct Page {
  std::string      single;
  std::vector<Row> rows;
};

std::string
escaped_by_rule(const std::string_view s)
{
  std::string r;
  for (const char c : s) {
    if (c == '&') r += "&amp;";
    else if (c == '<') r += "&lt;";
    else if (c == '>') r += "&gt;";
    else if (c == '"') r += "&quot;";
    else if (c == '\'') r += "&#39;";
    else r += c;
  }
  return r;
}

template <mustache::fixed_string Src>
std::string
rendered(const Page &page, const size_t capacity)
{
  mustache::std_host::Host h;
  if (mustache::render<Src>(h, page, capacity) != mustache::Fault::none) return "<fault>";
  return h.answer;
}

std::string
expected_rows(const Page &page)
{
  std::string r = escaped_by_rule(page.single) + "|";
  for (const Row &row : page.rows) r += "<" + escaped_by_rule(row.a) + ":" + escaped_by_rule(row.b) + ">";
  return r;
}

std::string
heap_held(const std::string &value)
{
  std::string s;
  s.reserve(100);
  s = value;
  return s;
}

// A list whose body holds only text and values is written in one batch
// per element: one capacity check, then every value without a branch. A
// value that needs escaping, or is longer than 32 bytes, makes the
// element render again on the checked path. Each test below takes one
// of those ways.
TEST(ReflectBatch, ValuesOfZeroToFortyBytes)
{
  Page page{"top", {}};
  for (size_t n = 0; n <= 40; n++) page.rows.push_back({std::string(n, 'a'), std::string(40 - n, 'b')});
  EXPECT_EQ(rendered<"{{single}}|{{#rows}}<{{a}}:{{b}}>{{/rows}}">(page, 64), expected_rows(page));
}

TEST(ReflectBatch, OneDirtyRowAmongCleanRows)
{
  const Page page{"<top>", {{"x", "y"}, {"a&b", "c"}, {"plain", std::string(31, 'q') + "\""}, {"z", "w"}}};
  EXPECT_EQ(rendered<"{{single}}|{{#rows}}<{{a}}:{{b}}>{{/rows}}">(page, 64), expected_rows(page));
}

TEST(ReflectBatch, ValuesHeldOnTheHeap)
{
  const Page page{"t",
                  {{heap_held(std::string(16, 'h')), heap_held("abc")}, {heap_held("<"), heap_held(std::string(15, 'k'))}}};
  EXPECT_EQ(rendered<"{{single}}|{{#rows}}<{{a}}:{{b}}>{{/rows}}">(page, 64), expected_rows(page));
}

// The checked render of a dirty row may grow the buffer, so the batch
// keeps no pointer into it past the first try.
TEST(ReflectBatch, DirtyRowThatGrowsTheBuffer)
{
  const Page page{"g", {{"ok", "ok"}, {std::string(2000, '&'), "x"}, {"ok", "ok"}}};
  EXPECT_EQ(rendered<"{{single}}|{{#rows}}<{{a}}:{{b}}>{{/rows}}">(page, 1), expected_rows(page));
}

// A raw value is not escaped and is not limited to 32 bytes. A batch
// reserves 32 bytes for each value, so a longer raw value makes the
// element render again on the checked path instead of writing into the
// room of the values after it.
TEST(ReflectBatch, RawValueOverThirtyTwoBytes)
{
  const Page page{"r", {{std::string(40, '<'), "b"}, {"s", "t"}}};
  std::string want = "r|";
  for (const Row &row : page.rows) want += "[" + row.a + "]";
  EXPECT_EQ(rendered<"{{single}}|{{#rows}}[{{{a}}}]{{/rows}}">(page, 64), want);
}

}
