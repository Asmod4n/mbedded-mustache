#pragma once

#ifdef __cpp_impl_reflection

#include <meta>
#include <array>
#include <charconv>
#include <memory>
#include <optional>
#include <tuple>
#include <ranges>
#include <string>
#include <string_view>
#include <variant>
#include <type_traits>

#include "compile.hpp"
#include "out.hpp"
#include "render.hpp"
#include "media_type.hpp"

namespace mustache {

template <size_t N>
struct fixed_string {
  char s[N]{};
  consteval fixed_string() = default;
  consteval fixed_string(const char (&a)[N]) { for (size_t i = 0; i < N; i++) s[i] = a[i]; }
  constexpr std::string_view view() const { return {s, N - 1}; }
};

template <size_t N>
consteval fixed_string<N + 1>
fixed_of(const std::string_view text)
{
  fixed_string<N + 1> f;
  for (size_t i = 0; i < N; i++) f.s[i] = text.at(i);
  return f;
}

struct KeyRef {
  uint32_t off;
  uint32_t len;
};

struct Sizes {
  size_t ops;
  size_t texts;
  size_t keys;
  size_t key_refs;
  bool   refused;
};

consteval std::variant<Compiled, Refusal>
compiled_for(const std::string_view src, const std::string_view media_type)
{
  if (media_type.empty()) return detail::compile(src);
  return detail::compile(src, media_type);
}

consteval Sizes
sizes_of(const std::string_view src, const std::string_view media_type)
{
  const std::variant<Compiled, Refusal> r = compiled_for(src, media_type);
  if (std::holds_alternative<Refusal>(r)) [[unlikely]] return {0, 0, 0, 0, true};
  const Compiled &c = std::get<Compiled>(r);
  size_t keys = 0;
  for (const std::string &k : c.keys) keys += k.size();
  return {c.ops.size(), c.texts.size(), keys, c.keys.size(), false};
}

template <Sizes S>
struct StaticProgram {
  std::array<Op, S.ops>          ops{};
  std::array<char, S.texts + 1>  texts{};
  std::array<char, S.keys + 1>   keys{};
  std::array<KeyRef, S.key_refs> key_refs{};
  bool                           refused = S.refused;
};

template <Sizes S>
consteval StaticProgram<S>
make_static(const std::string_view src, const std::string_view media_type)
{
  StaticProgram<S> p;
  const std::variant<Compiled, Refusal> r = compiled_for(src, media_type);
  if (std::holds_alternative<Refusal>(r)) [[unlikely]] return p;
  const Compiled &c = std::get<Compiled>(r);
  for (size_t i = 0; i < S.ops; i++) p.ops.at(i) = c.ops.at(i);
  for (size_t i = 0; i < S.texts; i++) p.texts.at(i) = c.texts.at(i);
  size_t at = 0;
  for (size_t i = 0; i < S.key_refs; i++) {
    p.key_refs.at(i) = {(uint32_t)at, (uint32_t)c.keys.at(i).size()};
    for (const char ch : c.keys.at(i)) p.keys.at(at++) = ch;
  }
  return p;
}

template <fixed_string Src, fixed_string Media>
consteval Sizes
sizes_from_copy()
{
  const fixed_string copy = Src;
  const fixed_string media = Media;
  return sizes_of(copy.view(), media.view());
}

template <fixed_string Src, fixed_string Media>
consteval auto
static_program_from_copy()
{
  const fixed_string copy = Src;
  const fixed_string media = Media;
  return make_static<sizes_from_copy<Src, Media>()>(copy.view(), media.view());
}

template <fixed_string Src, fixed_string Media = fixed_string<1>{}>
inline constexpr auto static_program_of = static_program_from_copy<Src, Media>();

namespace detail {

consteval std::meta::info
member_named(std::meta::info type, std::string_view name)
{
  if (!std::meta::is_class_type(type)) [[unlikely]] return std::meta::info{};
  for (std::meta::info m : std::meta::nonstatic_data_members_of(type, std::meta::access_context::unprivileged())) {
    if (std::meta::has_identifier(m) && std::meta::identifier_of(m) == name) return m;
  }
  return std::meta::info{};
}

inline constexpr auto kCxxKeywords = std::to_array<std::string_view>({
    "alignas", "alignof", "and", "and_eq", "asm", "auto", "bitand", "bitor", "bool", "break", "case", "catch",
    "char", "char8_t", "char16_t", "char32_t", "class", "compl", "concept", "const", "consteval", "constexpr",
    "constinit", "const_cast", "continue", "contract_assert", "co_await", "co_return", "co_yield", "decltype",
    "default", "delete", "do", "double", "dynamic_cast", "else", "enum", "explicit", "export", "extern", "false",
    "float", "for", "friend", "goto", "if", "inline", "int", "long", "mutable", "namespace", "new", "noexcept",
    "not", "not_eq", "nullptr", "operator", "or", "or_eq", "private", "protected", "public", "register",
    "reinterpret_cast", "requires", "return", "short", "signed", "sizeof", "static", "static_assert",
    "static_cast", "struct", "switch", "template", "this", "thread_local", "throw", "true", "try", "typedef",
    "typeid", "typename", "union", "unsigned", "using", "virtual", "void", "volatile", "wchar_t", "while", "xor",
    "xor_eq", "atomic_cancel", "atomic_commit", "atomic_noexcept", "reflexpr", "synchronized", "transaction_safe"});

consteval bool
is_cxx_keyword(const std::string_view name)
{
  for (const std::string_view k : kCxxKeywords) {
    if (k == name) [[unlikely]] return true;
  }
  return false;
}

template <class T>
concept text_like = std::is_convertible_v<const T &, std::string_view>;

template <class T>
concept number_like = std::is_arithmetic_v<T> && !std::is_same_v<T, bool> && !std::is_same_v<T, char>;

template <class T>
concept map_like = requires(const T &m) {
  typename T::key_type;
  typename T::mapped_type;
  { m.end() } -> std::same_as<typename T::const_iterator>;
  { m.empty() } -> std::convertible_to<bool>;
} && std::is_convertible_v<const typename T::key_type &, std::string_view>;

template <class T>
concept list_like = std::ranges::range<T> && !text_like<T> && !map_like<T>;

struct missing {};

template <class M>
constexpr auto
found_in(const M &m, const std::string_view key)
{
  if constexpr (requires { m.find(key); }) return m.find(key);
  else return m.find(typename M::key_type(key));
}

template <class T>
struct is_optional : std::false_type {};
template <class T>
struct is_optional<std::optional<T>> : std::true_type {};

template <int Depth>
consteval void nests_too_deep() = delete("partials, parents or blocks nest deeper than 64");

template <fixed_string Source>
consteval void does_not_compile() = delete("a template does not compile");

template <fixed_string Key>
consteval void names_a_cxx_keyword() = delete("a key is a C++ keyword, which no member can be named");

template <fixed_string Key>
consteval void names_no_member() = delete("a key names no member of any context");

template <class Value>
consteval void is_not_text() = delete("a value is not text");

}

template <fixed_string Name, fixed_string Source>
struct static_partial {
  static constexpr auto name = Name;
  static constexpr auto source = Source;
};

template <class... Partials>
struct static_partial_list {};

struct no_frame {};

template <fixed_string Source, uint32_t From, uint32_t Stop, class Outer>
struct args_frame {
  static constexpr auto     source = Source;
  static constexpr uint32_t from = From;
  static constexpr uint32_t stop = Stop;
  using outer = Outer;
};

struct no_indent {};

template <fixed_string Text, class Outer>
struct indent_frame {};

namespace detail {

template <class Indent>
struct indent_text {
  static constexpr std::string_view view{};
};

template <fixed_string Text, class Outer>
struct indent_text<indent_frame<Text, Outer>> {
  static constexpr std::array<char, indent_text<Outer>::view.size() + Text.view().size()> storage = [] {
    std::array<char, indent_text<Outer>::view.size() + Text.view().size()> all{};
    size_t i = 0;
    for (const char c : indent_text<Outer>::view) all.at(i++) = c;
    for (const char c : Text.view()) all.at(i++) = c;
    return all;
  }();
  static constexpr std::string_view view{storage.data(), storage.size()};
};

template <class Args, size_t K>
struct frame_at {
  using type = typename frame_at<typename Args::outer, K - 1>::type;
};

template <class Args>
struct frame_at<Args, 0> {
  using type = Args;
};

template <class List>
struct partials_of;

template <class... Partials>
struct partials_of<static_partial_list<Partials...>> {
  static consteval int index_of(const std::string_view name)
  {
    int found = -1;
    int i = 0;
    ((found = (found < 0 && Partials::name.view() == name) ? i : found, i++), ...);
    return found;
  }

