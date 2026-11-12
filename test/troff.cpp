#include <mustache/media_type.hpp>
#include <mustache/mustache.hpp>

#include "node.hpp"

#include <gtest/gtest.h>

#include <array>
#include <string>
#include <string_view>

namespace {

// Every case renders a template compiled at run time for text/troff.
std::expected<std::string, mustache::Error>
rendered(const std::string_view source, const std::string_view x, const std::string_view y = {})
{
  const std::expected<mustache::Template, mustache::Error> t = mustache::compile(source, "text/troff");
  if (!t) return std::unexpected(t.error());
  const NodeMap data{{"x", Node{std::string(x)}}, {"y", Node{std::string(y)}}};
  return mustache::render(*t, data);
}

std::string
escaped(const std::string_view value)
{
  return rendered("{{x}}", value).value_or("<fault>");
}

// roff.7 "Text Lines" and "Whitespace", man.7 and mandoc_char.7: the
// escapes of one character each. mandoc_char.7 says a hyphen needs no
// escape.
TEST(Troff, EscapesTheCharactersThatMeanSomething)
{
  EXPECT_EQ(escaped("a\\b"), "a\\eb");
  EXPECT_EQ(escaped("say \"hi\""), "say \\(dqhi\\(dq");
  EXPECT_EQ(escaped("it's"), "it\\(aqs");
  EXPECT_EQ(escaped("`x`"), "\\(gax\\(ga");
  EXPECT_EQ(escaped("a~b"), "a\\(tib");
  EXPECT_EQ(escaped("a^b"), "a\\(hab");
  EXPECT_EQ(escaped("a-b"), "a-b");
  EXPECT_EQ(escaped("a.b"), "a.b");
}

// roff.7 "Text Lines": a line that begins with a period is a request or
// macro line. A value starts a line, and so does each line of a value.
TEST(Troff, ProtectsTheStartOfALine)
{
  EXPECT_EQ(escaped(".SH x"), "\\&.SH x");
  EXPECT_EQ(escaped("x\n.SH y"), "x\n\\&.SH y");
  EXPECT_EQ(rendered(".SH {{x}}\n", "x\n.SH y").value_or("<fault>"), ".SH x\n\\&.SH y\n");
}

// roff.7 "Whitespace": a text line that begins with a space is printed
// with a leading newline; a space at the end of a line is forced by a
// backslash, a space and \&.
TEST(Troff, ProtectsASpaceAtTheEdgeOfALine)
{
  EXPECT_EQ(escaped(" x"), "\\& x");
  EXPECT_EQ(escaped("x "), "x\\ \\&");
  EXPECT_EQ(escaped(" "), "\\&\\ \\&");
  EXPECT_EQ(escaped("a b"), "a b");
  EXPECT_EQ(escaped("x \ny"), "x\\ \\&\ny");
}

// roff.7 "Sentence Spacing": a period, an exclamation mark or a question
// mark, followed by zero or more of ) ] ' and ", ends a sentence at the
// end of a line, and \& at the end of the line says it does not.
TEST(Troff, ProtectsAMarkAtTheEndOfALine)
{
  EXPECT_EQ(escaped("a."), "a.\\&");
  EXPECT_EQ(escaped("a!"), "a!\\&");
  EXPECT_EQ(escaped("a?"), "a?\\&");
  EXPECT_EQ(escaped("a.)"), "a.)\\&");
  EXPECT_EQ(escaped("a.]"), "a.]\\&");
  EXPECT_EQ(escaped("a.'"), "a.\\(aq\\&");
  EXPECT_EQ(escaped("a.\""), "a.\\(dq\\&");
  EXPECT_EQ(escaped("a.\nb"), "a.\\&\nb");
  EXPECT_EQ(escaped("a. b"), "a. b");
  EXPECT_EQ(escaped("."), "\\&.\\&");
}

// A value is not text of a man page when it holds a control character;
// bytes from 0x80 on pass, because the caller decides what UTF-8 is.
TEST(Troff, StopsAtAControlCharacter)
{
  const std::expected<std::string, mustache::Error> r = rendered("a{{x}}b{{y}}", "v", "\t");
  ASSERT_FALSE(r);
  EXPECT_EQ(r.error().fault, mustache::Fault::control_character);
  EXPECT_EQ(std::string(r.error().what), "a value has a control character");
}

// Each byte stands at the start, in the middle and at the end of a value.
// The expectation is written from the rules of the pages, one table of
// the named escapes and the rules for a period, a space and the marks.
std::string
named(const char c)
{
  switch (c) {
    case '\\': return "\\e";
    case '"':  return "\\(dq";
    case '\'': return "\\(aq";
    case '`':  return "\\(ga";
    case '~':  return "\\(ti";
    case '^':  return "\\(ha";
    default:   return std::string(1, c);
  }
}

TEST(Troff, EveryByteAtEachPosition)
{
  for (int b = 0; b < 256; b++) {
    const char c = (char)b;
    SCOPED_TRACE(b);
    const std::string at_start = std::string(1, c) + "x";
    const std::string in_middle = "x" + std::string(1, c) + "x";
    const std::string at_end = "x" + std::string(1, c);
    if ((b < 0x20 && b != '\n') || b == 0x7f) {
      for (const std::string &v : {at_start, in_middle, at_end}) {
        const std::expected<std::string, mustache::Error> r = rendered("{{x}}", v);
        ASSERT_FALSE(r);
        EXPECT_EQ(r.error().fault, mustache::Fault::control_character);
      }
      continue;
    }
    if (b >= 0x80) {
      EXPECT_EQ(escaped(at_start), at_start);
      EXPECT_EQ(escaped(in_middle), in_middle);
      EXPECT_EQ(escaped(at_end), at_end);
      continue;
    }
    const std::string start_want = c == '.' ? "\\&.x" : c == ' ' ? "\\& x" : named(c) + "x";
    const std::string end_want = c == ' ' ? "x\\ \\&" : (c == '.' || c == '!' || c == '?') ? std::string("x") + c + "\\&" : "x" + named(c);
    EXPECT_EQ(escaped(at_start), start_want);
    EXPECT_EQ(escaped(in_middle), "x" + named(c) + "x");
    EXPECT_EQ(escaped(at_end), end_want);
  }
}

// The output of one input byte is at most six bytes, the bound that the
// renderer reserves for a value of this type, and it holds for every
// value of up to four bytes from the characters that grow the most.
TEST(Troff, OutputStaysWithinTheBound)
{
  constexpr std::string_view kAlphabet = ". !\"'a\\\n)";
  constexpr size_t kGrowthMax = 6;
  std::string value;
  const auto check = [&](const auto &self, const size_t length) -> void {
    if (value.size() == length) {
      const std::string out = escaped(value);
      ASSERT_NE(out, "<fault>");
      EXPECT_LE(out.size(), kGrowthMax * value.size()) << value;
      return;
    }
    for (const char c : kAlphabet) {
      value.push_back(c);
      self(self, length);
      value.pop_back();
    }
  };
  for (size_t length = 1; length <= 4; length++) check(check, length);
}

}
