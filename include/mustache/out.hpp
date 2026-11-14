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
  for (const char c : s) size += entity_length(entity_word_of(kEntities, static_cast<unsigned char>(c)));
  return size;
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

namespace detail {

constexpr char *
escaped_into(char *const w, const std::string_view s)
{
  if (std::is_constant_evaluated()) return escape_bytes(w, s);
  return escape_into(w, s);
}

constexpr size_t
escaped_into_in(const Context c, const std::span<char> room, const std::string_view s)
{
  switch (c) {
    case Context::xml_text:       return xml_escaped_into(room, s);
    case Context::json_string:    return json_string_escaped_into(room, s);
    case Context::js_string:      return js_string_escaped_into(room, s);
    case Context::css_string:     return css_string_escaped_into(room, s);
    case Context::troff_text:     return troff_text_escaped_into(room, s);
    default:                      return static_cast<size_t>(std::distance(room.data(), escaped_into(room.data(), s)));
  }
}

}

constexpr bool
room_holds_the_escaped_value(const Context c, const std::string_view s, const size_t room)
{
  if (s.empty()) return true;
  if (room < kBytesPastTheWrittenCount) [[unlikely]] return false;
  const size_t usable = room - kBytesPastTheWrittenCount;
  if (s.size() <= usable / escaped_size_max_in(c, 1)) [[likely]] return true;
  if (c != Context::html_text) [[unlikely]] return false;
  return escaped_size_of(s) <= usable;
}

constexpr size_t
room_needed_for_the_escaped_value(const Context c, const std::string_view s)
{
  if (c == Context::html_text) return escaped_size_of(s);
  return escaped_size_max_in(c, s.size());
}

constexpr bool
room_holds_the_copied_value(const std::string_view s, const size_t room)
{
  return s.size() <= room;
}

struct Out {
  char *w;
  char *end;
  bool  full = false;

  constexpr size_t room() const { return (size_t)(end - w); }

  constexpr void raw(const std::string_view s)
  {
    if (!room_holds_the_copied_value(s, room())) [[unlikely]] {
      full = true;
      return;
    }
    w = std::copy_n(s.begin(), s.size(), w);
  }

  constexpr void escaped_in(const Context c, const std::string_view s)
  {
    if (!room_holds_the_escaped_value(c, s, room())) [[unlikely]] {
      full = true;
      return;
    }
    const std::span<char> room(w, end);
    w = room.subspan(detail::escaped_into_in(c, room, s)).data();
  }

  constexpr void escaped(const std::string_view s)
  {
    if (!room_holds_the_escaped_value(Context::html_text, s, room())) [[unlikely]] {
      full = true;
      return;
    }
    w = detail::escaped_into(w, s);
  }
};

constexpr Out
out_over(const std::span<char> buffer)
{
  if (buffer.size() <= kEscapeSlack) [[unlikely]] return Out{buffer.data(), buffer.data()};
  return Out{buffer.data(), buffer.data() + (buffer.size() - kEscapeSlack)};
}

}