  template <int I>
  using at = std::tuple_element_t<I, std::tuple<Partials...>>;
};

struct Argument {
  int      depth;
  uint32_t at;
};

}

inline constexpr int kMaxStaticDepth = 64;

template <fixed_string Src, class Partials = static_partial_list<>, class Args = no_frame, class Indent = no_indent,
          int Depth = 0, fixed_string Media = fixed_string<1>{}>
struct Renderer {
  static constexpr const auto &P = static_program_of<Src, Media>;
  static constexpr bool kIndented = !std::is_same_v<Indent, no_indent>;

  static consteval std::string_view key(uint32_t i)
  {
    return {P.keys.data() + P.key_refs.at(i).off, P.key_refs.at(i).len};
  }

  static consteval std::string_view text_at(const uint32_t at, const uint32_t length)
  {
    return {P.texts.data() + at, length};
  }

  template <class... Ctx>
  static consteval bool resolvable(std::string_view name)
  {
    constexpr std::array<std::meta::info, sizeof...(Ctx)> types = {^^std::remove_cvref_t<Ctx>...};
    constexpr std::array<bool, sizeof...(Ctx)> maps = {detail::map_like<std::remove_cvref_t<Ctx>>...};
    for (size_t i = 0; i < types.size(); i++) {
      if (maps.at(i) || detail::member_named(types.at(i), name) != std::meta::info{}) return true;
    }
    return false;
  }

