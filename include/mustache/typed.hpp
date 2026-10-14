#pragma once

#include <array>
#include <charconv>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <expected>
#if defined(__cpp_impl_reflection)
#include <meta>
#else
#include <boost/pfr/core.hpp>
#include <boost/pfr/core_name.hpp>
#endif
#include <optional>
#include <tuple>
#include <variant>
#include <algorithm>
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
template <class T>
concept list_like = containers::list_like<T> && !containers::is_optional_v<T>;
using containers::number_like;
using containers::text_like;

template <class T>
concept leaf_like = text_like<T> || number_like<T> || std::same_as<T, bool>;

using containers::falsy_like;
using containers::map_like;
using containers::variant_base;
using containers::variant_like;

template <class T>
concept struct_like = std::is_class_v<T> && std::is_aggregate_v<T> && !leaf_like<T> && !list_like<T> && !is_optional_v<T> &&
                      !map_like<T> && !variant_like<T> && !falsy_like<T>;

template <class... Ts>
struct types {};

template <class A, class B>
struct joined;
template <class... A, class... B>
struct joined<types<A...>, types<B...>> {
  using type = types<A..., B...>;
};

#if defined(__cpp_impl_reflection)
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
consteval auto
members_of_type()
{
  return std::define_static_array(std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::unprivileged()));
}

template <class T>
inline constexpr size_t member_count = members_of_type<T>().size();

template <class T, size_t I>
using member_type = std::remove_cvref_t<typename[:std::meta::type_of(members_of_type<T>()[I]):]>;

template <class T, size_t I>
constexpr std::string_view member_name = std::meta::identifier_of(members_of_type<T>()[I]);

template <size_t I, class T>
const auto &
member_at_index(const T &v)
{
  return v.[:members_of_type<T>()[I]:];
}
#else
template <class T>
inline constexpr size_t member_count = boost::pfr::tuple_size_v<T>;

template <class T, size_t I>
using member_type = std::remove_cvref_t<boost::pfr::tuple_element_t<I, T>>;

template <class T, size_t I>
constexpr std::string_view member_name = boost::pfr::get_name<I, T>();

template <size_t I, class T>
const auto &
member_at_index(const T &v)
{
  return boost::pfr::get<I>(v);
}
#endif

template <class T, size_t... I>
types<member_type<T, I>...> member_types_in(std::index_sequence<I...>);

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
  requires variant_like<T>
struct children<T> {
  using type = decltype([]<class... A>(const std::variant<A...> &) { return types<A...>{}; }(variant_base(std::declval<const T &>())));
};
template <class T>
  requires(map_like<T> && !variant_like<T>)
struct children<T> {
  using type = types<typename T::mapped_type>;
};
template <class T>
  requires(list_like<T> && !variant_like<T> && !map_like<T>)
struct children<T> {
  using type = types<std::remove_cvref_t<std::ranges::range_reference_t<const T>>>;
};
template <class T>
  requires struct_like<T>
struct children<T> {
  using type = decltype(member_types_in<T>(std::make_index_sequence<member_count<T>>{}));
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
  bool     fixed = false;
  uint32_t level = 0;
  uint32_t first = 0;
  uint32_t count = 0;
  uint32_t type = kNone;
};

struct Entry {
  const void *p = nullptr;
  uint32_t    type = kNone;
};

template <class List>
struct Types;
template <class... Ts>
struct Types<types<Ts...>> {
  template <class T>
  static constexpr uint32_t index_of = index_in<T, types<Ts...>>::value;

  template <class F>
  static decltype(auto) with_type(const Entry e, const F &f)
  {
    using R = std::invoke_result_t<const F &, const std::tuple_element_t<0, std::tuple<Ts...>> &>;
    if constexpr (std::is_void_v<R>) {
      [&]<size_t... I>(std::index_sequence<I...>) {
        (void)((e.type == I ? (f(*static_cast<const Ts *>(e.p)), true) : false) || ...);
      }(std::index_sequence_for<Ts...>{});
    }
    else {
      R r{};
      [&]<size_t... I>(std::index_sequence<I...>) {
        (void)((e.type == I ? (r = f(*static_cast<const Ts *>(e.p)), true) : false) || ...);
      }(std::index_sequence_for<Ts...>{});
      return r;
    }
  }

  template <class X>
  static Entry entry(const X &x)
  {
    return Entry{std::addressof(x), index_of<X>};
  }

