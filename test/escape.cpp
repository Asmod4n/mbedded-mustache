#include <mustache/escape.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <string>
#include <string_view>
#include <vector>

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
  for (const std::string &s : every_case()) {
    SCOPED_TRACE(s);
    std::array<char, 512> out{};
    const size_t count = mustache::escape_string_into(out, s);
    EXPECT_EQ(std::string_view(out.data(), count), escaped_by_rule(s));
  }
}

// Every SIMD form of copied_string is held to the scalar form. A value
// longer than kCopiedStringMax is not copied: the count is 0 and no byte
// of the room changes. The mark is held to the rule of the spec.
TEST(CopiedString, MatchesTheScalarForm)
{
  for (const std::string &s : every_case()) {
    SCOPED_TRACE(s);
    std::array<char, 64> wide{};
    std::array<char, 64> scalar{};
    const mustache::Copied w = mustache::copied_string(wide, s);
    const mustache::Copied c = mustache::copied_string_scalar(scalar, s);
    const bool copied = s.size() <= mustache::kCopiedStringMax;
    EXPECT_EQ(c.count, copied ? s.size() : 0);
    EXPECT_EQ(c.marks != 0, copied && escaped_by_rule(s) != s);
    EXPECT_EQ(w.count, c.count);
    EXPECT_EQ(w.marks != 0, c.marks != 0);
    EXPECT_EQ(std::string_view(wide.data(), c.count), std::string_view(scalar.data(), c.count));
    if (!copied) {
      EXPECT_TRUE(std::ranges::all_of(wide, [](const char b) { return b == 0; }));
      EXPECT_TRUE(std::ranges::all_of(scalar, [](const char b) { return b == 0; }));
    }
  }
}

}
