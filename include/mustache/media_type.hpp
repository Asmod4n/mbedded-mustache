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

enum class Grammar : uint8_t { html, xml, json, plain };

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

constexpr std::optional<Grammar>
grammar_of(const std::string_view media_type)
{
  using namespace detail;
  const std::string_view essence = ows_trimmed(media_type.substr(0, media_type.find(';')));
  const size_t slash = essence.find('/');
  if (slash == std::string_view::npos) [[unlikely]] return std::nullopt;
  const std::string_view type = essence.substr(0, slash);
  const std::string_view subtype = essence.substr(slash + 1);
  if (!is_restricted_name(type) || !is_restricted_name(subtype)) [[unlikely]] return std::nullopt;
  const std::string t = lowered(type);
  const std::string st = lowered(subtype);
  if (t == "text" && st == "html") return Grammar::html;
  if (t == "application" && st == "xhtml+xml") return Grammar::html;
  if (t == "text" && st == "plain") return Grammar::plain;
  if (t == "application" && st == "json") return Grammar::json;
  if (st.ends_with("+json")) return Grammar::json;
  if ((t == "application" || t == "text") && st == "xml") return Grammar::xml;
  if (t == "image" && st == "svg+xml") return std::nullopt;
  if (st.ends_with("+xml")) return Grammar::xml;
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

enum class AttributeKind : uint8_t { plain, url, unsafe };

enum class UrlState : uint8_t { empty, static_prefix, after_value, past_scheme };

enum class XmlState : uint8_t { text, tag, quoted_value, comment, cdata, processing_instruction, doctype };

enum class JsonState : uint8_t { outside, string, string_escape };

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
  int           doctype_depth = 0;

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
  if (name.starts_with("on") || name == "style" || name == "srcset") return AttributeKind::unsafe;
  if (std::ranges::find(kUrlAttributes, name) != kUrlAttributes.end()) return AttributeKind::url;
  return AttributeKind::plain;
}

constexpr Place
html_tag_closed(Place p)
{
  if (std::ranges::find(kRawTextElements, p.element) != kRawTextElements.end()) p.html = HtmlState::raw_text;
  else if (std::ranges::find(kRcdataElements, p.element) != kRcdataElements.end()) p.html = HtmlState::rcdata;
  else p.html = HtmlState::text;
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
      if (c == '>') p.html = HtmlState::text;
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
      }
      else if (c == '>') return html_tag_closed(p);
      else if (!is_html_space(c)) p.html = HtmlState::unquoted_value;
      return p;
    case HtmlState::quoted_value:
      if (c == p.quote) {
        p.html = HtmlState::before_attribute_name;
        p.quote = 0;
        p.attribute = AttributeKind::plain;
        p.url = UrlState::empty;
      }
      else if (p.attribute == AttributeKind::url && p.url != UrlState::past_scheme) {
        if (c == ':' || c == '/' || c == '?' || c == '#') p.url = UrlState::past_scheme;
        else if (p.url == UrlState::empty) p.url = UrlState::static_prefix;
      }
      return p;
    case HtmlState::unquoted_value:
      if (is_html_space(c)) p.html = HtmlState::before_attribute_name;
      else if (c == '>') return html_tag_closed(p);
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
    case HtmlState::rcdata:
      p.tail += c;
      if (p.tail.size() > p.element.size() + 3) p.tail.erase(0, p.tail.size() - p.element.size() - 3);
      if (ends_raw_element(p.tail, p.element)) {
        p.html = p.tail.back() == '>' ? HtmlState::text : HtmlState::end_tag;
        p.tail.clear();
      }
      return p;
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
  }
  return p;
}

constexpr std::optional<Context>
context_of(const Place &p, const Grammar g)
{
  switch (g) {
    case Grammar::plain:
      return Context::none;
    case Grammar::json:
      if (p.json == JsonState::string) return Context::json_string;
      return std::nullopt;
    case Grammar::xml:
      if (p.xml == XmlState::text) return Context::xml_text;
      if (p.xml == XmlState::quoted_value) return Context::xml_attribute;
      return std::nullopt;
    case Grammar::html:
      if (p.html == HtmlState::text || p.html == HtmlState::rcdata) return Context::html_text;
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
