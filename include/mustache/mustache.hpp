#pragma once

#include <cstddef>
#include <expected>
#include <string>
#include <string_view>

#include "containers.hpp"
#include "render.hpp"
#include "target.hpp"

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
  return Walk<>(detail::containers::Access{}, &partials, kMaxRenderScore, initial_capacity).run(t, detail::containers::value_of(data));
}

namespace detail {

template <class T>
struct RunTimeAppender {
  const Template &t;
  const T        &data;
  const Partials &partials;
  size_t          initial_capacity;

  std::expected<void, Error> operator()(std::string &out) const
  {
    return Walk<>(containers::Access{}, &partials, kMaxRenderScore, initial_capacity)
        .appended_to(out, t, containers::value_of(data));
  }
};

}

/// Provisional name and comment. Appends the rendered bytes to `out`, as render does with
/// the same arguments. On an error `out` is as it was before the call. A std::string keeps
/// its capacity and takes the render without a copy; another string type takes one copy.
template <class Traits, class Allocator, class T>
std::expected<void, Error>
render_to(std::basic_string<char, Traits, Allocator> &out, const Template &t, const T &data, const Partials &partials = {},
          const size_t initial_capacity = kInitialCapacity)
{
  return detail::appended_to(out, detail::RunTimeAppender<T>{t, data, partials, initial_capacity});
}

/// Provisional name and comment. Appends the rendered bytes to `out`, as render does with
/// the same arguments. On an error `out` is as it was before the call. The render is
/// made in a temporary std::string and copied once.
template <detail::byte_element Element, class Allocator, class T>
std::expected<void, Error>
render_to(std::vector<Element, Allocator> &out, const Template &t, const T &data, const Partials &partials = {},
          const size_t initial_capacity = kInitialCapacity)
{
  return detail::appended_to(out, detail::RunTimeAppender<T>{t, data, partials, initial_capacity});
}

}

#if defined(__cpp_impl_reflection) || __has_include(<boost/pfr/core_name.hpp>)
#include "typed.hpp"
#endif
