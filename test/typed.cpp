#include <mustache/mustache.hpp>

#include <gtest/gtest.h>

#include <expected>
#include <optional>
#include <string>
#include <vector>

namespace {

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

}
