#pragma once

#include <cstddef>
#include <expected>
#include <string>
#include <string_view>

#include "containers.hpp"
#include "render.hpp"

#if defined(__cpp_impl_reflection)
#include "reflect.hpp"
#endif

namespace mustache {

inline std::expected<Template, Error>
compile(const std::string_view source)
{
  return compiled_by(source, [source] { return detail::compile(source); });
}

inline std::expected<Template, Error>
compile(const std::string_view source, const std::string_view media_type)
{
  return compiled_by(source, [source, media_type] { return detail::compile(source, media_type); });
}

template <class T>
std::expected<std::string, Error>
render(const Template &t, const T &data, const Partials &partials = {}, const size_t initial_capacity = kInitialCapacity)
{
  return Walk(&partials, kMaxRenderScore, initial_capacity).run(t, detail::containers::value_of(data));
}

}

#if defined(__cpp_impl_reflection)
#include "typed.hpp"
#endif