  template <class T>
  static std::optional<Step> member_named(const std::string_view name)
  {
    if constexpr (struct_like<T>) {
      std::optional<Step> found;
      [&]<size_t... I>(std::index_sequence<I...>) {
        (void)((member_name<T, I> == name ? (found = Step{index_of<member_type<T, I>>, (uint32_t)I}, true) : false) || ...);
      }(std::make_index_sequence<member_count<T>>{});
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

  template <class T>
  static constexpr bool fixed_type()
  {
    if constexpr (is_optional_v<T>) return leaf_like<typename T::value_type>;
    else return struct_like<T> || leaf_like<T> || list_like<T> || falsy_like<T>;
  }

  static bool is_fixed(const uint32_t type)
  {
    bool fixed = false;
    [&]<size_t... I>(std::index_sequence<I...>) {
      (void)((type == I ? (fixed = fixed_type<Ts>(), true) : false) || ...);
    }(std::index_sequence_for<Ts...>{});
    return fixed;
  }

  template <class T>
  static constexpr uint32_t pushed_type(const bool inverted)
  {
    if (inverted) return kNone;
    if constexpr (list_like<T>) return index_of<std::remove_cvref_t<std::ranges::range_reference_t<const T>>>;
    else return index_of<T>;
  }

  static uint32_t pushed_by(const uint32_t type)
  {
    uint32_t pushed = kNone;
    [&]<size_t... I>(std::index_sequence<I...>) {
      (void)((type == I ? (pushed = pushed_type<Ts>(false), true) : false) || ...);
    }(std::index_sequence_for<Ts...>{});
    return pushed;
  }

  template <class T>
  static const void *member_at(const T &v, const uint32_t index)
  {
    const void *found = nullptr;
    if constexpr (struct_like<T>) {
      [&]<size_t... I>(std::index_sequence<I...>) {
        (void)((index == I ? (found = std::addressof(member_at_index<I>(v)), true) : false) || ...);
      }(std::make_index_sequence<member_count<T>>{});
    }
    return found;
  }

  static const void *followed(const Step s, const void *const p)
  {
    return with_type(Entry{p, s.type}, [&](const auto &v) { return member_at(v, s.member); });
  }
};

template <class T>
class Access;

}

template <class T>
struct TypedTemplate {
  using Closure = typename detail::typed::closure<detail::typed::types<>, detail::typed::types<T>>::type;
  using Table = detail::typed::Types<Closure>;

