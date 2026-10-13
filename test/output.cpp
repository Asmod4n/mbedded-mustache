#include <mustache/mustache.hpp>

#include "c_host.hpp"

#include <gtest/gtest.h>

#include <string>
#include <string_view>

namespace {

namespace containers = mustache::detail::containers;
using Map = NodeMap;

struct Rendered {
  mustache::Fault fault;
  std::string     answer;
};

Rendered
rendered(const std::string_view source, const Map &data, const size_t initial_capacity, const size_t max_capacity)
{
  containers::Host h;
  const std::optional<containers::Program> p = mustache::program_of<containers::Key>(h, source, mustache::kSourceMax);
  if (!p) return {mustache::Fault::parse, {}};
  const mustache::Fault f = mustache::Walk<containers::Host, containers::Value, containers::Key>(
                                h, mustache::kMaxRenderScore, initial_capacity, max_capacity)
                                .run(*p, containers::value_of(data));
  return {f, h.answer};
}

// The same render through the C API. A template asks for the size of
// its last output as its first capacity, so each render here uses a
// fresh template and starts from the default, as the Walk does.
Rendered
rendered_through_c(const std::string_view source, const Map &data)
{
  mustache_template *tpl = nullptr;
  if (mustache_compile(source.data(), source.size(), &tpl) != 0) return {mustache::Fault::parse, {}};
  c_set_callbacks(tpl);
  const Node root{data};
  CUser user;
  void *string = nullptr;
  const int r = mustache_render(tpl, &root, c_release, &user, &string);
  mustache_dispose_template(tpl);
  return {r == 0 ? mustache::Fault::none : mustache::Fault::no_memory, user.answer};
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
      const Rendered c = rendered_through_c("{{{a}}}{{b}}{{{c}}}", m);
      EXPECT_EQ(c.fault, mustache::Fault::none);
      EXPECT_EQ(c.answer, expected);
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
    const Rendered c = rendered_through_c("{{{a}}}{{b}}", m);
    EXPECT_EQ(c.fault, mustache::Fault::none);
    EXPECT_EQ(c.answer, expected);
  }
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
