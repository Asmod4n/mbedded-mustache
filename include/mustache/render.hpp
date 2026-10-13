#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <functional>
#include <limits>
#include <memory>
#include <new>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "compile.hpp"
#include "containers.hpp"
#include "media_type.hpp"
#include "out.hpp"

namespace mustache {

enum class Fault : uint8_t { none, over_limit, not_text, too_deep, over_work, parse, no_memory };

struct Error {
  Fault            fault;
  std::string_view what;
  size_t           asked;
  size_t           allowed;
};

using Template = Compiled;

struct StringHash {
  using is_transparent = void;
  size_t operator()(const std::string_view s) const { return std::hash<std::string_view>{}(s); }
};

using Partials = std::unordered_map<std::string, Template, StringHash, std::equal_to<>>;

inline constexpr size_t kInitialCapacity = 1024;

inline constexpr size_t kMaxCapacity = (size_t)std::numeric_limits<std::ptrdiff_t>::max() - kEscapeSlack;

struct Answer {
  std::string bytes;
  size_t      size = 0;
  size_t      max_capacity = kMaxCapacity;
  Fault       fault = Fault::none;
  size_t      asked = 0;
  size_t      allowed = 0;

  size_t capacity() const { return bytes.size() - kEscapeSlack; }

  Error error() const { return Error{fault, "render output", asked, allowed}; }

  bool resized(const size_t capacity)
  {
    try {
      bytes.resize(capacity + kEscapeSlack);
      return true;
    }
    catch (const std::bad_alloc &) {
    }
    catch (const std::length_error &) {
    }
    fault = Fault::no_memory;
    asked = capacity;
    return false;
  }

  bool opened(const size_t initial_capacity)
  {
    if (initial_capacity > max_capacity) [[unlikely]] {
      fault = Fault::over_limit;
      asked = initial_capacity;
      allowed = max_capacity;
      return false;
    }
    return resized(initial_capacity);
  }

  bool grown(const size_t needed)
  {
    if (fault != Fault::none) [[unlikely]] return false;
    if (needed > max_capacity) [[unlikely]] {
      fault = Fault::over_limit;
      asked = needed;
      allowed = max_capacity;
      return false;
    }
    return resized(std::min(std::max(capacity() * 2, needed), max_capacity));
  }

  std::span<char> rest() { return std::span<char>(bytes).subspan(size); }

  std::string closed()
  {
    bytes.resize(size);
    return std::move(bytes);
  }

  [[gnu::always_inline]] void raw(const std::string_view s)
  {
    if (size + s.size() > capacity() && !grown(size + s.size())) [[unlikely]] return;
    std::ranges::copy(s, rest().begin());
    size += s.size();
  }

  [[gnu::always_inline]] bool reserved(const size_t bound)
  {
    return size + bound <= capacity() || grown(size + bound);
  }

  [[gnu::always_inline]] void escaped_string(const std::string &s)
  {
    if (!escaped_fits(s, capacity() - size) && !grown(size + escaped_size_of(s))) [[unlikely]] return;
    char *const w = rest().data();
    size += (size_t)std::distance(w, escape_string_into(w, s));
  }

  void escaped_in(const Context c, const std::string_view s)
  {
    if (c == Context::html_text) {
      escaped(s);
      return;
    }
    const size_t needed = escaped_size_max_in(c, s.size());
    if (size + needed > capacity() && !grown(size + needed)) [[unlikely]] return;
    char *const w = rest().data();
    size += (size_t)std::distance(w, escaped_into_in(c, w, s));
  }

  [[gnu::always_inline]] void escaped(const std::string_view s)
  {
    if (!escaped_fits(s, capacity() - size) && !grown(size + escaped_size_of(s))) [[unlikely]] return;
    char *const w = rest().data();
    size += (size_t)std::distance(w, escaped_into(w, s));
  }
};

struct Batch {
  char *w;
  bool  dirty = false;