  template <uint32_t first, uint32_t count, uint32_t j, bool FromMap, class T, class F>
  static constexpr void follow(const T &v, const F &with)
  {
    if constexpr (j == count) {
      with(v, std::bool_constant<FromMap>{});
    }
    else if constexpr (detail::map_like<T>) {
      const auto it = detail::found_in(v, key(first + j));
      if (it == v.end()) with(detail::missing{}, std::true_type{});
      else follow<first, count, j + 1, true>(it->second, with);
    }
    else if constexpr (detail::is_cxx_keyword(key(first + j))) {
      detail::names_a_cxx_keyword<fixed_of<key(first + j).size()>(key(first + j))>();
    }
    else if constexpr (detail::member_named(^^T, key(first + j)) == std::meta::info{}) {
      detail::names_no_member<fixed_of<key(first + j).size()>(key(first + j))>();
    }
    else {
      constexpr std::meta::info m = detail::member_named(^^T, key(first + j));
      follow<first, count, j + 1, FromMap>(v.[:m:], with);
    }
  }

  template <uint32_t first, uint32_t count, int i, class F, class... Ctx>
  static constexpr void search(const F &with, const Ctx &...ctx)
  {
    if constexpr (i < 0) {
      with(detail::missing{}, std::true_type{});
    }
    else {
      const auto &c = std::get<i>(std::tie(ctx...));
      using C = std::remove_cvref_t<decltype(c)>;
      if constexpr (detail::map_like<C>) {
        const auto it = detail::found_in(c, key(first));
        if (it != c.end()) follow<first, count, 1, true>(it->second, with);
        else search<first, count, i - 1>(with, ctx...);
      }
      else if constexpr (detail::member_named(^^C, key(first)) != std::meta::info{}) {
        constexpr std::meta::info m = detail::member_named(^^C, key(first));
        follow<first, count, 1, false>(c.[:m:], with);
      }
      else {
        search<first, count, i - 1>(with, ctx...);
      }
    }
  }

