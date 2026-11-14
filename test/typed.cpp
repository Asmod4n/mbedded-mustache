#include <mustache/mustache.hpp>

#include <gtest/gtest.h>

#include <cstddef>
#include <expected>
#include <limits>
#include <memory_resource>
#include <map>
#include <optional>
#include <unordered_map>
#include <variant>
#include <string>
#include <vector>

// The structs have external linkage: Boost.PFR reads the names of the
// members through a declaration of each struct, and cl.exe refuses that
// declaration for a type in an anonymous namespace (C7631).
namespace typed {

struct Row {
  std::string a;
  std::string b;
};

struct Page {
  std::string        single;
  std::vector<Row>   rows;
  std::optional<int> count;
  bool               flag = false;
};

// A typed template is compiled for one aggregate type: every key is
// resolved to a member once, and the render reads the member without a
// search. With reflection the members come from std::meta; without it,
// from Boost.PFR, so clang has the same path.
TEST(Typed, RendersAStructWithoutASearch)
{
  const Page page{"top & co", {{"a1", "b1"}, {"a2", "b2"}}, 7, true};
  const std::expected<mustache::TypedTemplate<Page>, mustache::Error> t = mustache::compile<Page>(
      "{{single}}:{{#rows}}[{{a}}/{{b}}/{{single}}]{{/rows}}{{^rows}}none{{/rows}}{{count}}{{#flag}}!{{/flag}}");
  ASSERT_TRUE(t);
  const std::expected<std::string, mustache::Error> r = mustache::render(*t, page);
  ASSERT_TRUE(r);
  EXPECT_EQ(*r, "top &amp; co:[a1/b1/top &amp; co][a2/b2/top &amp; co]7!");
}

// A key that names no member is found when the template is compiled,
// not when it renders.
TEST(Typed, RefusesAKeyThatNamesNoMember)
{
  const std::expected<mustache::TypedTemplate<Page>, mustache::Error> t = mustache::compile<Page>("{{singel}}");
  ASSERT_FALSE(t);
  EXPECT_EQ(t.error().fault, mustache::Fault::parse);
}

// The media type picks the escaper of a typed template as of any other.
TEST(Typed, EscapesByTheMediaType)
{
  const Page page{"\"\n", {}, std::nullopt, false};
  const std::expected<mustache::TypedTemplate<Page>, mustache::Error> t =
      mustache::compile<Page>("{\"a\":\"{{single}}\"}", "application/json");
  ASSERT_TRUE(t);
  const std::expected<std::string, mustache::Error> r = mustache::render(*t, page);
  ASSERT_TRUE(r);
  EXPECT_EQ(*r, "{\"a\":\"\\\"\\n\"}");
}

struct Mixed {
  std::string                                  title;
  std::unordered_map<std::string, std::string> labels;
  std::variant<std::string, int64_t>           cell;
  std::optional<Row>                           extra;
  std::vector<std::variant<Row, std::string>>  items;
};

// Maps, variants and an optional struct are read at render time; the
// struct members around them are still read by their index.
TEST(Typed, ReadsMapsVariantsAndOptionalStructs)
{
  const Mixed m{"T", {{"lang", "de"}}, int64_t{42}, Row{"x", "y"}, {Row{"r1", "s1"}, std::string("plain")}};
  const std::expected<mustache::TypedTemplate<Mixed>, mustache::Error> t = mustache::compile<Mixed>(
      "{{title}}|{{labels.lang}}|{{#labels}}{{lang}}{{title}}{{/labels}}|{{cell}}|{{#extra}}{{a}}{{b}}{{title}}{{/extra}}|"
      "{{#items}}<{{a}}>{{/items}}");
  ASSERT_TRUE(t);
  const std::expected<std::string, mustache::Error> r = mustache::render(*t, m);
  ASSERT_TRUE(r);
  EXPECT_EQ(*r, "T|de|deT|42|xyT|<r1><>");
}

// A partial is a template of its own; it reads the data at render time
// on the stack of the typed template that includes it.
TEST(Typed, RendersPartials)
{
  const Page page{"top", {{"a1", "b1"}}, std::nullopt, false};
  const std::expected<mustache::Template, mustache::Error> row = mustache::compile("[{{a}}{{single}}]");
  ASSERT_TRUE(row);
  const mustache::Partials partials{{"row", *row}};
  const std::expected<mustache::TypedTemplate<Page>, mustache::Error> t = mustache::compile<Page>("{{#rows}}{{>row}}{{/rows}}");
  ASSERT_TRUE(t);
  const std::expected<std::string, mustache::Error> r = mustache::render(*t, page, partials);
  ASSERT_TRUE(r);
  EXPECT_EQ(*r, "[a1top]");
}

// render_to is the append form of render for a typed template: the
// bytes that render returns are appended behind the bytes of the string.
TEST(TypedRenderTo, AppendsWhatRenderReturns)
{
  const Page page{"top & co", {{"a1", "b1"}, {"a2", "b2"}}, 7, true};
  const std::expected<mustache::TypedTemplate<Page>, mustache::Error> t = mustache::compile<Page>("{{single}}:{{#rows}}[{{a}}/{{b}}]{{/rows}}{{count}}");
  ASSERT_TRUE(t);
  std::string out = "head:";
  ASSERT_TRUE(mustache::render_to(out, *t, page));
  EXPECT_EQ(out, "head:top &amp; co:[a1/b1][a2/b2]7");
}

// The string of the caller is as it was after an error.
TEST(TypedRenderTo, LeavesTheStringAsItWasOnAnError)
{
  const Page page{"x", {}, std::nullopt, false};
  const std::expected<mustache::TypedTemplate<Page>, mustache::Error> t = mustache::compile<Page>("{{single}}");
  ASSERT_TRUE(t);
  std::string out = "keep";
  const std::expected<void, mustache::Error> r =
      mustache::render_to(out, *t, page, {}, std::numeric_limits<size_t>::max());
  ASSERT_FALSE(r);
  EXPECT_EQ(r.error().fault, mustache::Fault::over_limit);
  EXPECT_EQ(out, "keep");
}

// A string of another allocator takes the render with one copy.
TEST(TypedRenderTo, AppendsToAStringOfAnotherAllocator)
{
  const Page page{"x", {}, std::nullopt, false};
  const std::expected<mustache::TypedTemplate<Page>, mustache::Error> t = mustache::compile<Page>("<{{single}}>");
  ASSERT_TRUE(t);
  std::pmr::string out("keep:");
  ASSERT_TRUE(mustache::render_to(out, *t, page));
  EXPECT_EQ(std::string_view(out), "keep:<x>");
}

// A vector of char and of std::byte take the bytes the same way.
TEST(TypedRenderTo, AppendsToAByteVector)
{
  const Page page{"x", {}, std::nullopt, false};
  const std::expected<mustache::TypedTemplate<Page>, mustache::Error> t = mustache::compile<Page>("<{{single}}>");
  ASSERT_TRUE(t);
  std::vector<char> chars = {'k'};
  ASSERT_TRUE(mustache::render_to(chars, *t, page));
  EXPECT_EQ(std::string(chars.begin(), chars.end()), "k<x>");
  std::vector<std::byte> octets = {std::byte{'k'}};
  ASSERT_TRUE(mustache::render_to(octets, *t, page));
  ASSERT_EQ(octets.size(), 4U);
  EXPECT_EQ(octets.at(1), std::byte{'<'});
  const std::expected<void, mustache::Error> over =
      mustache::render_to(octets, *t, page, {}, std::numeric_limits<size_t>::max());
  ASSERT_FALSE(over);
  EXPECT_EQ(octets.size(), 4U);
}

}