  [[gnu::always_inline]] void text(const std::string_view s)
  {
    std::ranges::copy(s, w);
    w += s.size();
  }

  [[gnu::always_inline]] void raw(const std::string_view s)
  {
    if (s.size() > 32) [[unlikely]] {
      dirty = true;
      return;
    }
    std::ranges::copy(s, w);
    w += s.size();
  }

  [[gnu::always_inline]] void escaped(const std::string_view) { dirty = true; }

  [[gnu::always_inline]] void escaped_in(const Context, const std::string_view) { dirty = true; }


  [[gnu::always_inline]] void escaped_string(const std::string &s)
  {
    const Copied c = copied_string(w, s);
    if (c.w == nullptr) [[unlikely]] {
      dirty = true;
      return;
    }
    dirty |= c.marks != 0;
    w = c.w;
  }
};

template <class Compile>
std::expected<Template, Error>
compiled_by(const std::string_view source, const Compile &compile_source)
{
  if (source.size() > kSourceMax) [[unlikely]] return std::unexpected(Error{Fault::over_limit, "template", source.size(), kSourceMax});
  try {
    std::variant<Compiled, Refusal> compiled = compile_source();
    if (std::holds_alternative<Refusal>(compiled)) [[unlikely]] {
      const Refusal r = std::get<Refusal>(compiled);
      return std::unexpected(Error{Fault::parse, kProblemTitles.at((size_t)r.problem), size_t{r.at}, size_t{0}});
    }
    return std::get<Compiled>(std::move(compiled));
  }
  catch (const std::bad_alloc &) {
  }
  catch (const std::length_error &) {
  }
  return std::unexpected(Error{Fault::no_memory, "template", source.size(), size_t{0}});
}

inline constexpr int kMaxDepth = 32;
inline constexpr int kMaxPartialDepth = 64;
inline constexpr size_t kMaxRenderScore = size_t{1} << 24;

template <int kDepthMax = kMaxDepth, int kPartialDepthMax = kMaxPartialDepth>
class Walk {
public:
  using Value = detail::containers::Value;

  Walk(const Partials *const partials, const size_t score_max = kMaxRenderScore,
       const size_t initial_capacity = kInitialCapacity, const size_t max_capacity = kMaxCapacity)
      : partials_(partials), score_max_(score_max), initial_capacity_(initial_capacity)
  {
    out_.max_capacity = max_capacity;
  }

  std::expected<std::string, Error> run(const Template &p, const Value root)
  {
    stack_.at(0) = root;
    depth_ = 1;
    context_ = p.context;
    if (!out_.opened(initial_capacity_)) [[unlikely]] return std::unexpected(out_.error());
    run(p, 0, (uint32_t)p.ops.size(), 0, nullptr, nullptr);
    if (fault_ != Fault::none) [[unlikely]] return std::unexpected(error_of(fault_));
    return out_.closed();
  }

private:
  Error error_of(const Fault fault) const
  {
    switch (fault) {
      case Fault::not_text: return Error{fault, "a value is not text", 0, 0};
      case Fault::too_deep: return Error{fault, "nesting too deep", 0, 0};
      case Fault::over_work: return Error{fault, "the render does more work than max_render_score", score_, score_max_};
      default: return out_.error();
    }
  }

  struct Indent {
    const Indent    *outer;
    std::string_view text;
    bool             pending;
  };

  struct Args {
    const Template *program;
    uint32_t            arguments;
    const Args         *outer;
  };

  const Partials *partials_;
  Answer out_;
  std::array<Value, kDepthMax> stack_{};
  int   depth_ = 0;
  size_t score_ = 0;
  size_t score_max_;
  size_t initial_capacity_;
  Fault fault_ = Fault::none;
  Context context_ = Context::html_text;
  std::array<char, detail::containers::kNumberTextMax> number_{};

  const Template *partial_named(const std::string &name) const
  {
    if (partials_ == nullptr) return nullptr;
    const auto it = partials_->find(name);
    return it == partials_->end() ? nullptr : &it->second;
  }

