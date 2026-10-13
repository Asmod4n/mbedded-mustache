#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "compile.hpp"

namespace mustache {

enum class Grammar : uint8_t { html, xml, json, js, css };

namespace detail {

constexpr bool
is_ows(const char c)
{
  return c == ' ' || c == '\t';
}

constexpr std::string_view
ows_trimmed(std::string_view s)
{
  while (!s.empty() && is_ows(s.front())) s.remove_prefix(1);
  while (!s.empty() && is_ows(s.back())) s.remove_suffix(1);
  return s;
}

constexpr bool
is_alpha_or_digit(const char c)
{
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

constexpr bool
is_restricted_name_char(const char c)
{
  return is_alpha_or_digit(c) || c == '!' || c == '#' || c == '$' || c == '&' || c == '-' || c == '^' || c == '_' ||
         c == '.' || c == '+';
}

constexpr bool
is_restricted_name(const std::string_view s)
{
  return !s.empty() && s.size() <= 127 && is_alpha_or_digit(s.front()) &&
         std::ranges::all_of(s, is_restricted_name_char);
}

constexpr char
lower(const char c)
{
  return c >= 'A' && c <= 'Z' ? (char)(c - 'A' + 'a') : c;
}

constexpr std::string
lowered(const std::string_view s)
{
  std::string r;
  for (const char c : s) r += lower(c);
  return r;
}

}

namespace detail {

constexpr bool
equals_ignoring_case(const std::string_view a, const std::string_view lower_b)
{
  return a.size() == lower_b.size() && std::ranges::equal(a, lower_b, {}, lower);
}

constexpr bool
ends_with_ignoring_case(const std::string_view a, const std::string_view lower_b)
{
  return a.size() >= lower_b.size() && equals_ignoring_case(a.substr(a.size() - lower_b.size()), lower_b);
}

}

constexpr std::optional<Grammar>
grammar_of(const std::string_view media_type)
{
  using namespace detail;
  const std::string_view essence = ows_trimmed(media_type.substr(0, media_type.find(';')));
  const size_t slash = essence.find('/');
  if (slash == std::string_view::npos) [[unlikely]] return std::nullopt;
  const std::string_view t = essence.substr(0, slash);
  const std::string_view st = essence.substr(slash + 1);
  if (!is_restricted_name(t) || !is_restricted_name(st)) [[unlikely]] return std::nullopt;
  const bool text = equals_ignoring_case(t, "text");
  const bool application = equals_ignoring_case(t, "application");
  if (text && equals_ignoring_case(st, "html")) return Grammar::html;
  if (application && equals_ignoring_case(st, "xhtml+xml")) return Grammar::html;
  if (application && equals_ignoring_case(st, "json")) return Grammar::json;
  if (ends_with_ignoring_case(st, "+json")) return Grammar::json;
  if ((application || text) && equals_ignoring_case(st, "xml")) return Grammar::xml;
  if (text && equals_ignoring_case(st, "css")) return Grammar::css;
  if ((text || application) && (equals_ignoring_case(st, "javascript") || equals_ignoring_case(st, "ecmascript")))
    return Grammar::js;
  if (equals_ignoring_case(t, "image") && equals_ignoring_case(st, "svg+xml")) return std::nullopt;
  if (ends_with_ignoring_case(st, "+xml")) return Grammar::xml;
  return std::nullopt;
}

namespace detail {

enum class HtmlState : uint8_t {
  text,
  tag_open,
  markup_declaration,
  tag_name,
  end_tag,
  before_attribute_name,
  attribute_name,
  after_attribute_name,
  before_value,
  quoted_value,
  unquoted_value,
  comment,
  bogus_comment,
  raw_text,
  rcdata,
};

enum class AttributeKind : uint8_t { plain, url, js, css, unsafe };

enum class UrlState : uint8_t { empty, static_prefix, after_value, past_scheme };

enum class XmlState : uint8_t { text, tag, quoted_value, comment, cdata, processing_instruction, doctype };

enum class JsonState : uint8_t { outside, string, string_escape };

enum class JsState : uint8_t { code, string, string_escape, template_text, line_comment, block_comment, ambiguous };

enum class CssState : uint8_t { code, string, string_escape, comment };

enum class Embedded : uint8_t { none, js, css };

struct Place {
  HtmlState     html = HtmlState::text;
  XmlState      xml = XmlState::text;
  JsonState     json = JsonState::outside;
  char          quote = 0;
  AttributeKind attribute = AttributeKind::plain;
  UrlState      url = UrlState::empty;
  std::string   name;
  std::string   element;
  std::string   tail;
  std::string   raw_tail;
  int           doctype_depth = 0;
  Embedded      embedded = Embedded::none;
  JsState       js = JsState::code;
  char          js_quote = 0;
  char          js_last = 0;
  CssState      css = CssState::code;
  char          css_quote = 0;
  bool          css_string_is_url = false;
  std::string   css_word;
  std::string   script_type;
  bool          script_type_seen = false;
  bool          embedded_refused = false;

