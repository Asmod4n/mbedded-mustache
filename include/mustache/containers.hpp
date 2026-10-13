#pragma once

#include <array>
#include <charconv>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <variant>

namespace mustache {

enum class Kind : uint8_t { falsy, text, list, map, truthy };

}

namespace mustache::detail::containers {

using mustache::Kind;

template <class... A>
const std::variant<A...> &variant_base(const std::variant<A...> &v)
{
  return v;
}

template <class T>
concept variant_like = requires(const T &v) { variant_base(v); };

template <class T>
concept text_like = std::is_convertible_v<const T &, std::string_view>;

template <class T>
concept number_like = std::is_arithmetic_v<T> && !std::same_as<T, bool> && !std::same_as<T, char>;

template <class T>
concept falsy_like = std::same_as<T, std::nullptr_t> || std::same_as<T, std::monostate>;

template <class T>
inline constexpr bool is_optional_v = false;
template <class T>
inline constexpr bool is_optional_v<std::optional<T>> = true;

template <class T>
concept map_like = requires(const T &m, const std::string &k) {
  typename T::key_type;
  typename T::mapped_type;
  { m.find(k) } -> std::same_as<typename T::const_iterator>;
  { m.end() } -> std::same_as<typename T::const_iterator>;
  { m.empty() } -> std::convertible_to<bool>;
} && std::is_convertible_v<const typename T::key_type &, std::string_view>;

template <class T>
concept list_like = std::ranges::random_access_range<const T> &&
                    std::is_lvalue_reference_v<std::ranges::range_reference_t<const T>> && !text_like<T> &&
                    !map_like<T>;

inline constexpr size_t kNumberTextMax = 32;

using NumberText = std::span<char, kNumberTextMax>;

struct Table;

struct Value {
  const void  *data;
  const Table *table;
};

struct Table {
  Kind (*kind_of)(const void *);
  std::string_view (*text_of)(const void *, NumberText);
  size_t (*size_of)(const void *);
  Value (*element)(const void *, size_t);
  std::optional<Value> (*find)(const void *, const std::string &);
};

template <class T>
Value value_of(const T &v);

template <class T>
struct TableOf {
  static const T &of(const void *const p) { return *static_cast<const T *>(p); }

  static Kind kind_of(const void *const p)
  {
    const T &v = of(p);
    if constexpr (variant_like<T>) {
      return std::visit([](const auto &x) { return value_of(x).table->kind_of(&x); }, variant_base(v));
    }
    else if constexpr (std::same_as<T, bool>) {
      return v ? Kind::truthy : Kind::falsy;
    }
    else if constexpr (number_like<T> || text_like<T>) {
      return Kind::text;
    }
    else if constexpr (falsy_like<T>) {
      return Kind::falsy;
    }
    else if constexpr (is_optional_v<T>) {
      return v ? value_of(*v).table->kind_of(std::addressof(*v)) : Kind::falsy;
    }
    else if constexpr (map_like<T>) {
      return v.empty() ? Kind::falsy : Kind::map;
    }
    else {
      static_assert(list_like<T>, "a value is text, a number, a bool, an optional, a variant, a map or a list");
      return Kind::list;
    }
  }

  static std::string_view text_of(const void *const p, const NumberText text)
  {
    const T &v = of(p);
    if constexpr (variant_like<T>) {
      return std::visit([text](const auto &x) { return value_of(x).table->text_of(&x, text); }, variant_base(v));
    }
    else if constexpr (text_like<T>) {
      return std::string_view(v);
    }
    else if constexpr (number_like<T>) {
      const std::to_chars_result r = std::to_chars(text.data(), std::to_address(text.end()), v);
      return {text.data(), r.ptr};
    }
    else if constexpr (is_optional_v<T>) {
      return v ? value_of(*v).table->text_of(std::addressof(*v), text) : std::string_view();
    }
    else {
      return {};
    }
  }

  static size_t size_of(const void *const p)
  {
    const T &v = of(p);
    if constexpr (variant_like<T>) {
      return std::visit([](const auto &x) { return value_of(x).table->size_of(&x); }, variant_base(v));
    }
    else if constexpr (is_optional_v<T>) {
      return v ? value_of(*v).table->size_of(std::addressof(*v)) : 0;
    }
    else if constexpr (list_like<T>) {
      return (size_t)std::ranges::size(v);
    }
    else {
      return 0;
    }
  }

  static Value element(const void *const p, const size_t i)
  {
    const T &v = of(p);
    if constexpr (variant_like<T>) {
      return std::visit([i](const auto &x) { return value_of(x).table->element(&x, i); }, variant_base(v));
    }
    else if constexpr (is_optional_v<T>) {
      return value_of(*v).table->element(std::addressof(*v), i);
    }
    else if constexpr (list_like<T>) {
      return value_of(*std::ranges::next(std::ranges::begin(v), (std::ptrdiff_t)i));
    }
    else {
      return value_of(v);
    }
  }

  static std::optional<Value> find(const void *const p, const std::string &key)
  {
    const T &v = of(p);
    if constexpr (variant_like<T>) {
      return std::visit([&key](const auto &x) { return value_of(x).table->find(&x, key); }, variant_base(v));
    }
    else if constexpr (is_optional_v<T>) {
      if (!v) return std::nullopt;
      return value_of(*v).table->find(std::addressof(*v), key);
    }
    else if constexpr (map_like<T>) {
      const auto it = v.find(key);
      if (it == v.end()) return std::nullopt;
      return value_of(it->second);
    }
    else {
      return std::nullopt;
    }
  }

  static constexpr Table kTable{kind_of, text_of, size_of, element, find};
};

template <class T>
Value
value_of(const T &v)
{
  return Value{std::addressof(v), &TableOf<T>::kTable};
}

}
