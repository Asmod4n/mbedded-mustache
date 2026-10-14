#pragma once

#include <array>
#include <charconv>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <meta>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "containers.hpp"
#include "render.hpp"

namespace mustache {

namespace detail::typed {

using containers::is_optional_v;
using containers::list_like;
using containers::number_like;
using containers::text_like;

template <class T>
concept leaf_like = text_like<T> || number_like<T> || std::same_as<T, bool>;

template <class T>
concept struct_like = std::is_class_v<T> && !leaf_like<T> && !list_like<T> && !is_optional_v<T>;

template <class... Ts>
struct types {};

template <class A, class B>
struct joined;
template <class... A, class... B>
struct joined<types<A...>, types<B...>> {
  using type = types<A..., B...>;
};

consteval std::meta::info
member_types_of(const std::meta::info t)
{
  std::vector<std::meta::info> found;
  for (const std::meta::info m : std::meta::nonstatic_data_members_of(t, std::meta::access_context::unprivileged())) {
    found.push_back(std::meta::remove_cvref(std::meta::type_of(m)));
  }
  return std::meta::substitute(^^types, found);
}

template <class T>
struct children {
  using type = types<>;
};
template <class T>
  requires is_optional_v<T>
struct children<T> {
  using type = types<typename T::value_type>;
};
template <class T>
  requires list_like<T>
struct children<T> {
  using type = types<std::remove_cvref_t<std::ranges::range_reference_t<const T>>>;
};
template <class T>
  requires struct_like<T>
struct children<T> {
  using type = [:member_types_of(^^T):];
};

template <class Seen, class Todo>
struct closure;
template <class... S>
struct closure<types<S...>, types<>> {
  using type = types<S...>;
};
template <class... S, class T, class... R>
struct closure<types<S...>, types<T, R...>> {
  using type = typename std::conditional_t<
      (std::same_as<T, S> || ...), closure<types<S...>, types<R...>>,
      closure<types<S..., T>, typename joined<types<R...>, typename children<T>::type>::type>>::type;
};

template <class T, class List>
struct index_in;
template <class T, class... Ts>
struct index_in<T, types<Ts...>> {
  static constexpr uint32_t value = []() {
    constexpr std::array<bool, sizeof...(Ts)> same = {std::same_as<T, Ts>...};
    return (uint32_t)std::distance(same.begin(), std::ranges::find(same, true));
  }();
};

inline constexpr uint32_t kNone = std::numeric_limits<uint32_t>::max();

struct Step {
  uint32_t type;
  uint32_t member;
};

struct Binding {
  uint32_t level = 0;
  uint32_t first = 0;
  uint32_t count = 0;
  uint32_t type = kNone;
  uint32_t pushes = kNone;
};

template <class List>
struct Types;
template <class... Ts>
struct Types<types<Ts...>> {
  static constexpr size_t size = sizeof...(Ts);

  template <class F>
  static void with_type(const uint32_t index, const void *const p, const F &f)
  {
    [&]<size_t... I>(std::index_sequence<I...>) {
      (void)((index == I ? (f(*static_cast<const Ts *>(p)), true) : false) || ...);
    }(std::index_sequence_for<Ts...>{});
  }

  template <class T>
  static constexpr uint32_t index_of = index_in<T, types<Ts...>>::value;

  template <class T>
  static std::optional<Step> member_named(const std::string_view name)
  {
    if constexpr (struct_like<T>) {
      uint32_t i = 0;
      std::optional<Step> found;
      template for (constexpr std::meta::info m : std::define_static_array(
                        std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::unprivileged()))) {
        if (!found && std::meta::identifier_of(m) == name) {
          found = Step{index_of<std::remove_cvref_t<typename [:std::meta::type_of(m):]>>, i};
        }
        i++;
      }
      return found;
    }
    else {
      return std::nullopt;
    }
  }

  static std::optional<Step> member_named(const uint32_t type, const std::string_view name)
  {
    std::optional<Step> found;
    [&]<size_t... I>(std::index_sequence<I...>) {
      (void)((type == I ? (found = member_named<Ts>(name), true) : false) || ...);
    }(std::index_sequence_for<Ts...>{});
    return found;
  }

  static uint32_t pushed_by(const uint32_t type)
  {
    uint32_t pushed = kNone;
    [&]<size_t... I>(std::index_sequence<I...>) {
      (void)((type == I ? (pushed = pushed_by_type<Ts>(), true) : false) || ...);
    }(std::index_sequence_for<Ts...>{});
    return pushed;
  }

  template <class T>
  static constexpr uint32_t pushed_by_type()
  {
    if constexpr (list_like<T>) return index_of<std::remove_cvref_t<std::ranges::range_reference_t<const T>>>;
    else if constexpr (struct_like<T>) return index_of<T>;
    else if constexpr (is_optional_v<T>) {
      if constexpr (struct_like<typename T::value_type>) return index_of<typename T::value_type>;
      else return kNone;
    }
    else return kNone;
  }

