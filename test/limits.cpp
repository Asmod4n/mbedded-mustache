#include <mustache/mustache.hpp>

#include "node.hpp"

#include <gtest/gtest.h>

#include <sys/mman.h>

#include <cstddef>
#include <expected>
#include <string>
#include <string_view>

namespace {

std::expected<std::string, mustache::Error>
rendered(const std::string_view source, const Node &data, const mustache::Partials &partials = {})
{
  const std::expected<mustache::Template, mustache::Error> t = mustache::compile(source);
  if (!t) return std::unexpected(t.error());
  return mustache::render(*t, data, partials);
}

// A template over kSourceMax is refused before a byte of it is read. The
// source is a mapping of zero pages that the kernel gives only when a
// page is read, so the test costs no memory.
TEST(Limits, RefusesASourceOverTheLimit)
{
  const size_t size = mustache::kSourceMax + 1;
  void *const pages = mmap(nullptr, size, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
  ASSERT_NE(pages, MAP_FAILED);
  const std::expected<mustache::Template, mustache::Error> t =
      mustache::compile(std::string_view(static_cast<const char *>(pages), size));
  munmap(pages, size);
  ASSERT_FALSE(t);
  EXPECT_EQ(t.error().fault, mustache::Fault::over_limit);
  EXPECT_EQ(t.error().asked, size);
  EXPECT_EQ(t.error().allowed, mustache::kSourceMax);
}

// A Set Delimiter tag may name delimiters of up to kDelimiterMax bytes.
TEST(Limits, RefusesADelimiterOverTheLimit)
{
  const std::string at_limit(mustache::kDelimiterMax, '<');
  const std::string over_limit(mustache::kDelimiterMax + 1, '<');
  EXPECT_TRUE(mustache::compile("{{=" + at_limit + " >>=}}"));
  const std::expected<mustache::Template, mustache::Error> t = mustache::compile("ab{{=" + over_limit + " >>=}}");
  ASSERT_FALSE(t);
  EXPECT_EQ(t.error().fault, mustache::Fault::parse);
  EXPECT_EQ(t.error().what, "a delimiter is longer than 16 bytes");
}

// A partial that includes itself stops at the partial depth of 64 with
// too_deep, before it takes the stack.
TEST(Limits, StopsPartialsDeeperThanTheLimit)
{
  const std::expected<mustache::Template, mustache::Error> self = mustache::compile("x{{>self}}");
  ASSERT_TRUE(self);
  const mustache::Partials partials{{"self", *self}};
  const std::expected<std::string, mustache::Error> r = rendered("{{>self}}", Node{NodeMap{}}, partials);
  ASSERT_FALSE(r);
  EXPECT_EQ(r.error().fault, mustache::Fault::too_deep);
}

// Two nested sections over a list of 5000 run the inner body 25 million
// times. The work limit of 2^24 stops the render before it ends.
TEST(Limits, StopsARenderOverTheWorkLimit)
{
  NodeList many(5000, Node{true});
  const Node data{NodeMap{{"a", Node{std::move(many)}}}};
  const std::expected<std::string, mustache::Error> r = rendered("{{#a}}{{#a}}x{{/a}}{{/a}}", data);
  ASSERT_FALSE(r);
  EXPECT_EQ(r.error().fault, mustache::Fault::over_work);
  EXPECT_EQ(r.error().allowed, mustache::kMaxRenderScore);
  EXPECT_GT(r.error().asked, mustache::kMaxRenderScore);
}

// A refused template names the problem and the byte offset where the
// parser found it.
TEST(Limits, NamesTheOffsetOfAParseError)
{
  struct Case {
    std::string_view source;
    std::string_view what;
    size_t           at;
  };
  for (const Case c : {Case{"ab{{x", "unclosed {{", 2}, Case{"ab{{{x}}", "unclosed {{{", 2},
                       Case{"ab{{/x}}", "a closing tag without an opening tag", 2},
                       Case{"{{#a}}{{/b}}", "a closing tag names another section than the one it closes", 6},
                       Case{"ab{{#a}}cd", "an unclosed section", 2}, Case{"ab{{}}", "empty tag", 2}}) {
    SCOPED_TRACE(c.source);
    const std::expected<mustache::Template, mustache::Error> t = mustache::compile(c.source);
    ASSERT_FALSE(t);
    EXPECT_EQ(t.error().fault, mustache::Fault::parse);
    EXPECT_EQ(t.error().what, c.what);
    EXPECT_EQ(t.error().asked, c.at);
  }
}

}