  void raw(const std::string_view s)
  {
    out_.raw(s);
    if (out_.fault != Fault::none) [[unlikely]] fault_ = out_.fault;
  }

  void escaped(const std::string_view s)
  {
    out_.escaped(s);
    if (out_.fault != Fault::none) [[unlikely]] fault_ = out_.fault;
  }

  void escaped_in(const Context c, const std::string_view s)
  {
    out_.escaped_in(c, s);
    if (out_.fault != Fault::none) [[unlikely]] fault_ = out_.fault;
  }

  void indent_of(const Indent *const ind)
  {
    if (ind == nullptr) return;
    indent_of(ind->outer);
    raw(ind->text);
  }

  void indent_if_pending(Indent *const ind)
  {
    if (ind != nullptr && ind->pending) [[unlikely]] {
      ind->pending = false;
      indent_of(ind);
    }
  }

  void text(const std::string_view all, Indent *const ind)
  {
    if (ind != nullptr) [[unlikely]] {
      std::string_view s = all;
      while (!s.empty()) {
        const size_t nl = s.find('\n');
        const size_t line = nl == std::string_view::npos ? s.size() : nl + 1;
        indent_if_pending(ind);
        raw(s.substr(0, line));
        if (nl != std::string_view::npos) ind->pending = true;
        s.remove_prefix(line);
      }
      return;
    }
    raw(all);
  }

  std::optional<Value> lookup(const Template &p, const Op &op)
  {
    if (op.b == 0) return stack_.at(depth_ - 1);
    const std::string &first = p.keys.at(op.a);
    std::optional<Value> base;
    for (int i = depth_ - 1; i >= 0; i--) {
      base = stack_.at(i).table->find(stack_.at(i).data, first);
      if (base) break;
    }
    for (uint32_t j = 1; j < op.b && base; j++) base = base->table->find(base->data, p.keys.at(op.a + j));
    return base;
  }

  bool push_run(const Template &p, const Value &v, const uint32_t pc, const uint32_t stop, const int pd,
                Indent *const ind, const Args *const args)
  {
    if (depth_ >= kDepthMax) [[unlikely]] {
      fault_ = Fault::too_deep;
      return false;
    }
    stack_.at(depth_++) = v;
    const bool ok = run(p, pc, stop, pd, ind, args);
    depth_--;
    return ok;
  }

  bool run_indented(const Template &p, const uint32_t from, const uint32_t stop, const int pd, Indent *const ind,
                    const std::string_view indent, const Args *const args)
  {
    if (indent.empty()) {
      indent_if_pending(ind);
      return run(p, from, stop, pd, ind, args);
    }
    Indent inner{ind, indent, true};
    const bool ok = run(p, from, stop, pd, &inner, args);
    if (ind != nullptr) ind->pending = inner.pending;
    return ok;
  }

  static std::string_view block_name_of(const Template &p, const Op &op)
  {
    return std::string_view(p.texts).substr(op.a, op.b);
  }

  struct Argument {
    const Template *program;
    uint32_t            at;
  };

  static std::optional<Argument> argument_of(const Args *const args, const std::string_view name)
  {
    std::optional<Argument> found;
    for (const Args *f = args; f != nullptr; f = f->outer) {
      const std::vector<uint32_t> &all = f->program->arguments;
      const std::span<const uint32_t> sorted = std::span(all).subspan(f->arguments + 1, all.at(f->arguments));
      const auto name_at = [f](const uint32_t q) { return block_name_of(*f->program, f->program->ops.at(q)); };
      const auto after = std::ranges::upper_bound(sorted, name, {}, name_at);
      if (after != sorted.begin() && name_at(*std::prev(after)) == name) found = Argument{f->program, *std::prev(after)};
    }
    return found;
  }