  template <class T>
  static const void *member_at(const T &v, const uint32_t index)
  {
    const void *found = nullptr;
    uint32_t i = 0;
    template for (constexpr std::meta::info m : std::define_static_array(
                      std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::unprivileged()))) {
      if (i == index) found = std::addressof(v.[:m:]);
      i++;
    }
    return found;
  }

  static const void *followed(const uint32_t type, const void *const p, const uint32_t member)
  {
    const void *found = nullptr;
    with_type(type, p, [&](const auto &v) {
      if constexpr (struct_like<std::remove_cvref_t<decltype(v)>>) found = member_at(v, member);
    });
    return found;
  }
};

}

template <class T>
struct TypedTemplate {
  using Closure = typename detail::typed::closure<detail::typed::types<>, detail::typed::types<T>>::type;
  using Table = detail::typed::Types<Closure>;

  Template                             program;
  std::vector<detail::typed::Binding>  bindings;
  std::vector<detail::typed::Step>     steps;
};

template <class T>
std::expected<TypedTemplate<T>, Error>
typed_template(Template program)
{
  using namespace detail::typed;
  using Table = typename TypedTemplate<T>::Table;
  TypedTemplate<T> b{std::move(program), {}, {}};
  const Template &p = b.program;
  b.bindings.resize(p.ops.size());
  std::vector<uint32_t> stack{Table::template index_of<T>};
  std::vector<std::pair<uint32_t, bool>> open;
  for (uint32_t pc = 0; pc < p.ops.size(); pc++) {
    while (!open.empty() && open.back().first == pc) {
      if (open.back().second) stack.pop_back();
      open.pop_back();
    }
    const Op &op = p.ops.at(pc);
    if (op.tag == Tag::partial || op.tag == Tag::parent || op.tag == Tag::block) [[unlikely]] {
      return std::unexpected(Error{Fault::parse, "a typed template has no partials yet", pc, 0});
    }
    if (op.tag != Tag::var && op.tag != Tag::raw && op.tag != Tag::section && op.tag != Tag::inverted) continue;
    Binding &bind = b.bindings.at(pc);
    bind.first = (uint32_t)b.steps.size();
    if (op.b == 0) {
      bind.level = (uint32_t)stack.size() - 1;
      bind.type = stack.back();
    }
    else {
      std::optional<Step> found;
      for (size_t i = stack.size(); i-- > 0 && !found;) {
        found = Table::member_named(stack.at(i), p.keys.at(op.a));
        if (found) bind.level = (uint32_t)i;
      }
      if (!found) [[unlikely]] return std::unexpected(Error{Fault::parse, "a key names no member", pc, 0});
      b.steps.push_back(Step{stack.at(bind.level), found->member});
      uint32_t type = found->type;
      for (uint32_t j = 1; j < op.b; j++) {
        const std::optional<Step> next = Table::member_named(type, p.keys.at(op.a + j));
        if (!next) [[unlikely]] return std::unexpected(Error{Fault::parse, "a key names no member", pc, 0});
        b.steps.push_back(Step{type, next->member});
        type = next->type;
      }
      bind.type = type;
    }
    bind.count = (uint32_t)b.steps.size() - bind.first;
    if (op.tag == Tag::section) {
      bind.pushes = Table::pushed_by(bind.type);
      if (bind.pushes != kNone) stack.push_back(bind.pushes);
      open.emplace_back(op.c, bind.pushes != kNone);
    }
    else if (op.tag == Tag::inverted) {
      open.emplace_back(op.c, false);
    }
  }
  return b;
}

template <class T>
std::expected<TypedTemplate<T>, Error>
compile(const std::string_view source)
{
  std::expected<Template, Error> t = compile(source);
  if (!t) [[unlikely]] return std::unexpected(t.error());
  return typed_template<T>(std::move(*t));
}

template <class T>
std::expected<TypedTemplate<T>, Error>
compile(const std::string_view source, const std::string_view media_type)
{
  std::expected<Template, Error> t = compile(source, media_type);
  if (!t) [[unlikely]] return std::unexpected(t.error());
  return typed_template<T>(std::move(*t));
}

namespace detail::typed {

template <class T>
class TypedWalk {
public:
  using Table = typename TypedTemplate<T>::Table;

  TypedWalk(const TypedTemplate<T> &b, const size_t initial_capacity) : b_(b), initial_capacity_(initial_capacity) {}