  template <uint32_t first, uint32_t count, class F, class... Ctx>
  static constexpr void lookup(const F &with, const Ctx &...ctx)
  {
    if constexpr (count == 0) {
      with(std::get<sizeof...(Ctx) - 1>(std::tie(ctx...)), std::false_type{});
    }
    else if constexpr (detail::is_cxx_keyword(key(first))) {
      detail::names_a_cxx_keyword<fixed_of<key(first).size()>(key(first))>();
    }
    else if constexpr (!resolvable<Ctx...>(key(first))) {
      detail::names_no_member<fixed_of<key(first).size()>(key(first))>();
    }
    else {
      search<first, count, (int)sizeof...(Ctx) - 1>(with, ctx...);
    }
  }

  template <class A>
  static consteval detail::Argument argument_of(const std::string_view name)
  {
    if constexpr (std::is_same_v<A, no_frame>) {
      return {-1, 0};
    }
    else {
      const detail::Argument outer = argument_of<typename A::outer>(name);
      if (outer.depth >= 0) return {outer.depth + 1, outer.at};
      constexpr const auto &F = static_program_of<A::source>;
      detail::Argument found{-1, 0};
      for (uint32_t q = A::from; q < A::stop; q = F.ops.at(q).c) {
        if (std::string_view(F.texts.data() + F.ops.at(q).a, F.ops.at(q).b) == name) found = {0, q};
      }
      return found;
    }
  }

  static consteval size_t batch_bound(const uint32_t from, const uint32_t stop)
  {
    size_t bound = 0;
    for (uint32_t q = from; q < stop; q++) {
      const Op op = P.ops.at(q);
      if (op.tag == Tag::text) bound += op.b;
      else if (op.tag == Tag::var || op.tag == Tag::raw) bound += 32;
      else return 0;
    }
    return bound;
  }

  template <class W>
  [[gnu::always_inline]] static constexpr void indent_if_pending(W &out, bool &pending)
  {
    if constexpr (kIndented) {
      if (pending) {
        pending = false;
        out.raw(detail::indent_text<Indent>::view);
      }
    }
  }

  template <class W>
  [[gnu::always_inline]] static constexpr void text(W &out, bool &pending, const std::string_view all)
  {
    if constexpr (!kIndented) {
      if constexpr (requires { out.text(all); }) out.text(all);
      else out.raw(all);
    }
    else {
      std::string_view s = all;
      while (!s.empty()) {
        const size_t nl = s.find('\n');
        const size_t line = nl == std::string_view::npos ? s.size() : nl + 1;
        indent_if_pending(out, pending);
        out.raw(s.substr(0, line));
        if (nl != std::string_view::npos) pending = true;
        s.remove_prefix(line);
      }
    }
  }

  template <Tag tag, Context context, class W>
  [[gnu::always_inline]] static constexpr void value(W &out, bool &pending, const std::string_view s)
  {
    if (s.empty()) return;
    indent_if_pending(out, pending);
    if constexpr (tag != Tag::var) out.raw(s);
    else if constexpr (context == Context::html_text) out.escaped(s);
    else out.escaped_in(context, s);
  }

