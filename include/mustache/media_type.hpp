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

constexpr Context
context_of(const Grammar g)
{
  switch (g) {
    case Grammar::html: return Context::html_text;
    case Grammar::xml:  return Context::xml_text;
    case Grammar::json: return Context::json_string;
    case Grammar::js:   return Context::js_string;
    case Grammar::css:  return Context::css_string;
  }
  return Context::html_text;
}

namespace detail {

constexpr std::variant<Compiled, Refusal>
compile(const std::string_view src, const std::string_view media_type)
{
  const std::optional<Grammar> g = grammar_of(media_type);
  if (!g) [[unlikely]] return Refusal{Problem::unknown_media_type, 0};
  std::variant<Compiled, Refusal> compiled = detail::compile(src);
  if (std::holds_alternative<Refusal>(compiled)) [[unlikely]] return compiled;
  Compiled &c = std::get<Compiled>(compiled);
  c.context = context_of(*g);
  for (Op &op : c.ops) {
    if (op.tag == Tag::var) op.d = (uint32_t)c.context;
  }
  return compiled;
}

}

}