  constexpr bool operator==(const Place &) const = default;
};

inline constexpr std::array<std::string_view, 10> kUrlAttributes = {
    "href", "src", "action", "formaction", "cite", "poster", "background", "data", "codebase", "xlink:href"};

inline constexpr std::array<std::string_view, 8> kRawTextElements = {"script",   "style",    "xmp",      "iframe",
                                                                     "noembed",  "noframes", "noscript", "plaintext"};

inline constexpr std::array<std::string_view, 2> kRcdataElements = {"textarea", "title"};

constexpr bool
is_html_space(const char c)
{
  return c == ' ' || c == '\t' || c == '\n' || c == '\f' || c == '\r';
}

constexpr AttributeKind
attribute_kind_of(const std::string_view name)
{
  if (name.starts_with("on")) return AttributeKind::js;
  if (name == "style") return AttributeKind::css;
  if (name == "srcset") return AttributeKind::unsafe;
  if (std::ranges::find(kUrlAttributes, name) != kUrlAttributes.end()) return AttributeKind::url;
  return AttributeKind::plain;
}


constexpr Place
js_after(Place p, const char c)
{
  switch (p.js) {
    case JsState::code:
      if (p.tail == "/") {
        p.tail.clear();
        if (c == '/') {
          p.js = JsState::line_comment;
          return p;
        }
        if (c == '*') {
          p.js = JsState::block_comment;
          return p;
        }
        p.js = JsState::ambiguous;
        return p;
      }
      if (c == '/') p.tail = "/";
      else if (c == '"' || c == '\'') {
        p.js = JsState::string;
        p.js_quote = c;
      }
      else if (c == '`') {
        p.js = JsState::template_text;
        p.js_last = 0;
      }
      return p;
    case JsState::string:
      if (c == '\\') p.js = JsState::string_escape;
      else if (c == p.js_quote) {
        p.js = JsState::code;
        p.js_quote = 0;
      }
      else if (c == '\n' || c == '\r') p.js = JsState::ambiguous;
      return p;
    case JsState::string_escape:
      p.js = JsState::string;
      return p;
    case JsState::template_text:
      if (p.js_last == '\\') p.js_last = 0;
      else if (c == '\\') p.js_last = '\\';
      else if (c == '`') p.js = JsState::code;
      else if (c == '{' && p.js_last == '$') p.js = JsState::ambiguous;
      else p.js_last = c;
      return p;
    case JsState::line_comment:
      if (c == '\n' || c == '\r') p.js = JsState::code;
      return p;
    case JsState::block_comment:
      if (p.js_last == '*' && c == '/') {
        p.js = JsState::code;
        p.js_last = 0;
      }
      else p.js_last = c;
      return p;
    case JsState::ambiguous:
      return p;
  }
  return p;
}

constexpr Place
css_after(Place p, const char c)
{
  switch (p.css) {
    case CssState::code:
      if (p.tail == "/") {
        p.tail.clear();
        if (c == '*') {
          p.css = CssState::comment;
          p.css_word.clear();
          return p;
        }
      }
      if (c == '/') p.tail = "/";
      else if (c == '"' || c == '\'') {
        p.css = CssState::string;
        p.css_quote = c;
        p.css_string_is_url = p.css_word == "url(" || p.css_word == "@import" || p.css_word == "image-set(";
        p.css_word.clear();
      }
      else if (is_html_space(c)) {
        if (p.css_word != "@import") p.css_word.clear();
      }
      else {
        if (p.css_word == "@import") p.css_word.clear();
        p.css_word += lower(c);
        if (p.css_word.size() > 16) p.css_word.erase(0, p.css_word.size() - 16);
        const std::string_view w = p.css_word;
        if (w.ends_with("url(")) p.css_word = "url(";
        else if (w.ends_with("image-set(")) p.css_word = "image-set(";
        else if (w.ends_with("@import")) p.css_word = "@import";
      }
      return p;
    case CssState::string:
      if (c == '\\') p.css = CssState::string_escape;
      else if (c == p.css_quote) {
        p.css = CssState::code;
        p.css_quote = 0;
        p.css_string_is_url = false;
      }
      return p;
    case CssState::string_escape:
      p.css = CssState::string;
      return p;
    case CssState::comment:
      if (p.tail == "*" && c == '/') {
        p.css = CssState::code;
        p.tail.clear();
      }
      else p.tail = c == '*' ? "*" : "";
      return p;
  }
  return p;
}

constexpr Place
embedded_after(Place p, const char c)
{
  if (p.embedded == Embedded::js) return js_after(std::move(p), c);
  if (p.embedded == Embedded::css) return css_after(std::move(p), c);
  return p;
}

constexpr bool
is_js_type(const std::string_view type)
{
  const std::string t = lowered(ows_trimmed(type));
  return t.empty() || t == "module" || t == "text/javascript" || t == "application/javascript" ||
         t == "application/ecmascript" || t == "text/ecmascript";
}

constexpr Place
embedded_reset(Place p, const Embedded e)
{
  p.embedded = e;
  p.js = JsState::code;
  p.js_quote = 0;
  p.js_last = 0;
  p.css = CssState::code;
  p.css_quote = 0;
  p.css_string_is_url = false;
  p.css_word.clear();
  p.embedded_refused = false;
  return p;
}

constexpr Place
html_tag_closed(Place p)
{
  Embedded e = Embedded::none;
  if (p.element == "script" && is_js_type(p.script_type_seen ? std::string_view(p.script_type) : "")) e = Embedded::js;
  else if (p.element == "style") e = Embedded::css;
  if (std::ranges::find(kRawTextElements, p.element) != kRawTextElements.end()) p.html = HtmlState::raw_text;
  else if (std::ranges::find(kRcdataElements, p.element) != kRcdataElements.end()) p.html = HtmlState::rcdata;
  else p.html = HtmlState::text;
  p = embedded_reset(std::move(p), p.html == HtmlState::raw_text ? e : Embedded::none);
  if (p.html == HtmlState::text) p.element.clear();
  p.name.clear();
  p.script_type.clear();
  p.script_type_seen = false;
  p.tail.clear();
  return p;
}

constexpr bool
ends_raw_element(const std::string_view tail, const std::string_view element)
{
  const std::string close = "</" + std::string(element);
  return tail.size() >= close.size() + 1 && lowered(tail.substr(tail.size() - close.size() - 1, close.size())) == close &&
         (is_html_space(tail.back()) || tail.back() == '/' || tail.back() == '>');
}

constexpr Place
html_after(Place p, const char c)
{
  switch (p.html) {
    case HtmlState::text:
      if (c == '<') p.html = HtmlState::tag_open;
      return p;
    case HtmlState::tag_open:
      if (c == '!') {
        p.html = HtmlState::markup_declaration;
        p.tail.clear();
      }
      else if (c == '/') p.html = HtmlState::end_tag;
      else if (c == '?') p.html = HtmlState::bogus_comment;
      else if (is_alpha_or_digit(c) && !(c >= '0' && c <= '9')) {
        p.html = HtmlState::tag_name;
        p.element = std::string(1, lower(c));
        p.script_type.clear();
        p.script_type_seen = false;
      }
      else p.html = HtmlState::text;
      return p;
    case HtmlState::markup_declaration:
      p.tail += c;
      if (p.tail == "--") {
        p.html = HtmlState::comment;
        p.tail.clear();
      }
      else if (p.tail != "-") p.html = c == '>' ? HtmlState::text : HtmlState::bogus_comment;
      return p;
    case HtmlState::tag_name:
      if (is_html_space(c) || c == '/') p.html = HtmlState::before_attribute_name;
      else if (c == '>') return html_tag_closed(p);
      else p.element += lower(c);
      return p;
    case HtmlState::end_tag:
      if (c == '>') {
        p.html = HtmlState::text;
        p.element.clear();
      }
      return p;
    case HtmlState::before_attribute_name:
      if (c == '>') return html_tag_closed(p);
      if (!is_html_space(c) && c != '/') {
        p.html = HtmlState::attribute_name;
        p.name = std::string(1, lower(c));
      }
      return p;
    case HtmlState::attribute_name:
      if (is_html_space(c)) p.html = HtmlState::after_attribute_name;
      else if (c == '=') p.html = HtmlState::before_value;
      else if (c == '/') p.html = HtmlState::before_attribute_name;
      else if (c == '>') return html_tag_closed(p);
      else p.name += lower(c);
      return p;
    case HtmlState::after_attribute_name:
      if (c == '=') p.html = HtmlState::before_value;
      else if (c == '/') p.html = HtmlState::before_attribute_name;
      else if (c == '>') return html_tag_closed(p);
      else if (!is_html_space(c)) {
        p.html = HtmlState::attribute_name;
        p.name = std::string(1, lower(c));
      }
      return p;
    case HtmlState::before_value:
      if (c == '"' || c == '\'') {
        p.html = HtmlState::quoted_value;
        p.quote = c;
        p.attribute = attribute_kind_of(p.name);
        p.url = UrlState::empty;
        if (p.element == "script" && p.name == "type") p.script_type_seen = true;
        p = embedded_reset(std::move(p), p.attribute == AttributeKind::js    ? Embedded::js
                                         : p.attribute == AttributeKind::css ? Embedded::css
                                                                             : Embedded::none);
      }
      else if (c == '>') return html_tag_closed(p);
      else if (!is_html_space(c)) {
        p.html = HtmlState::unquoted_value;
        if (p.element == "script" && p.name == "type") {
          p.script_type_seen = true;
          p.script_type = std::string(1, c);
        }
      }
      return p;
    case HtmlState::quoted_value:
      if (c == p.quote) {
        p.html = HtmlState::before_attribute_name;
        p.quote = 0;
        p.attribute = AttributeKind::plain;
        p.url = UrlState::empty;
        p = embedded_reset(std::move(p), Embedded::none);
      }
      else if (p.embedded != Embedded::none) {
        if (c == '&') p.embedded_refused = true;
        else p = embedded_after(std::move(p), c);
      }
      else if (p.element == "script" && p.name == "type") p.script_type += c;
      else if (p.attribute == AttributeKind::url && p.url != UrlState::past_scheme) {
        if (c == ':' || c == '/' || c == '?' || c == '#') p.url = UrlState::past_scheme;
        else if (p.url == UrlState::empty) p.url = UrlState::static_prefix;
      }
      return p;
    case HtmlState::unquoted_value:
      if (is_html_space(c)) p.html = HtmlState::before_attribute_name;
      else if (c == '>') return html_tag_closed(p);
      else if (p.element == "script" && p.name == "type") p.script_type += c;
      return p;
    case HtmlState::comment:
      p.tail += c;
      if (p.tail.size() > 3) p.tail.erase(0, p.tail.size() - 3);
      if (p.tail.ends_with("-->")) {
        p.html = HtmlState::text;
        p.tail.clear();
      }
      return p;
    case HtmlState::bogus_comment:
      if (c == '>') p.html = HtmlState::text;
      return p;
    case HtmlState::raw_text:
    case HtmlState::rcdata: {
      std::string tail = p.raw_tail + c;
      if (tail.size() > p.element.size() + 3) tail.erase(0, tail.size() - p.element.size() - 3);
      p.raw_tail = tail;
      if (ends_raw_element(tail, p.element)) {
        p.html = tail.back() == '>' ? HtmlState::text : HtmlState::end_tag;
        p.raw_tail.clear();
        if (p.html == HtmlState::text) p.element.clear();
        return embedded_reset(std::move(p), Embedded::none);
      }
      if (p.embedded == Embedded::js && tail.ends_with("<!--")) p.js = JsState::ambiguous;
      return embedded_after(std::move(p), c);
    }
  }
  return p;
}

constexpr Place
xml_after(Place p, const char c)
{
  switch (p.xml) {
    case XmlState::text:
      if (c == '<') {
        p.xml = XmlState::tag;
        p.tail = "<";
      }
      return p;
    case XmlState::tag:
      p.tail += c;
      if (p.tail == "<!--") {
        p.xml = XmlState::comment;
        p.tail.clear();
      }
      else if (p.tail == "<![CDATA[") {
        p.xml = XmlState::cdata;
        p.tail.clear();
      }
      else if (p.tail == "<?") p.xml = XmlState::processing_instruction;
      else if (p.tail.size() > 2 && p.tail.starts_with("<!") && !std::string_view("<![CDATA[").starts_with(p.tail) &&
               !std::string_view("<!--").starts_with(p.tail)) {
        p.xml = XmlState::doctype;
        p.doctype_depth = 0;
      }
      else if (c == '"' || c == '\'') {
        p.xml = XmlState::quoted_value;
        p.quote = c;
      }
      else if (c == '>') p.xml = XmlState::text;
      if (p.tail.size() > 9) p.tail.erase(1, p.tail.size() - 9);
      return p;
    case XmlState::quoted_value:
      if (c == p.quote) {
        p.xml = XmlState::tag;
        p.quote = 0;
      }
      return p;
    case XmlState::comment:
    case XmlState::cdata:
    case XmlState::processing_instruction: {
      p.tail += c;
      if (p.tail.size() > 3) p.tail.erase(0, p.tail.size() - 3);
      const std::string_view end = p.xml == XmlState::comment ? "-->" : p.xml == XmlState::cdata ? "]]>" : "?>";
      if (p.tail.ends_with(end)) {
        p.xml = XmlState::text;
        p.tail.clear();
      }
      return p;
    }
    case XmlState::doctype:
      if (c == '[') p.doctype_depth++;
      else if (c == ']') p.doctype_depth--;
      else if (c == '>' && p.doctype_depth <= 0) p.xml = XmlState::text;
      return p;
  }
  return p;
}

constexpr Place
json_after(Place p, const char c)
{
  switch (p.json) {
    case JsonState::outside:
      if (c == '"') p.json = JsonState::string;
      return p;
    case JsonState::string:
      if (c == '\\') p.json = JsonState::string_escape;
      else if (c == '"') p.json = JsonState::outside;
      return p;
    case JsonState::string_escape:
      p.json = JsonState::string;
      return p;
  }
  return p;
}

constexpr Place
place_after(Place p, const Grammar g, const std::string_view text)
{
  for (const char c : text) {
    if (g == Grammar::html) p = html_after(std::move(p), c);
    else if (g == Grammar::xml) p = xml_after(std::move(p), c);
    else if (g == Grammar::json) p = json_after(std::move(p), c);
    else if (g == Grammar::js) p = js_after(std::move(p), c);
    else if (g == Grammar::css) p = css_after(std::move(p), c);
  }
  return p;
}

constexpr std::optional<Context>
context_of(const Place &p, const Grammar g)
{
  switch (g) {
    case Grammar::json:
      if (p.json == JsonState::string) return Context::json_string;
      return std::nullopt;
    case Grammar::xml:
      if (p.xml == XmlState::text) return Context::xml_text;
      if (p.xml == XmlState::quoted_value) return Context::xml_attribute;
      return std::nullopt;
    case Grammar::js:
      if (p.tail.empty() && (p.js == JsState::string || p.js == JsState::template_text)) return Context::js_string;
      return std::nullopt;
    case Grammar::css:
      if (p.css == CssState::string && !p.css_string_is_url) return Context::css_string;
      return std::nullopt;
    case Grammar::html:
      if (p.html == HtmlState::text || p.html == HtmlState::rcdata) return Context::html_text;
      if (p.embedded != Embedded::none && p.embedded_refused) return std::nullopt;
      if ((p.html == HtmlState::raw_text || p.html == HtmlState::quoted_value) && p.embedded == Embedded::js)
        return context_of(p, Grammar::js);
      if ((p.html == HtmlState::raw_text || p.html == HtmlState::quoted_value) && p.embedded == Embedded::css)
        return context_of(p, Grammar::css);
      if (p.html != HtmlState::quoted_value || p.attribute == AttributeKind::unsafe) return std::nullopt;
      if (p.attribute == AttributeKind::plain) return Context::html_attribute;
      if (p.url == UrlState::empty) return Context::html_url_start;
      if (p.url == UrlState::past_scheme) return Context::html_url_rest;
      return std::nullopt;
  }
  return std::nullopt;
}

constexpr bool
is_section(const Tag t)
{
  return t == Tag::section || t == Tag::inverted || t == Tag::block;
}

constexpr std::variant<Place, Refusal>
contexts_assigned_in(Compiled &c, const Grammar g, Place p, const uint32_t from, const uint32_t stop)
{
  uint32_t q = from;
  while (q < stop) {
    Op &op = c.ops.at(q);
    if (op.tag == Tag::text) {
      p = place_after(std::move(p), g, std::string_view(c.texts).substr(op.a, op.b));
      q++;
    }
    else if (op.tag == Tag::var) {
      const std::optional<Context> ctx = context_of(p, g);
      if (!ctx) [[unlikely]] return Refusal{Problem::placeholder_in_unsafe_place, 0};
      op.d = (uint32_t)*ctx;
      if (*ctx == Context::html_url_start) p.url = UrlState::after_value;
      q++;
    }
    else if (op.tag == Tag::raw) {
      op.d = (uint32_t)Context::none;
      q++;
    }
    else if (is_section(op.tag)) {
      std::variant<Place, Refusal> end = contexts_assigned_in(c, g, p, q + 1, op.c);
      if (std::holds_alternative<Refusal>(end)) [[unlikely]] return end;
      if (std::get<Place>(end) != p) [[unlikely]] return Refusal{Problem::context_changes_in_section, 0};
      q = c.ops.at(q).c;
    }
    else {
      if (p != Place{}) [[unlikely]] return Refusal{Problem::context_changes_in_section, 0};
      if (op.tag == Tag::parent) {
        std::variant<Place, Refusal> end = contexts_assigned_in(c, g, Place{}, q + 1, op.c);
        if (std::holds_alternative<Refusal>(end)) [[unlikely]] return end;
        q = c.ops.at(q).c;
      }
      else q++;
    }
  }
  return p;
}

}

constexpr std::variant<Compiled, Refusal>
compile(const std::string_view src, const std::string_view media_type)
{
  const std::optional<Grammar> g = grammar_of(media_type);
  if (!g) [[unlikely]] return Refusal{Problem::unknown_media_type, 0};
  std::variant<Compiled, Refusal> compiled = compile(src);
  if (std::holds_alternative<Refusal>(compiled)) [[unlikely]] return compiled;
  Compiled &c = std::get<Compiled>(compiled);
  const std::variant<detail::Place, Refusal> end =
      detail::contexts_assigned_in(c, *g, detail::Place{}, 0, (uint32_t)c.ops.size());
  if (std::holds_alternative<Refusal>(end)) [[unlikely]] return std::get<Refusal>(end);
  return compiled;
}

}
