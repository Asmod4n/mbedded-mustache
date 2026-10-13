#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <span>
#include <string_view>
#include <type_traits>

#include "compile.hpp"
#include "escape.hpp"

namespace mustache {

inline constexpr size_t kSlack = 4096;

constexpr size_t
escaped_size_of(const std::string_view s)
{
  size_t size = 0;
  for (const char c : s) size += entity_length(kEntities.word.at((unsigned char)c));
  return size;
}

constexpr bool
escaped_fits(const std::string_view s, const size_t room)
{
  if (s.size() > room / kEntityMax) [[unlikely]] return escaped_size_of(s) <= room;
  return true;
}

constexpr char *
escaped_into(char *const w, const std::string_view s)
{
  if (std::is_constant_evaluated()) return escape_bytes(w, s);
  return escape_into(w, s);
}

constexpr size_t
escaped_size_max_in(const Context c, const size_t n)
{
  switch (c) {
    case Context::css_string:
    case Context::js_string:      return 4 * n;
    default:                      return kEntityMax * n;
  }
}

constexpr char *
escaped_into_in(const Context c, char *const w, const std::string_view s)
{
  switch (c) {
    case Context::xml_text:       return xml_escaped_into(w, s);
    case Context::json_string:    return json_string_escaped_into(w, s);
    case Context::js_string:      return js_string_escaped_into(w, s);
    case Context::css_string:     return css_string_escaped_into(w, s);
    default:                      return escaped_into(w, s);
  }
}

struct Out {
  char *w;
  char *end;
  bool  full = false;

  constexpr size_t room() const { return (size_t)(end - w); }

  constexpr void raw(const std::string_view s)
  {
    if (room() < s.size()) [[unlikely]] {
      full = true;
      return;
    }
    w = std::copy_n(s.begin(), s.size(), w);
  }

  constexpr void escaped_in(const Context c, const std::string_view s)
  {
    if (escaped_size_max_in(c, s.size()) > room()) [[unlikely]] {
      full = true;
      return;
    }
    w = escaped_into_in(c, w, s);
  }

  constexpr void escaped(const std::string_view s)
  {
    if (!escaped_fits(s, room())) [[unlikely]] {
      full = true;
      return;
    }
    w = escaped_into(w, s);
  }
};

constexpr Out
out_over(const std::span<char> buffer)
{
  if (buffer.size() <= kEscapeSlack) [[unlikely]] return Out{buffer.data(), buffer.data()};
  return Out{buffer.data(), buffer.data() + (buffer.size() - kEscapeSlack)};
}

}