  bool run(const Template &p, const uint32_t from, const uint32_t stop, const int pd, Indent *const ind,
           const Args *const args)
  {
    const size_t work = size_t{stop - from} + 1;
    if (score_max_ - score_ < work) [[unlikely]] {
      score_ += work;
      fault_ = Fault::over_work;
      return false;
    }
    score_ += work;
    uint32_t pc = from;
    while (pc < stop) {
      if (fault_ != Fault::none) [[unlikely]] return false;
      const Op &op = p.ops.at(pc);
      switch (op.tag) {
        case Tag::text:
          text(std::string_view(p.texts).substr(op.a, op.b), ind);
          pc++;
          break;
        case Tag::var:
        case Tag::raw: {
          pc++;
          const std::optional<Value> v = lookup(p, op);
          if (!v) break;
          const Kind k = v->table->kind_of(v->data);
          if (k == Kind::falsy) break;
          if (k != Kind::text) [[unlikely]] {
            fault_ = Fault::not_text;
            return false;
          }
          const std::string_view s = v->table->text_of(v->data, detail::containers::NumberText(number_));
          if (s.empty()) break;
          indent_if_pending(ind);
          if (op.tag == Tag::var) escaped_in(context_, s);
          else raw(s);
          break;
        }
        case Tag::section: {
          const std::optional<Value> v = lookup(p, op);
          const Kind k = v ? v->table->kind_of(v->data) : Kind::falsy;
          if (k == Kind::list) {
            const size_t n = v->table->size_of(v->data);
            for (size_t i = 0; i < n; i++) {
              if (!push_run(p, v->table->element(v->data, i), pc + 1, op.c, pd, ind, args)) [[unlikely]] return false;
            }
          }
          else if (k != Kind::falsy) {
            if (!push_run(p, *v, pc + 1, op.c, pd, ind, args)) [[unlikely]] return false;
          }
          pc = op.c;
          break;
        }
        case Tag::inverted: {
          const std::optional<Value> v = lookup(p, op);
          const Kind k = v ? v->table->kind_of(v->data) : Kind::falsy;
          const bool empty = k == Kind::falsy || (k == Kind::list && v->table->size_of(v->data) == 0);
          if (empty && !push_run(p, stack_.at(depth_ - 1), pc + 1, op.c, pd, ind, args)) [[unlikely]] return false;
          pc = op.c;
          break;
        }
        case Tag::partial: {
          pc++;
          if (pd >= kPartialDepthMax) [[unlikely]] {
            fault_ = Fault::too_deep;
            return false;
          }
          const Template *const sub = partial_named(p.keys.at(op.a));
          if (sub == nullptr) [[unlikely]] break;
          if (!run_indented(*sub, 0, (uint32_t)sub->ops.size(), pd + 1, ind,
                            std::string_view(p.texts).substr(op.c, op.b), nullptr)) [[unlikely]] {
            return false;
          }
          break;
        }
        case Tag::parent: {
          if (pd >= kPartialDepthMax) [[unlikely]] {
            fault_ = Fault::too_deep;
            return false;
          }
          const Template *const sub = partial_named(p.keys.at(op.a));
          if (sub != nullptr) {
            const Args frame{&p, op.e, args};
            if (!run_indented(*sub, 0, (uint32_t)sub->ops.size(), pd + 1, ind,
                              std::string_view(p.texts).substr(op.d, op.b), &frame)) [[unlikely]] {
              return false;
            }
          }
          pc = op.c;
          break;
        }
        case Tag::block: {
          if (pd >= kPartialDepthMax) [[unlikely]] {
            fault_ = Fault::too_deep;
            return false;
          }
          const std::optional<Argument> found = argument_of(args, block_name_of(p, op));
          const Template &body = found ? *found->program : p;
          const uint32_t first = found ? found->at + 1 : pc + 1;
          const uint32_t last = found ? body.ops.at(found->at).c : op.c;
          if (!run_indented(body, first, last, pd + 1, ind, std::string_view(p.texts).substr(op.e, op.d),
                            args)) [[unlikely]] {
            return false;
          }
          pc = op.c;
          break;
        }
      }
    }
    return true;
  }
};

}
