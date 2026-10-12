#include <mustache/escape.hpp>

#include <gtest/gtest.h>

#include <array>
#include <string>
#include <string_view>

namespace {

// Mustache spec, interpolation: {{name}} escapes HTML. These five
// replacements are the rule, written here from the spec and not taken
// from the table under test.
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

// copied_string reads 16 bytes from a std::string shorter than 16. That
// is inside the string's own buffer only while its capacity holds them,
// so each case runs with the short buffer, with a large heap buffer and
// with a buffer that was shrunk after it held a long value.
std::array<std::string, 3>
held_three_ways(const std::string &value)
{
  std::string heap_held;
  heap_held.reserve(100);
  heap_held = value;
  std::string shrunk(40, 'y');
  shrunk = value;
  shrunk.shrink_to_fit();
  return {value, heap_held, shrunk};
}

std::vector<std::string>
every_case()
{
  constexpr std::string_view kMarks = "&<>\"'";
  std::vector<std::string> all;
  for (size_t n = 0; n <= 40; n++) {
    all.push_back(std::string(n, 'x'));
    for (size_t at = 0; at < n; at++) {
      for (const char m : kMarks) {
        std::string s(n, 'x');
        s.at(at) = m;
        all.push_back(s);
      }
    }
  }
  return all;
}

TEST(EscapeString, MatchesTheRule)
{
  for (const std::string &value : every_case()) {
    for (const std::string &s : held_three_ways(value)) {
      SCOPED_TRACE(s);
      std::array<char, 512> out{};
      char *const end = mustache::escape_string_into(out.data(), s);
      EXPECT_EQ(std::string_view(out.data(), (size_t)(end - out.data())), escaped_by_rule(s));
    }
  }
}

// Every SIMD form of copied_string is held to the scalar form.
TEST(CopiedString, MatchesTheScalarForm)
{
  for (const std::string &value : every_case()) {
    for (const std::string &s : held_three_ways(value)) {
      SCOPED_TRACE(s);
      std::array<char, 64> wide{};
      std::array<char, 64> scalar{};
      const mustache::Copied w = mustache::copied_string(wide.data(), s);
      const mustache::Copied c = mustache::copied_string_scalar(scalar.data(), s);
      EXPECT_EQ(c.w != nullptr, s.size() <= 32);
      if (w.w == nullptr || c.w == nullptr) continue;
      EXPECT_EQ(w.marks != 0, c.marks != 0);
      EXPECT_EQ(w.w - wide.data(), c.w - scalar.data());
      EXPECT_EQ(std::string_view(wide.data(), s.size()), std::string_view(scalar.data(), s.size()));
    }
  }
}

}
