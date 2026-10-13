#include <mustache/mustache.hpp>

#include "node.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <expected>
#include <string>
#include <string_view>

namespace {

using Map = NodeMap;

struct Rendered {
  mustache::Fault fault;
  std::string     answer;
};

Rendered
rendered(const std::string_view source, const Map &data, const size_t initial_capacity, const size_t max_capacity)
{
  const std::expected<mustache::Template, mustache::Error> t = mustache::compile(source);
  if (!t) return {mustache::Fault::parse, {}};
  std::expected<std::string, mustache::Error> r =
      mustache::Walk(nullptr, mustache::kMaxRenderScore, initial_capacity, max_capacity)
          .run(*t, mustache::detail::containers::value_of(data));
  if (!r) return {r.error().fault, {}};
  return {mustache::Fault::none, std::move(*r)};
}

std::string
repeated(const std::string_view s, const size_t n)
{
  std::string r;
  for (size_t i = 0; i < n; i++) r += s;
  return r;
}

// The string of the host starts at the initial capacity and doubles.
// A value that does not fit makes it grow before the copy. Every
// length around a doubled capacity has to come back whole, from an
// initial capacity of one byte as well as from the default.
TEST(Output, GrowsAroundEachDoubledCapacity)
{
  for (const size_t initial : {size_t{1}, mustache::kInitialCapacity}) {
    for (const size_t n : {4000, 4090, 4095, 4096, 4097, 8191, 8192, 8193, 20000}) {
      SCOPED_TRACE(testing::Message() << "initial " << initial << ", length " << n);
      Map m;
      m.insert_or_assign("a", Node{std::string(n, 'x')});
      m.insert_or_assign("b", Node{repeated("<&", 3000)});
      m.insert_or_assign("c", Node{std::string(n, 'y')});
      const std::string expected = std::string(n, 'x') + repeated("&lt;&amp;", 3000) + std::string(n, 'y');
      const Rendered r = rendered("{{{a}}}{{b}}{{{c}}}", m, initial, mustache::kMaxCapacity);
      EXPECT_EQ(r.fault, mustache::Fault::none);
      EXPECT_EQ(r.answer, expected);
    }
  }
}

// An escaped value grows up to six times. The string grows to the
// exact escaped size before the escape, and the escape writes whole
// 32-byte blocks, so the end of the escape near a doubled capacity is
// where a write past the end would show under ASan.
TEST(Output, EscapesNearTheEndOfTheString)
{
  for (size_t n = 4080; n <= 4100; n++) {
    SCOPED_TRACE(testing::Message() << "length " << n);
    Map m;
    m.insert_or_assign("a", Node{std::string(n, 'x')});
    m.insert_or_assign("b", Node{std::string(700, '"')});
    const std::string expected = std::string(n, 'x') + repeated("&quot;", 700);
    const Rendered r = rendered("{{{a}}}{{b}}", m, mustache::kInitialCapacity, mustache::kMaxCapacity);
    EXPECT_EQ(r.fault, mustache::Fault::none);
    EXPECT_EQ(r.answer, expected);
  }
}

// An initial capacity above max_capacity is refused before the first
// allocation. The string holds the capacity and the escape slack, and
// SIZE_MAX plus the slack wraps around to a small number.
TEST(Output, RefusesAnInitialCapacityOverTheLimit)
{
  Map m;
  m.insert_or_assign("a", Node{std::string(100, 'x')});
  EXPECT_EQ(rendered("{{{a}}}", m, SIZE_MAX, mustache::kMaxCapacity).fault, mustache::Fault::over_limit);
  EXPECT_EQ(rendered("{{{a}}}", m, 4097, 4096).fault, mustache::Fault::over_limit);

  const std::expected<mustache::Template, mustache::Error> t = mustache::compile("{{{a}}}");
  ASSERT_TRUE(t);
  const std::expected<std::string, mustache::Error> r = mustache::render(*t, m, {}, SIZE_MAX);
  ASSERT_FALSE(r);
  EXPECT_EQ(r.error().fault, mustache::Fault::over_limit);
}

// A render that needs more than max_capacity stops with over_limit
// and hands the host a string it can still read.
TEST(Output, StopsOverTheLimit)
{
  Map m;
  m.insert_or_assign("a", Node{std::string(5000, 'x')});
  EXPECT_EQ(rendered("{{{a}}}", m, mustache::kInitialCapacity, 4096).fault, mustache::Fault::over_limit);
}

}
