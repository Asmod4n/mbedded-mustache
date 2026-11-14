// The targets of render_to: a string, a byte vector or a stream that the caller owns.
// A target is written once, after the render has succeeded.
#pragma once

#include <cstddef>
#include <expected>
#include <string>
#include <string_view>

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

}
