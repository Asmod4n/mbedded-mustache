#include <mustache/media_type.hpp>
#include <mustache/std.hpp>

#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <string_view>

namespace {

using mustache::std_host::Host;
using mustache::std_host::Key;
using mustache::std_host::Map;
using mustache::std_host::Value;

struct Rendered {
  bool        compiled;
  std::string answer;
};

// Every case renders through the Walk, the runtime path that a dynamic
// language uses. test/reflect.cpp runs the same cases through the
// reflection render.
Rendered
rendered(const std::string_view source, const std::string_view media_type, const std::string_view value)
{
  Host h;
  const std::optional<mustache::Program<Key>> p = mustache::program_of<Key>(h, source, mustache::kSourceMax, media_type);
  if (!p) return {false, {}};
  Map m;
  m.insert_or_assign("x", Value{std::string(value)});
  m.insert_or_assign("y", Value{std::string(value)});
  const Value root{m};
  if (mustache::Walk<Host, const Value *, Key>(h).run(*p, &root) != mustache::Fault::none) return {false, {}};
  return {true, h.answer};
}

// RFC 6838 4.2 names a media type type "/" subtype, case-insensitive;
// RFC 9110 8.3.1 puts parameters after ";" with optional whitespace.
// RFC 6839 adds structured suffixes, and +json and +xml carry the
// grammar of JSON and XML. RFC 9239 names text/javascript and keeps the
// older names as aliases.
TEST(MediaType, NamesAGrammar)
{
  using mustache::Grammar;
  EXPECT_EQ(mustache::grammar_of("text/html"), Grammar::html);
  EXPECT_EQ(mustache::grammar_of("TEXT/HTML ; charset=utf-8"), Grammar::html);
  EXPECT_EQ(mustache::grammar_of("application/xhtml+xml"), Grammar::html);
  EXPECT_EQ(mustache::grammar_of("application/xml"), Grammar::xml);
  EXPECT_EQ(mustache::grammar_of("application/atom+xml"), Grammar::xml);
  EXPECT_EQ(mustache::grammar_of("application/json"), Grammar::json);
  EXPECT_EQ(mustache::grammar_of("application/ld+json"), Grammar::json);
  EXPECT_EQ(mustache::grammar_of("text/javascript"), Grammar::js);
  EXPECT_EQ(mustache::grammar_of("application/ecmascript"), Grammar::js);
  EXPECT_EQ(mustache::grammar_of("text/css"), Grammar::css);
}

// A type with no escaper of its own names no grammar and compiles
// nothing, so a template is never sent with the escaping of another
// format. SVG is XML but carries script, which XML escaping does not
// guard; a wildcard or a malformed type names nothing.
TEST(MediaType, NamesNoGrammar)
{
  for (const std::string_view t : {"image/svg+xml", "text/plain", "*/*", "text/*", "text", "text/", "/html",
                                   "text/jscript", "application/x-javascript", "te xt/html"}) {
    SCOPED_TRACE(t);
    EXPECT_FALSE(mustache::grammar_of(t).has_value());
    EXPECT_FALSE(rendered("{{x}}", t, "v").compiled);
  }
}

// The media type picks one escaper for every {{tag}} of the template.
// Where a tag stands in that format is the author's choice, as in
// Mustache; the escaper makes a value safe inside the place the format
// gives a value: text in HTML and XML, a string in JSON, JavaScript and
// CSS. A {{{raw}}} tag is never escaped.
TEST(Escaper, Html)
{
  EXPECT_EQ(rendered("<p title=\"{{x}}\">{{y}}</p>", "text/html", "<a&'\">").answer,
            "<p title=\"&lt;a&amp;&#39;&quot;&gt;\">&lt;a&amp;&#39;&quot;&gt;</p>");
  EXPECT_EQ(rendered("{{{x}}}", "text/html", "<b>").answer, "<b>");
}

// XML 1.0 4.6 predefines five entities; &apos; is one of them.
TEST(Escaper, Xml)
{
  EXPECT_EQ(rendered("<a b=\"{{x}}\">{{y}}</a>", "application/xml", "<'&\">").answer,
            "<a b=\"&lt;&apos;&amp;&quot;&gt;\">&lt;&apos;&amp;&quot;&gt;</a>");
}

// RFC 8259 7: inside a string, quotation mark, reverse solidus and the
// control characters U+0000 to U+001F must be escaped.
TEST(Escaper, Json)
{
  EXPECT_EQ(rendered("{\"a\":\"{{x}}\"}", "application/json", "\"\\\n\x01").answer,
            "{\"a\":\"\\\"\\\\\\n\\u0001\"}");
  EXPECT_EQ(rendered("{\"a\":{{{x}}}}", "application/ld+json", "42").answer, "{\"a\":42}");
}

// ECMA-262 12.9.4: a string literal ends at its quote or a line
// terminator, and a script element ends at </script. Every byte that
// could end either becomes \xHH; U+2028 and U+2029 become \u escapes.
TEST(Escaper, JavaScript)
{
  EXPECT_EQ(rendered("var a = '{{x}}';", "text/javascript", "'</script>\\`${").answer,
            "var a = '\\x27\\x3c\\x2fscript\\x3e\\x5c\\x60\\x24\\x7b';");
  EXPECT_EQ(rendered("var a = '{{x}}';", "text/javascript", "\xe2\x80\xa8").answer, "var a = '\\u2028';");
}

// CSS Syntax 4.3.7: a backslash, one to six hex digits and one space
// stand for one code point.
TEST(Escaper, Css)
{
  EXPECT_EQ(rendered("a::after { content: '{{x}}'; }", "text/css", "'}").answer,
            "a::after { content: '\\27 \\7d '; }");
}

}
