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

struct Pair {
  std::string x;
  std::string y;
};

template <mustache::fixed_string Src>
std::string
rendered_as(const std::string_view media_type, const std::string &value)
{
  mustache::std_host::Host h;
  const Pair data{value, value};
  if (mustache::render<Src>(h, data, media_type) != mustache::Fault::none) return "<fault>";
  return h.answer;
}

// The reflection render compiles a template once for each media type
// it can escape for, and picks one by the type the caller negotiated.
TEST(ReflectMediaType, EscapesByTheNegotiatedType)
{
  EXPECT_EQ(rendered_as<"<a href=\"{{x}}\">{{y}}</a>">("text/html", "'"), "<a href=\"&#39;\">&#39;</a>");
  EXPECT_EQ(rendered_as<"<a b=\"{{x}}\">{{y}}</a>">("application/xml", "'"), "<a b=\"&apos;\">&apos;</a>");
  EXPECT_EQ(rendered_as<"{\"a\":\"{{x}}\"}">("application/json; charset=utf-8", "\"\n"), "{\"a\":\"\\\"\\n\"}");
  EXPECT_EQ(rendered_as<"var a = '{{x}}';">("text/javascript", "'"), "var a = '\\x27';");
  EXPECT_EQ(rendered_as<"a { content: '{{x}}' }">("text/css", "'"), "a { content: '\\27 ' }");
}

// A type with no escaper renders nothing and gives a fault.
TEST(ReflectMediaType, RefusesATypeWithoutEscaper)
{
  EXPECT_EQ(rendered_as<"<p>{{x}}</p>">("image/svg+xml", "v"), "<fault>");
  EXPECT_EQ(rendered_as<"<p>{{x}}</p>">("text/plain", "v"), "<fault>");
}

}
