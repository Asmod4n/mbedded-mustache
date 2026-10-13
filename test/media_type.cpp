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

bool
refused(const std::string_view source, const std::string_view media_type)
{
  return !rendered(source, media_type, "v").compiled;
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

// SVG carries script and event handler attributes that the XML grammar
// does not guard, and a wildcard or a malformed type names no grammar.
TEST(MediaType, NamesNoGrammar)
{
  for (const std::string_view t : {"image/svg+xml", "text/plain", "*/*", "text/*", "text", "text/", "/html",
                                   "text/jscript", "application/x-javascript", "te xt/html"}) {
    SCOPED_TRACE(t);
    EXPECT_FALSE(mustache::grammar_of(t).has_value());
    EXPECT_TRUE(refused("{{x}}", t));
  }
}

// WHATWG HTML 13.2.5: text and a quoted attribute value take the five
// HTML escapes; a tag, an attribute name, an unquoted value and a
// comment take none that makes a value safe.
TEST(Html, EscapesTextAndQuotedValues)
{
  EXPECT_EQ(rendered("<p>{{x}}</p>", "text/html", "<a&'\">").answer, "<p>&lt;a&amp;&#39;&quot;&gt;</p>");
  EXPECT_EQ(rendered("<p title=\"{{x}}\">", "text/html", "\"x").answer, "<p title=\"&quot;x\">");
  EXPECT_EQ(rendered("<textarea>{{x}}</textarea>", "text/html", "</textarea>").answer,
            "<textarea>&lt;/textarea&gt;</textarea>");
}

TEST(Html, RefusesPlacesWithoutSafeEscaping)
{
  for (const std::string_view s : {"<{{x}}>", "<p {{x}}>", "<p {{x}}=1>", "<p title={{x}}>", "<!-- {{x}} -->",
                                   "<!DOCTYPE {{x}}>", "<p srcset=\"{{x}}\">", "<script type=\"text/template\">{{x}}</script>",
                                   "<xmp>{{x}}</xmp>"}) {
    SCOPED_TRACE(s);
    EXPECT_TRUE(refused(s, "text/html"));
  }
}

// A URL attribute decides its scheme with its first bytes. A value
// there may not choose a scheme other than http, https or mailto; a
// browser drops leading spaces and controls and every tab and newline
// before it reads the scheme, so the check does too.
TEST(Html, ChecksTheSchemeAtTheStartOfAUrl)
{
  EXPECT_EQ(rendered("<a href=\"{{x}}\">", "text/html", "https://e.test/?a&b").answer,
            "<a href=\"https://e.test/?a&amp;b\">");
  EXPECT_EQ(rendered("<a href=\"{{x}}\">", "text/html", "/path").answer, "<a href=\"/path\">");
  for (const std::string_view v : {"javascript:alert(1)", " JavaScript:alert(1)", "java\tscript:alert(1)",
                                   "data:text/html,x", "vbscript:x"}) {
    SCOPED_TRACE(v);
    EXPECT_EQ(rendered("<a href=\"{{x}}\">", "text/html", v).answer, "<a href=\"about:invalid\">");
  }
  EXPECT_EQ(rendered("<a href=\"/u/{{x}}\">", "text/html", "javascript:x").answer, "<a href=\"/u/javascript:x\">");
  EXPECT_TRUE(refused("<a href=\"{{x}}{{y}}\">", "text/html"));
  EXPECT_TRUE(refused("<a href=\"java{{x}}\">", "text/html"));
}

// A section repeats or drops its body, so the body must leave the
// grammar where it found it; otherwise the place of the next tag
// depends on the data.
TEST(Html, RefusesASectionThatChangesTheContext)
{
  EXPECT_TRUE(refused("{{#x}}<a href=\"{{/x}}{{y}}\">", "text/html"));
  EXPECT_FALSE(refused("<ul>{{#x}}<li>{{y}}</li>{{/x}}</ul>", "text/html"));
}

// ECMA-262 12.9.4: a string literal ends at its quote or a line
// terminator. A value inside one becomes \xHH for every byte that could
// end it or open markup; anywhere else in script a value is code.
TEST(JavaScript, EscapesInsideAStringOnly)
{
  EXPECT_EQ(rendered("var a = '{{x}}';", "text/javascript", "'</script>\\").answer,
            "var a = '\\x27\\x3c\\x2fscript\\x3e\\x5c';");
  EXPECT_EQ(rendered("<script>var a = \"{{x}}\";</script>", "text/html", "\"").answer,
            "<script>var a = \"\\x22\";</script>");
  EXPECT_EQ(rendered("<b onclick=\"f('{{x}}')\">", "text/html", "');x('").answer,
            "<b onclick=\"f('\\x27\\x29\\x3bx\\x28\\x27')\">");
  EXPECT_EQ(rendered("var a = '{{x}}';", "text/javascript", "\xe2\x80\xa8").answer, "var a = '\\u2028';");
}

// A '/' in code is a regular expression or a division depending on the
// token before it, and a wrong guess moves every later string. After the
// first such '/', and after the first ${ of a template literal, every
// later value in that script is refused rather than guessed.
TEST(JavaScript, RefusesWhatItCannotPlace)
{
  for (const std::string_view s : {"var a = {{x}};", "// {{x}}", "/* {{x}} */", "var r = /a/; var s = '{{x}}';",
                                   "var t = `${a}{{x}}`;"}) {
    SCOPED_TRACE(s);
    EXPECT_TRUE(refused(s, "text/javascript"));
  }
  EXPECT_TRUE(refused("<script><!-- var a = '{{x}}';</script>", "text/html"));
  EXPECT_TRUE(refused("<b onclick=\"f(&quot;{{x}}&quot;)\">", "text/html"));
  EXPECT_FALSE(refused("var t = `{{x}}`;", "text/javascript"));
}

// CSS Syntax 4.3.5: a string ends at its quote or a newline. A value
// inside one is written as hex escapes; a string after url(, @import or
// image-set( names a resource and takes no value.
TEST(Css, EscapesInsideAStringOnly)
{
  EXPECT_EQ(rendered("a::after { content: '{{x}}'; }", "text/css", "'}").answer,
            "a::after { content: '\\27 \\7d '; }");
  EXPECT_EQ(rendered("<p style=\"font-family: '{{x}}'\">", "text/html", "x\"").answer,
            "<p style=\"font-family: 'x\\22 '\">");
  for (const std::string_view s : {"a { color: {{x}} }", "a { background: url('{{x}}') }", "@import '{{x}}';",
                                   "/* {{x}} */"}) {
    SCOPED_TRACE(s);
    EXPECT_TRUE(refused(s, "text/css"));
  }
}

// RFC 8259 7: inside a string, quotation mark, reverse solidus and the
// control characters U+0000 to U+001F must be escaped. Outside a string
// a value would be JSON syntax; a raw tag writes it as the author wants.
TEST(Json, EscapesInsideAStringOnly)
{
  EXPECT_EQ(rendered("{\"a\":\"{{x}}\"}", "application/json", "\"\\\n\x01").answer,
            "{\"a\":\"\\\"\\\\\\n\\u0001\"}");
  EXPECT_TRUE(refused("{\"a\":{{x}}}", "application/json"));
  EXPECT_EQ(rendered("{\"a\":{{{x}}}}", "application/json", "42").answer, "{\"a\":42}");
}

// XML 1.0 2.4 and 4.6: text and attribute values take the five
// predefined entities; a comment, CDATA, a processing instruction and
// a document type declaration take none.
TEST(Xml, EscapesTextAndAttributeValues)
{
  EXPECT_EQ(rendered("<a b=\"{{x}}\">{{y}}</a>", "application/xml", "<'&\">").answer,
            "<a b=\"&lt;&apos;&amp;&quot;&gt;\">&lt;&apos;&amp;&quot;&gt;</a>");
  for (const std::string_view s : {"<!-- {{x}} -->", "<![CDATA[{{x}}]]>", "<?pi {{x}}?>", "<!DOCTYPE a [{{x}}]>",
                                   "<a {{x}}=\"1\"/>"}) {
    SCOPED_TRACE(s);
    EXPECT_TRUE(refused(s, "application/xml"));
  }
}

}
