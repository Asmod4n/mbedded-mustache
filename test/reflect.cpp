#include <mustache/mustache.hpp>

#include <gtest/gtest.h>

#include "node.hpp"

#include <cstdint>
#include <flat_map>
#include <optional>
#include <variant>
#include <map>
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
  const std::expected<std::string, mustache::Error> r = mustache::render<Src>(page, capacity);
  return r ? *r : "<fault>";
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
  const Pair data{value, value};
  const std::expected<std::string, mustache::Error> r = mustache::render<Src>(data, media_type);
  return r ? *r : "<fault>";
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

class Account {
public:
  std::string name = "ada";

private:
  std::string password = "secret";
};

// A template is data, and whoever writes it must not reach what the
// type keeps private. Only a public member answers a key; a private
// member is as absent as a member that does not exist.
TEST(ReflectAccess, ReadsPublicMembersOnly)
{
  constexpr bool public_found = mustache::detail::member_named(^^Account, "name") != std::meta::info{};
  constexpr bool private_hidden = mustache::detail::member_named(^^Account, "password") == std::meta::info{};
  EXPECT_TRUE(public_found);
  EXPECT_TRUE(private_hidden);
  const std::expected<std::string, mustache::Error> r = mustache::render<"{{name}}">(Account{});
  ASSERT_TRUE(r);
  EXPECT_EQ(*r, "ada");
}

struct Labels {
  std::string                                           title = "outer";
  std::map<std::string, std::string>                    names;
  std::map<std::string, std::map<std::string, std::string>> nested;
  std::vector<std::string>                              tags;
};

// Mustache spec, sections.yml, name resolution steps 3 and 4: a hash is
// a context whose keys are names, and only a list is iterated. A
// std::map is a range in C++, but in a template it is a hash. A key the
// map lacks resolves in the next context outward, as in the run-time
// render.
TEST(ReflectMap, IsAContextAndNotAList)
{
  const Labels l{"outer", {{"title", "inner"}, {"lang", "de"}}, {{"a", {{"b", "deep"}}}}, {"x", "y"}};
  const auto r = [&l]<mustache::fixed_string Src>() {
    const std::expected<std::string, mustache::Error> got = mustache::render<Src>(l);
    return got ? *got : std::string("<fault>");
  };
  EXPECT_EQ(r.template operator()<"{{#names}}{{title}}/{{lang}}{{/names}}">(), "inner/de");
  EXPECT_EQ(r.template operator()<"{{#nested}}{{title}}{{/nested}}">(), "outer");
  EXPECT_EQ(r.template operator()<"{{#nested}}{{a}}{{/nested}}">(), "<fault>");
  EXPECT_EQ(r.template operator()<"{{names.lang}} {{nested.a.b}} {{names.none}}.">(), "de deep .");
  EXPECT_EQ(r.template operator()<"{{#tags}}[{{.}}]{{/tags}}">(), "[x][y]");
  EXPECT_EQ(r.template operator()<"{{^names}}empty{{/names}}">(), "");

  const Labels empty{};
  const std::expected<std::string, mustache::Error> e = mustache::render<"{{#names}}x{{/names}}{{^names}}empty{{/names}}">(empty);
  ASSERT_TRUE(e);
  EXPECT_EQ(*e, "empty");
}

struct Fixed {
  std::flat_map<std::string_view, std::string_view> names;
};

constexpr Fixed
fixed()
{
  return Fixed{{{"title", "Hello"}, {"lang", "en"}}};
}

// A map that is known at compile time is read at compile time: the
// page holds the values, and no lookup is left for the run time.
TEST(ReflectMap, KnownAtCompileTimeIsFilledIn)
{
  constexpr auto page = mustache::static_page_of<"{{#names}}{{title}}/{{lang}}{{/names}}", fixed>;
  EXPECT_EQ(std::string_view(page.data()), "Hello/en");
}

struct Cell {
  std::variant<std::string, int64_t, std::monostate> value;
  std::optional<int>                                 count;
  std::optional<std::string>                         note;
};

// A variant renders the value it holds, as the run-time render does: text
// as text, a number by std::to_chars, std::monostate as nothing. An
// optional renders by the rule of its value type, numbers included.
TEST(ReflectValue, VariantAndOptionalRenderTheirValue)
{
  const auto r = [](const Cell &c) {
    const std::expected<std::string, mustache::Error> got =
        mustache::render<"[{{value}}|{{count}}|{{note}}]{{#value}}+{{/value}}{{^count}}-{{/count}}">(c);
    return got ? *got : std::string("<fault>");
  };
  EXPECT_EQ(r(Cell{std::string("a<b"), 3, "n"}), "[a&lt;b|3|n]+");
  EXPECT_EQ(r(Cell{int64_t{-42}, std::nullopt, std::nullopt}), "[-42||]+-");
  EXPECT_EQ(r(Cell{std::monostate{}, 0, std::nullopt}), "[|0|]");
}

// A document of standard types, a variant of maps, lists and scalars,
// renders the same bytes through the reflection render and through the
// run-time render.
TEST(ReflectValue, ADocumentRendersAsAtRunTime)
{
  NodeMap item1{{"name", Node{std::string("one")}}, {"n", Node{int64_t{1}}}};
  NodeMap item2{{"name", Node{std::string("<two>")}}};
  const Node doc{NodeMap{{"title", Node{std::string("T")}},
                         {"items", Node{NodeList{Node{item1}, Node{item2}}}},
                         {"flag", Node{true}},
                         {"none", Node{nullptr}}}};
  constexpr std::string_view source =
      "{{title}}:{{#items}}<{{name}}{{#n}}={{n}}{{/n}}>{{/items}}{{#flag}}!{{/flag}}{{^none}}~{{/none}}{{missing}}";
  const std::expected<std::string, mustache::Error> reflected =
      mustache::render<"{{title}}:{{#items}}<{{name}}{{#n}}={{n}}{{/n}}>{{/items}}{{#flag}}!{{/flag}}{{^none}}~{{/none}}{{missing}}">(doc);
  const std::expected<mustache::Template, mustache::Error> t = mustache::compile(source);
  ASSERT_TRUE(t);
  const std::expected<std::string, mustache::Error> walked = mustache::render(*t, doc);
  ASSERT_TRUE(reflected);
  ASSERT_TRUE(walked);
  EXPECT_EQ(*reflected, *walked);
  EXPECT_EQ(*reflected, "T:<one=1><&lt;two&gt;>!~");
}

}