  std::expected<std::string, Error> run(const T &data)
  {
    stack_.at(0) = std::addressof(data);
    if (!out_.opened(initial_capacity_)) [[unlikely]] return std::unexpected(out_.error());
    run(0, (uint32_t)b_.program.ops.size(), 1);
    if (fault_ != Fault::none) [[unlikely]] return std::unexpected(Error{fault_, "a value is not text", 0, 0});
    if (out_.fault != Fault::none) [[unlikely]] return std::unexpected(out_.error());
    return out_.closed();
  }

private:
  const TypedTemplate<T> &b_;
  size_t           initial_capacity_;
  Answer           out_;
  std::array<const void *, kMaxDepth> stack_{};
  Fault            fault_ = Fault::none;

  const void *value_of(const Binding &bind) const
  {
    const void *p = stack_.at(bind.level);
    for (uint32_t i = 0; i < bind.count; i++) {
      const Step s = b_.steps.at(bind.first + i);
      p = Table::followed(s.type, p, s.member);
    }
    return p;
  }

  template <class V>
  void interpolate(const Op &op, const V &v)
  {
    if constexpr (is_optional_v<V>) {
      if (v) interpolate(op, *v);
    }
    else if constexpr (number_like<V>) {
      std::array<char, 32> digits{};
      const std::to_chars_result r = std::to_chars(digits.data(), std::to_address(digits.end()), v);
      emit(op, std::string_view(digits.data(), r.ptr));
    }
    else if constexpr (text_like<V>) {
      emit(op, std::string_view(v));
    }
    else {
      fault_ = Fault::not_text;
    }
  }

  void emit(const Op &op, const std::string_view s)
  {
    if (s.empty()) return;
    if (op.tag == Tag::var) out_.escaped_in(b_.program.context, s);
    else out_.raw(s);
  }

  template <class V>
  static bool is_empty(const V &v)
  {
    if constexpr (std::same_as<V, bool>) return !v;
    else if constexpr (is_optional_v<V>) return !v;
    else if constexpr (list_like<V>) return std::ranges::empty(v);
    else if constexpr (text_like<V>) return std::string_view(v).empty();
    else return false;
  }

  bool run(const uint32_t from, const uint32_t stop, const uint32_t depth)
  {
    const Template &p = b_.program;
    uint32_t pc = from;
    while (pc < stop) {
      if (fault_ != Fault::none || out_.fault != Fault::none) [[unlikely]] return false;
      const Op &op = p.ops.at(pc);
      switch (op.tag) {
        case Tag::text:
          out_.raw(std::string_view(p.texts).substr(op.a, op.b));
          pc++;
          break;
        case Tag::var:
        case Tag::raw: {
          const Binding &bind = b_.bindings.at(pc);
          Table::with_type(bind.type, value_of(bind), [&](const auto &v) { interpolate(op, v); });
          pc++;
          break;
        }
        case Tag::section: {
          const Binding &bind = b_.bindings.at(pc);
          Table::with_type(bind.type, value_of(bind), [&](const auto &v) { section(op, pc, depth, v); });
          pc = op.c;
          break;
        }
        case Tag::inverted: {
          const Binding &bind = b_.bindings.at(pc);
          bool empty = false;
          Table::with_type(bind.type, value_of(bind), [&](const auto &v) { empty = is_empty(v); });
          if (empty) run(pc + 1, op.c, depth);
          pc = op.c;
          break;
        }
        default:
          pc++;
          break;
      }
    }
    return true;
  }

  template <class V>
  void section(const Op &op, const uint32_t pc, const uint32_t depth, const V &v)
  {
    if constexpr (list_like<V>) {
      if (depth >= kMaxDepth) [[unlikely]] {
        fault_ = Fault::too_deep;
        return;
      }
      for (const auto &e : v) {
        stack_.at(depth) = std::addressof(e);
        if (!run(pc + 1, op.c, depth + 1)) [[unlikely]] return;
      }
    }
    else if constexpr (struct_like<V>) {
      if (depth >= kMaxDepth) [[unlikely]] {
        fault_ = Fault::too_deep;
        return;
      }
      stack_.at(depth) = std::addressof(v);
      run(pc + 1, op.c, depth + 1);
    }
    else if constexpr (is_optional_v<V>) {
      if (!v) return;
      if constexpr (struct_like<typename V::value_type>) {
        if (depth >= kMaxDepth) [[unlikely]] {
          fault_ = Fault::too_deep;
          return;
        }
        stack_.at(depth) = std::addressof(*v);
        run(pc + 1, op.c, depth + 1);
      }
      else {
        run(pc + 1, op.c, depth);
      }
    }
    else {
      if (!is_empty(v)) run(pc + 1, op.c, depth);
    }
  }
};

}

template <class T>
std::expected<std::string, Error>
render(const TypedTemplate<T> &b, const T &data, const size_t initial_capacity = kInitialCapacity)
{
  return detail::typed::TypedWalk<T>(b, initial_capacity).run(data);
}

}