  template <fixed_string Source, class InnerArgs, uint32_t IndentLength, uint32_t IndentAt, uint32_t From, uint32_t Stop,
            class W, class... Ctx>
  static constexpr void run_nested(W &out, bool &pending, const Ctx &...ctx)
  {
    if constexpr (Depth + 1 > kMaxStaticDepth) {
      detail::nests_too_deep<Depth + 1>();
    }
    else if constexpr (static_program_of<Source, Media>.refused) {
      detail::does_not_compile<Source>();
    }
    else if constexpr (IndentLength == 0) {
      indent_if_pending(out, pending);
      Renderer<Source, Partials, InnerArgs, Indent, Depth + 1, Media>::template run<From, Stop>(out, pending, ctx...);
    }
    else {
      using Inner = indent_frame<fixed_of<IndentLength>(text_at(IndentAt, IndentLength)), Indent>;
      bool inner = true;
      Renderer<Source, Partials, InnerArgs, Inner, Depth + 1, Media>::template run<From, Stop>(out, inner, ctx...);
      if constexpr (kIndented) pending = inner;
    }
  }

  template <class W>
  static constexpr void found_no_text(W &out)
  {
    if constexpr (requires { out.fault; }) {
      if (out.fault == Fault::none) out.fault = Fault::not_text;
    }
    else if constexpr (requires { out.dirty; }) {
      out.dirty = true;
    }
    else {
      throw "a value is not text";
    }
  }

  template <Op op, bool FromMap, class W, class V>
  static constexpr void interpolate(W &out, bool &pending, const V &v)
  {
    if constexpr (std::is_same_v<V, detail::missing>) {
    }
    else if constexpr (FromMap && !detail::text_like<V> && !detail::number_like<V> && !detail::is_optional<V>::value) {
      found_no_text(out);
    }
    else if constexpr (detail::is_optional<V>::value) {
      if constexpr (!detail::text_like<typename V::value_type>) detail::is_not_text<V>();
      else if (v) value<op.tag, (Context)op.d>(out, pending, std::string_view(*v));
    }
    else if constexpr (detail::number_like<V>) {
      std::array<char, 32> digits{};
      const std::to_chars_result r = std::to_chars(digits.data(), std::to_address(digits.end()), v);
      value<op.tag, (Context)op.d>(out, pending, std::string_view(digits.data(), r.ptr));
    }
    else if constexpr (!detail::text_like<V>) {
      detail::is_not_text<V>();
    }
    else if constexpr (op.tag == Tag::var && std::is_same_v<V, std::string> && (Context)op.d == Context::html_text &&
                       requires { out.escaped_string(v); }) {
      if (!v.empty()) {
        indent_if_pending(out, pending);
        out.escaped_string(v);
      }
    }
    else {
      value<op.tag, (Context)op.d>(out, pending, std::string_view(v));
    }
  }

  template <class V>
  static constexpr bool is_empty(const V &v)
  {
    if constexpr (std::is_same_v<V, detail::missing>) return true;
    else if constexpr (std::is_same_v<V, bool>) return !v;
    else if constexpr (detail::is_optional<V>::value) return !v;
    else if constexpr (detail::map_like<V> || detail::list_like<V>) return std::ranges::empty(v);
    else return false;
  }

  template <uint32_t pc, Op op, class W, class V, class... Ctx>
  static constexpr void section(W &out, bool &pending, const V &v, const Ctx &...ctx)
  {
    if constexpr (std::is_same_v<V, detail::missing>) {
    }
    else if constexpr (std::is_same_v<V, bool>) {
      if (v) run<pc + 1, op.c>(out, pending, ctx...);
    }
    else if constexpr (detail::is_optional<V>::value) {
      if (v) run<pc + 1, op.c>(out, pending, ctx..., *v);
    }
    else if constexpr (detail::map_like<V>) {
      if (!v.empty()) run<pc + 1, op.c>(out, pending, ctx..., v);
    }
    else if constexpr (detail::list_like<V>) {
      constexpr size_t bound = batch_bound(pc + 1, op.c);
      if constexpr (bound != 0 && !kIndented && requires { out.reserved(bound); }) {
        for (const auto &e : v) {
          if (!out.reserved(bound)) [[unlikely]] return;
          char *const start = out.rest().data();
          Batch b{start};
          run<pc + 1, op.c>(b, pending, ctx..., e);
          if (!b.dirty) [[likely]]
            out.size += (size_t)std::distance(start, b.w);
          else
            run<pc + 1, op.c>(out, pending, ctx..., e);
        }
      }
      else {
        for (const auto &e : v) run<pc + 1, op.c>(out, pending, ctx..., e);
      }
    }
    else {
      run<pc + 1, op.c>(out, pending, ctx..., v);
    }
  }

