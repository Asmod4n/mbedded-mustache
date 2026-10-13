#pragma once

#include <cstddef>
#include <expected>
#include <optional>
#include <string>
#include <string_view>

#include "containers.hpp"
#include "render.hpp"

#if defined(__cpp_impl_reflection)
#include "reflect.hpp"
#endif

namespace mustache {

using Template = detail::containers::Program;

using Partials = detail::containers::Partials;

struct Error {
  Fault            fault;
  std::string_view what;
  size_t           asked;
  size_t           allowed;
};

namespace detail::containers {

inline Error
error_of(const Host &h)
{
  return Error{h.fault, h.what, h.asked, h.allowed};
}

}

inline std::expected<Template, Error>
compile(const std::string_view source)
{
  detail::containers::Host h;
  std::optional<Template> t = program_of<detail::containers::Key>(h, source, kSourceMax);
  if (!t) [[unlikely]] return std::unexpected(detail::containers::error_of(h));
  return std::move(*t);
}

inline std::expected<Template, Error>
compile(const std::string_view source, const std::string_view media_type)
{
  detail::containers::Host h;
  std::optional<Template> t = program_of<detail::containers::Key>(h, source, kSourceMax, media_type);
  if (!t) [[unlikely]] return std::unexpected(detail::containers::error_of(h));
  return std::move(*t);
}

template <class T>
std::expected<std::string, Error>
render(const Template &t, const T &data, const Partials &partials = {}, const size_t initial_capacity = kInitialCapacity)
{
  detail::containers::Host h;
  h.partials = &partials;
  const Fault f = Walk<detail::containers::Host, detail::containers::Value, detail::containers::Key>(
                      h, kMaxRenderScore, initial_capacity)
                      .run(t, detail::containers::value_of(data));
  if (f != Fault::none) [[unlikely]] {
    if (h.fault == Fault::none) h.fault = f;
    return std::unexpected(detail::containers::error_of(h));
  }
  return std::move(h.answer);
}

#if defined(__cpp_impl_reflection)

template <fixed_string Src, class... PartialSources, class T>
std::expected<std::string, Error>
render(const T &data, const std::string_view media_type, const size_t initial_capacity = kInitialCapacity)
{
  detail::containers::Host h;
  const Fault f = detail::render_with_host<Src, PartialSources...>(h, data, media_type, initial_capacity);
  if (f != Fault::none) [[unlikely]] {
    if (h.fault == Fault::none) h.fault = f;
    return std::unexpected(detail::containers::error_of(h));
  }
  return std::move(h.answer);
}

template <fixed_string Src, class... PartialSources, class T>
std::expected<std::string, Error>
render(const T &data, const size_t initial_capacity = kInitialCapacity)
{
  detail::containers::Host h;
  const Fault f = detail::render_with_host<Src, PartialSources...>(h, data, initial_capacity);
  if (f != Fault::none) [[unlikely]] {
    if (h.fault == Fault::none) h.fault = f;
    return std::unexpected(detail::containers::error_of(h));
  }
  return std::move(h.answer);
}

#endif

}
