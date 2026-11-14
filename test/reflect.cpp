#include <mustache/mustache.hpp>

#include <gtest/gtest.h>

#include "node.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory_resource>
#include <flat_map>
#include <optional>
#include <sstream>
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
  EXPECT_EQ(rendered_as<".B {{x}}\n{{y}}\n">("text/troff", ".a"), ".B \\&.a\n\\&.a\n");
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

struct Title {
  std::string_view title;
};

constexpr Title
marked_title()
{
  return Title{"a<b & \"c\""};
}

constexpr Title
long_title()
{
  return Title{"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\""};
}

// The page of static_page has the exact length of the output, and its
// room ends at the end of the whole buffer. A room that ended at the
// exact length would refuse every escaped value, because the check
// counts the bytes past the written count, and the page would lose the
// values without a fault. The escaped value must stand whole in the
// page, also where its escape is longer than the bytes past the count.
// The html escape writes whole words past the count, so the test reads
// the page up to the length that static_length gives, not up to a NUL.
TEST(ReflectPage, StaticPageHoldsTheEscapedValue)
{
  constexpr auto marked = mustache::static_page_of<"[{{title}}]", marked_title>;
  constexpr size_t marked_length = mustache::static_length<"[{{title}}]", marked_title>();
  EXPECT_EQ(std::string_view(marked.data(), marked_length), "[" + escaped_by_rule(marked_title().title) + "]");
  constexpr auto long_page = mustache::static_page_of<"[{{title}}]", long_title>;
  constexpr size_t long_length = mustache::static_length<"[{{title}}]", long_title>();
  EXPECT_EQ(std::string_view(long_page.data(), long_length), "[" + escaped_by_rule(long_title().title) + "]");
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

// render_to is the append form of the reflection render, with the media
// type and without it. The initial capacity and the media type are two
// arguments that overload resolution does not mix up.
TEST(ReflectRenderTo, AppendsWhatRenderReturns)
{
  const Pair data{"a<b", "'"};
  std::string out = "head:";
  ASSERT_TRUE(mustache::render_to<"{{x}}|{{y}}">(out, data));
  EXPECT_EQ(out, "head:a&lt;b|&#39;");
  out = "head:";
  ASSERT_TRUE(mustache::render_to<"{{x}}|{{y}}">(out, data, 0));
  EXPECT_EQ(out, "head:a&lt;b|&#39;");
  out = "head:";
  ASSERT_TRUE(mustache::render_to<"{\"a\":\"{{x}}\"}">(out, data, "application/json"));
  EXPECT_EQ(out, "head:{\"a\":\"a<b\"}");
  out = "head:";
  ASSERT_TRUE(mustache::render_to<"{{x}}">(out, data, std::string_view("text/html"), 0));
  EXPECT_EQ(out, "head:a&lt;b");
}

// An error leaves the string as it was: a type with no escaper, an
// initial capacity over the limit.
TEST(ReflectRenderTo, LeavesTheStringAsItWasOnAnError)
{
  const Pair data{"v", "w"};
  std::string out = "keep";
  const std::expected<void, mustache::Error> no_escaper = mustache::render_to<"<p>{{x}}</p>">(out, data, "text/plain");
  ASSERT_FALSE(no_escaper);
  EXPECT_EQ(no_escaper.error().fault, mustache::Fault::parse);
  EXPECT_EQ(out, "keep");
  const std::expected<void, mustache::Error> over =
      mustache::render_to<"<p>{{x}}</p>">(out, data, std::numeric_limits<size_t>::max());
  ASSERT_FALSE(over);
  EXPECT_EQ(over.error().fault, mustache::Fault::over_limit);
  EXPECT_EQ(out, "keep");
  const std::expected<void, mustache::Error> over_media =
      mustache::render_to<"<p>{{x}}</p>">(out, data, "text/html", std::numeric_limits<size_t>::max());
  ASSERT_FALSE(over_media);
  EXPECT_EQ(over_media.error().fault, mustache::Fault::over_limit);
  EXPECT_EQ(out, "keep");
}

// A string of another allocator takes the render with one copy.
TEST(ReflectRenderTo, AppendsToAStringOfAnotherAllocator)
{
  const Pair data{"v", "w"};
  std::pmr::string out("keep:");
  ASSERT_TRUE(mustache::render_to<"{{x}}{{y}}">(out, data));
  ASSERT_TRUE(mustache::render_to<"{{x}}{{y}}">(out, data, "text/html"));
  EXPECT_EQ(std::string_view(out), "keep:vwvw");
}

// A vector of char and of std::byte take the bytes the same way.
TEST(ReflectRenderTo, AppendsToAByteVector)
{
  const Pair data{"a<b", "w"};
  std::vector<char> chars = {'k'};
  ASSERT_TRUE(mustache::render_to<"{{x}}|{{y}}">(chars, data));
  ASSERT_TRUE(mustache::render_to<"{{x}}">(chars, data, "text/html"));
  EXPECT_EQ(std::string(chars.begin(), chars.end()), "ka&lt;b|wa&lt;b");
  std::vector<std::byte> octets = {std::byte{'k'}};
  ASSERT_TRUE(mustache::render_to<"{{y}}">(octets, data, 0));
  ASSERT_EQ(octets.size(), 2U);
  EXPECT_EQ(octets.at(1), std::byte{'w'});
  const std::expected<void, mustache::Error> refused = mustache::render_to<"{{y}}">(octets, data, "text/plain");
  ASSERT_FALSE(refused);
  EXPECT_EQ(octets.size(), 2U);
}

// A stream takes the bytes of a reflection render, with the media type
// and without it. A type with no escaper writes nothing.
TEST(ReflectRenderTo, WritesToAStream)
{
  const Pair data{"a<b", "w"};
  std::ostringstream out;
  ASSERT_TRUE(mustache::render_to<"{{x}}|{{y}}">(out, data));
  ASSERT_TRUE(mustache::render_to<"{{x}}">(out, data, "application/json"));
  ASSERT_TRUE(mustache::render_to<"{{y}}">(out, data, 0));
  EXPECT_EQ(out.str(), "a&lt;b|wa<bw");
  const std::expected<void, mustache::Error> refused = mustache::render_to<"{{y}}">(out, data, "text/plain");
  ASSERT_FALSE(refused);
  EXPECT_EQ(out.str(), "a&lt;b|wa<bw");
}

}
