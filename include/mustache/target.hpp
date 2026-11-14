// The targets of render_to: a string, a byte vector or a stream that the caller owns.
// A target is written once, after the render has succeeded.
#pragma once

#include <concepts>
#include <cstddef>
#include <expected>
#include <ios>
#include <limits>
#include <ostream>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "render.hpp"

namespace mustache::detail {

template <class Appender>
std::expected<void, Error>
appended_to(std::string &out, const Appender &appender)
{
  return appender(out);
}

template <class Appender>
std::expected<std::string, Error>
rendered_through(const Appender &appender)
{
  std::string rendered;
  if (const std::expected<void, Error> r = appender(rendered); !r) [[unlikely]] return std::unexpected(r.error());
  return rendered;
}

template <class Traits, class Allocator, class Appender>
std::expected<void, Error>
appended_to(std::basic_string<char, Traits, Allocator> &out, const Appender &appender)
{
  const std::expected<std::string, Error> rendered = rendered_through(appender);
  if (!rendered) [[unlikely]] return std::unexpected(rendered.error());
  if (!memory_sufficed([&] { out.append(rendered->data(), rendered->size()); })) [[unlikely]]
    return std::unexpected(Error{Fault::no_memory, "render output", out.size() + rendered->size(), 0});
  return {};
}

template <class Element>
concept byte_element = std::same_as<Element, char> || std::same_as<Element, std::byte>;

template <byte_element Element, class Allocator, class Appender>
std::expected<void, Error>
appended_to(std::vector<Element, Allocator> &out, const Appender &appender)
{
  const std::expected<std::string, Error> rendered = rendered_through(appender);
  if (!rendered) [[unlikely]] return std::unexpected(rendered.error());
  const auto inserted = [&](const auto bytes) { out.insert(out.end(), bytes.begin(), bytes.end()); };
  const auto copied = [&] {
    if constexpr (std::is_same_v<Element, char>) inserted(std::string_view(*rendered));
    else inserted(std::as_bytes(std::span<const char>(*rendered)));
  };
  if (!memory_sufficed(copied)) [[unlikely]]
    return std::unexpected(Error{Fault::no_memory, "render output", out.size() + rendered->size(), 0});
  return {};
}

static_assert(kMaxCapacity <= static_cast<size_t>(std::numeric_limits<std::streamsize>::max()),
              "the bytes of a render fit in a std::streamsize");

template <class Appender>
std::expected<void, Error>
appended_to(std::ostream &out, const Appender &appender)
{
  const std::expected<std::string, Error> rendered = rendered_through(appender);
  if (!rendered) [[unlikely]] return std::unexpected(rendered.error());
  out.write(rendered->data(), static_cast<std::streamsize>(rendered->size()));
  return {};
}

}