  template <uint32_t pc, uint32_t stop, class W, class... Ctx>
  static constexpr void run(W &out, bool &pending, const Ctx &...ctx)
  {
    if constexpr (pc < stop) {
      if constexpr (requires { out.fault; }) {
        if (out.fault != Fault::none) [[unlikely]] return;
      }
      constexpr Op op = P.ops.at(pc);
      if constexpr (op.tag == Tag::text) {
        text(out, pending, text_at(op.a, op.b));
        run<pc + 1, stop>(out, pending, ctx...);
      }
      else if constexpr (op.tag == Tag::var || op.tag == Tag::raw) {
        lookup<op.a, op.b>([&](const auto &v, auto from_map) { interpolate<op, from_map()>(out, pending, v); }, ctx...);
        run<pc + 1, stop>(out, pending, ctx...);
      }
      else if constexpr (op.tag == Tag::section) {
        lookup<op.a, op.b>([&](const auto &v, auto) { section<pc, op>(out, pending, v, ctx...); }, ctx...);
        run<op.c, stop>(out, pending, ctx...);
      }
      else if constexpr (op.tag == Tag::inverted) {
        bool empty = false;
        lookup<op.a, op.b>([&](const auto &v, auto) { empty = is_empty(v); }, ctx...);
        if (empty) run<pc + 1, op.c>(out, pending, ctx...);
        run<op.c, stop>(out, pending, ctx...);
      }
      else if constexpr (op.tag == Tag::partial) {
        constexpr int i = detail::partials_of<Partials>::index_of(key(op.a));
        if constexpr (i >= 0) {
          using Found = typename detail::partials_of<Partials>::template at<i>;
          run_nested<Found::source, no_frame, op.b, op.c, 0, (uint32_t)static_program_of<Found::source, Media>.ops.size()>(
              out, pending, ctx...);
        }
        run<pc + 1, stop>(out, pending, ctx...);
      }
      else if constexpr (op.tag == Tag::parent) {
        constexpr int i = detail::partials_of<Partials>::index_of(key(op.a));
        if constexpr (i >= 0) {
          using Found = typename detail::partials_of<Partials>::template at<i>;
          run_nested<Found::source, args_frame<Src, pc + 1, op.c, Args>, op.b, op.d, 0,
                     (uint32_t)static_program_of<Found::source, Media>.ops.size()>(out, pending, ctx...);
        }
        run<op.c, stop>(out, pending, ctx...);
      }
      else if constexpr (op.tag == Tag::block) {
        constexpr detail::Argument found = argument_of<Args>(text_at(op.a, op.b));
        if constexpr (found.depth >= 0) {
          using Frame = typename detail::frame_at<Args, (size_t)found.depth>::type;
          run_nested<Frame::source, Args, op.d, op.e, found.at + 1,
                     static_program_of<Frame::source, Media>.ops.at(found.at).c>(out, pending, ctx...);
        }
        else {
          run_nested<Src, Args, op.d, op.e, pc + 1, op.c>(out, pending, ctx...);
        }
        run<op.c, stop>(out, pending, ctx...);
      }
    }
  }
};

namespace detail {

inline constexpr fixed_string kHtml = "text/html";
inline constexpr fixed_string kXml = "application/xml";
inline constexpr fixed_string kJson = "application/json";
inline constexpr fixed_string kJs = "text/javascript";
inline constexpr fixed_string kCss = "text/css";

template <fixed_string Src, fixed_string Media, class Partials, class T>
std::expected<std::string, Error>
rendered_as(const T &data, const size_t initial_capacity)
{
  if constexpr (static_program_of<Src, Media>.refused) {
    return std::unexpected(Error{Fault::parse, {}, 0, 0});
  }
  else {
    Answer out;
    if (!out.opened(initial_capacity)) [[unlikely]] return std::unexpected(out.error());
    bool pending = false;
    Renderer<Src, Partials, no_frame, no_indent, 0, Media>::template run<
        0, (uint32_t)static_program_of<Src, Media>.ops.size()>(out, pending, data);
    if (out.fault != Fault::none) [[unlikely]] return std::unexpected(out.error());
    return out.closed();
  }
}

}

template <fixed_string Src, class... Partials, class T>
std::expected<std::string, Error>
render(const T &data, const std::string_view media_type, const size_t initial_capacity = kInitialCapacity)
{
  using List = static_partial_list<Partials...>;
  const std::optional<Grammar> g = grammar_of(media_type);
  if (!g) [[unlikely]] return std::unexpected(Error{Fault::parse, {}, 0, 0});
  switch (*g) {
    case Grammar::html: return detail::rendered_as<Src, detail::kHtml, List>(data, initial_capacity);
    case Grammar::xml:  return detail::rendered_as<Src, detail::kXml, List>(data, initial_capacity);
    case Grammar::json: return detail::rendered_as<Src, detail::kJson, List>(data, initial_capacity);
    case Grammar::js:   return detail::rendered_as<Src, detail::kJs, List>(data, initial_capacity);
    case Grammar::css:  return detail::rendered_as<Src, detail::kCss, List>(data, initial_capacity);
    default:            return std::unexpected(Error{Fault::parse, {}, 0, 0});
  }
}

template <fixed_string Src, class... Partials, class T>
std::expected<std::string, Error>
render(const T &data, const size_t initial_capacity = kInitialCapacity)
{
  if constexpr (static_program_of<Src>.refused) detail::does_not_compile<Src>();
  Answer out;
  if (!out.opened(initial_capacity)) [[unlikely]] return std::unexpected(out.error());
  bool pending = false;
  Renderer<Src, static_partial_list<Partials...>>::template run<0, (uint32_t)static_program_of<Src>.ops.size()>(
      out, pending, data);
  if (out.fault != Fault::none) [[unlikely]] return std::unexpected(out.error());
  return out.closed();
}

}