  Template                            program;
  std::vector<detail::typed::Binding> bindings;
  std::vector<detail::typed::Step>    steps;
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
  if (std::ranges::any_of(p.ops, [](const Op &op) { return op.tag == Tag::parent || op.tag == Tag::block; })) return b;
  std::vector<uint32_t> stack{Table::template index_of<T>};
  std::vector<std::pair<uint32_t, bool>> open;
  for (uint32_t pc = 0; pc < p.ops.size(); pc++) {
    while (!open.empty() && open.back().first == pc) {
      if (open.back().second) stack.pop_back();
      open.pop_back();
    }
    const Op &op = p.ops.at(pc);
    if (op.tag != Tag::var && op.tag != Tag::raw && op.tag != Tag::section && op.tag != Tag::inverted) continue;
    Binding &bind = b.bindings.at(pc);
    bind.first = (uint32_t)b.steps.size();
    bool fixed = true;
    if (op.b == 0) {
      bind.level = (uint32_t)stack.size() - 1;
      bind.type = stack.back();
      fixed = bind.type != kNone;
    }
    else {
      std::optional<Step> found;
      for (size_t i = stack.size(); i-- > 0;) {
        if (stack.at(i) == kNone || !Table::is_fixed(stack.at(i))) {
          fixed = false;
          break;
        }
        found = Table::member_named(stack.at(i), p.keys.at(op.a));
        if (found) {
          bind.level = (uint32_t)i;
          break;
        }
      }
      if (fixed && !found) [[unlikely]] return std::unexpected(Error{Fault::parse, "a key names no member", pc, 0});
      if (fixed) {
        b.steps.push_back(Step{stack.at(bind.level), found->member});
        uint32_t type = found->type;
        for (uint32_t j = 1; j < op.b && fixed; j++) {
          if (!Table::is_fixed(type)) {
            fixed = false;
            break;
          }
          const std::optional<Step> next = Table::member_named(type, p.keys.at(op.a + j));
          if (!next) [[unlikely]] return std::unexpected(Error{Fault::parse, "a key names no member", pc, 0});
          b.steps.push_back(Step{type, next->member});
          type = next->type;
        }
        bind.type = type;
      }
    }
    if (!fixed) b.steps.resize(bind.first);
    bind.fixed = fixed;
    bind.count = (uint32_t)b.steps.size() - bind.first;
    if (op.tag == Tag::section) {
      const uint32_t pushed = fixed ? Table::pushed_by(bind.type) : kNone;
      stack.push_back(pushed);
      open.emplace_back(op.c, true);
    }
    else if (op.tag == Tag::inverted) {
      stack.push_back(stack.back());
      open.emplace_back(op.c, true);
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
class Access {
public:
  using Value = Entry;
  using Table = typename TypedTemplate<T>::Table;

  explicit Access(const TypedTemplate<T> &t) : t_(&t) {}

  mustache::Kind kind_of(const Entry e) const
  {
    return Table::with_type(e, [this]<class V>(const V &v) -> mustache::Kind {
      if constexpr (variant_like<V>) return std::visit([this](const auto &x) { return kind_of(Table::entry(x)); }, variant_base(v));
      else if constexpr (std::same_as<V, bool>) return v ? mustache::Kind::truthy : mustache::Kind::falsy;
      else if constexpr (number_like<V> || text_like<V>) return mustache::Kind::text;
      else if constexpr (falsy_like<V>) return mustache::Kind::falsy;
      else if constexpr (is_optional_v<V>) return v ? kind_of(Table::entry(*v)) : mustache::Kind::falsy;
      else if constexpr (map_like<V>) return v.empty() ? mustache::Kind::falsy : mustache::Kind::map;
      else if constexpr (list_like<V>) return mustache::Kind::list;
      else return mustache::Kind::map;
    });
  }

  std::string_view text_of(const Entry e, const containers::NumberText text) const
  {
    return Table::with_type(e, [this, text]<class V>(const V &v) -> std::string_view {
      if constexpr (variant_like<V>) return std::visit([this, text](const auto &x) { return text_of(Table::entry(x), text); }, variant_base(v));
      else if constexpr (text_like<V>) return std::string_view(v);
      else if constexpr (number_like<V>) {
        const std::to_chars_result r = std::to_chars(text.data(), std::to_address(text.end()), v);
        return {text.data(), r.ptr};
      }
      else if constexpr (is_optional_v<V>) return v ? text_of(Table::entry(*v), text) : std::string_view();
      else return {};
    });
  }

  size_t size_of(const Entry e) const
  {
    return Table::with_type(e, [this]<class V>(const V &v) -> size_t {
      if constexpr (variant_like<V>) return std::visit([this](const auto &x) { return size_of(Table::entry(x)); }, variant_base(v));
      else if constexpr (is_optional_v<V>) return v ? size_of(Table::entry(*v)) : 0;
      else if constexpr (list_like<V>) return (size_t)std::ranges::size(v);
      else return 0;
    });
  }

  Entry element(const Entry e, const size_t i) const
  {
    return Table::with_type(e, [this, i, e]<class V>(const V &v) -> Entry {
      if constexpr (variant_like<V>) return std::visit([this, i](const auto &x) { return element(Table::entry(x), i); }, variant_base(v));
      else if constexpr (is_optional_v<V>) return element(Table::entry(*v), i);
      else if constexpr (list_like<V>) return Table::entry(*std::ranges::next(std::ranges::begin(v), (std::ptrdiff_t)i));
      else return e;
    });
  }

  std::optional<Entry> find(const Entry e, const std::string &key) const
  {
    return Table::with_type(e, [this, &key]<class V>(const V &v) -> std::optional<Entry> {
      if constexpr (variant_like<V>) return std::visit([this, &key](const auto &x) { return find(Table::entry(x), key); }, variant_base(v));
      else if constexpr (is_optional_v<V>) {
        if (!v) return std::nullopt;
        return find(Table::entry(*v), key);
      }
      else if constexpr (map_like<V>) {
        const auto it = v.find(key);
        if (it == v.end()) return std::nullopt;
        return Table::entry(it->second);
      }
      else if constexpr (struct_like<V>) {
        const std::optional<Step> s = Table::template member_named<V>(key);
        if (!s) return std::nullopt;
        return Entry{Table::member_at(v, s->member), s->type};
      }
      else return std::nullopt;
    });
  }

  template <class Stack>
  std::optional<std::optional<Entry>> bound(const Template &p, const uint32_t pc, const Stack &stack, const int) const
  {
    if (&p != &t_->program) return std::nullopt;
    const Binding &bind = t_->bindings.at(pc);
    if (!bind.fixed) return std::nullopt;
    const void *at = stack.at(bind.level).p;
    for (uint32_t i = 0; i < bind.count; i++) at = Table::followed(t_->steps.at(bind.first + i), at);
    return std::optional<Entry>(Entry{at, bind.type});
  }

private:
  const TypedTemplate<T> *t_;
};

}

template <class T>
std::expected<std::string, Error>
render(const TypedTemplate<T> &t, const T &data, const Partials &partials = {}, const size_t initial_capacity = kInitialCapacity)
{
  using Table = typename TypedTemplate<T>::Table;
  return Walk<detail::typed::Access<T>>(detail::typed::Access<T>(t), &partials, kMaxRenderScore, initial_capacity)
      .run(t.program, Table::entry(data));
}

}