namespace mustache {

template <fixed_string Src, auto Make, class... Partials>
consteval size_t
static_length()
{
  if constexpr (static_program_of<Src>.refused) detail::does_not_compile<Src>();
  for (size_t capacity = 4096;; capacity *= 2) {
    char *buffer = new char[capacity + kSlack];
    Out out{buffer, buffer + capacity};
    bool pending = false;
    Renderer<Src, static_partial_list<Partials...>>::template run<0, (uint32_t)static_program_of<Src>.ops.size()>(out, pending,
                                                                                                             Make());
    const size_t n = (size_t)(out.w - buffer);
    const bool full = out.full;
    delete[] buffer;
    if (!full) return n;
  }
}

template <fixed_string Src, auto Make, class... Partials>
consteval auto
static_page()
{
  constexpr size_t n = static_length<Src, Make, Partials...>();
  std::array<char, n + kSlack> page{};
  Out out{page.data(), page.data() + n};
  bool pending = false;
  Renderer<Src, static_partial_list<Partials...>>::template run<0, (uint32_t)static_program_of<Src>.ops.size()>(out, pending,
                                                                                                           Make());
  return page;
}

template <fixed_string Src, auto Make, class... Partials>
inline constexpr auto static_page_of = static_page<Src, Make, Partials...>();

}

#endif
