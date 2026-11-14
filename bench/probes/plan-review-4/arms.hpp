// Scratch probe of the plan review, form 4. The arms of the HTML escape:
// 1 and 2: mustache::escape_into of the tree (2 is the same code, built again: the noise floor).
// 50: the final form of bench/probes/escape-form-h/final.cpp (no check, preconditions of span).
// 60: one check for each block of 32 input bytes; the block is std::span<char, kBlockRoom>.
// 61: one check for each store; no store relies on a precondition.
// 51: arm 50 in an unnamed namespace. 63: arm 60 with a plain comparison, no std::optional of a span.
// 62: one check above the loop (kEntityMax * n + slack), then the loop of arm 50.
#include <mustache/escape.hpp>
#include <algorithm>
#include <array>
#include <bit>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>

namespace probe {
#if ARM == 51
namespace {
#endif

inline constexpr size_t kVector = 32;
inline constexpr size_t kWord = 8;
inline constexpr size_t kBlockRoom = kVector * mustache::kEntityMax + kVector;

inline __m256i loaded(const std::span<const char, kVector> b)
{
  std::array<char, kVector> a;
  std::copy_n(b.begin(), b.size(), a.begin());
  return std::bit_cast<__m256i>(a);
}

inline void stored(const std::span<char, kVector> o, const __m256i v)
{
  const std::array<char, kVector> a = std::bit_cast<std::array<char, kVector>>(v);
  std::copy_n(a.begin(), a.size(), o.begin());
}

inline void stored_word(const std::span<char, kWord> o, const uint64_t word)
{
  const std::array<char, kWord> a = std::bit_cast<std::array<char, kWord>>(word);
  std::copy_n(a.begin(), a.size(), o.begin());
}

template <size_t N>
inline std::optional<std::span<char, N>> fixed_at(const std::span<char> s, const size_t at)
{
  if (at > s.size() || s.size() - at < N) [[unlikely]] return std::nullopt;
  return s.subspan(at).template first<N>();
}

inline size_t scalar_h(const std::span<char> out, size_t w, const std::string_view s)
{
  for (const char c : s) {
    const uint64_t word = mustache::kEntities.word[static_cast<unsigned char>(c)];
    stored_word(out.subspan(w).first<kWord>(), word);
    w += mustache::entity_length(word);
  }
  return w;
}

inline size_t escaped_h(const std::span<char> out, const std::string_view sv)
{
  const std::span<const char> in(sv);
  const size_t n = in.size();
  if (n < kVector) return scalar_h(out, 0, sv);
  size_t w = 0;
  size_t at = 0;
  std::array<char, 2 * kVector> block;
  for (; at + kVector <= n; at += kVector) {
    const __m256i b = loaded(in.subspan(at).first<kVector>());
    uint32_t mask = mustache::marks_of(b);
    stored(out.subspan(w).first<kVector>(), b);
    if (mask == 0) {
      w += kVector;
      continue;
    }
    if (std::cmp_greater(std::popcount(mask), 8)) [[unlikely]] {
      w = scalar_h(out, w, sv.substr(at, kVector));
      continue;
    }
    std::span<const char> from = in.subspan(at);
    if (at + 2 * kVector > n) {
      std::ranges::copy(std::bit_cast<std::array<char, kVector>>(b), block.begin());
      std::ranges::fill(std::span(block).subspan(kVector), '\0');
      from = block;
    }
    size_t done = 0;
    while (mask != 0) {
      const size_t i = static_cast<size_t>(std::countr_zero(mask));
      w += i - done;
      const uint64_t word = mustache::kEntities.word[static_cast<unsigned char>(from[i])];
      stored_word(out.subspan(w).first<kWord>(), word);
      w += mustache::entity_length(word);
      done = i + 1;
      stored(out.subspan(w).first<kVector>(), loaded(from.subspan(done).first<kVector>()));
      mask &= mask - 1;
    }
    w += kVector - done;
  }
  return scalar_h(out, w, sv.substr(at));
}

inline std::optional<size_t> scalar_checked(const std::span<char> out, size_t w, const std::string_view s)
{
  for (const char c : s) {
    const uint64_t word = mustache::kEntities.word[static_cast<unsigned char>(c)];
    const auto o = fixed_at<kWord>(out, w);
    if (!o) [[unlikely]] return std::nullopt;
    stored_word(*o, word);
    w += mustache::entity_length(word);
  }
  return w;
}

// Arm 60: one check for each block of 32 input bytes.
inline std::optional<size_t> escaped_per_block(const std::span<char> out, const std::string_view sv)
{
  const std::span<const char> in(sv);
  const size_t n = in.size();
  if (n < kVector) return scalar_checked(out, 0, sv);
  size_t w = 0;
  size_t at = 0;
  std::array<char, 2 * kVector> block;
  for (; at + kVector <= n; at += kVector) {
#if ARM == 63
    if (w > out.size() || out.size() - w < kBlockRoom) [[unlikely]] return std::nullopt;
    const std::span<char, kBlockRoom> r = out.subspan(w).first<kBlockRoom>();
#else
    const auto room = fixed_at<kBlockRoom>(out, w);
    if (!room) [[unlikely]] return std::nullopt;
    const std::span<char, kBlockRoom> r = *room;
#endif
    const __m256i b = loaded(in.subspan(at).first<kVector>());
    uint32_t mask = mustache::marks_of(b);
    stored(r.first<kVector>(), b);
    if (mask == 0) {
      w += kVector;
      continue;
    }
    size_t l = 0;
    if (std::cmp_greater(std::popcount(mask), 8)) [[unlikely]] {
      for (const char c : sv.substr(at, kVector)) {
        const uint64_t word = mustache::kEntities.word[static_cast<unsigned char>(c)];
        stored_word(r.subspan(l).first<kWord>(), word);
        l += mustache::entity_length(word);
      }
      w += l;
      continue;
    }
    std::span<const char> from = in.subspan(at);
    if (at + 2 * kVector > n) {
      std::ranges::copy(std::bit_cast<std::array<char, kVector>>(b), block.begin());
      std::ranges::fill(std::span(block).subspan(kVector), '\0');
      from = block;
    }
    size_t done = 0;
    while (mask != 0) {
      const size_t i = static_cast<size_t>(std::countr_zero(mask));
      l += i - done;
      const uint64_t word = mustache::kEntities.word[static_cast<unsigned char>(from[i])];
      stored_word(r.subspan(l).first<kWord>(), word);
      l += mustache::entity_length(word);
      done = i + 1;
      stored(r.subspan(l).first<kVector>(), loaded(from.subspan(done).first<kVector>()));
      mask &= mask - 1;
    }
    w += l + kVector - done;
  }
  return scalar_checked(out, w, sv.substr(at));
}

// Arm 61: one check for each store.
inline std::optional<size_t> escaped_per_store(const std::span<char> out, const std::string_view sv)
{
  const std::span<const char> in(sv);
  const size_t n = in.size();
  if (n < kVector) return scalar_checked(out, 0, sv);
  size_t w = 0;
  size_t at = 0;
  std::array<char, 2 * kVector> block;
  for (; at + kVector <= n; at += kVector) {
    const __m256i b = loaded(in.subspan(at).first<kVector>());
    uint32_t mask = mustache::marks_of(b);
    const auto first = fixed_at<kVector>(out, w);
    if (!first) [[unlikely]] return std::nullopt;
    stored(*first, b);
    if (mask == 0) {
      w += kVector;
      continue;
    }
    if (std::cmp_greater(std::popcount(mask), 8)) [[unlikely]] {
      const auto r = scalar_checked(out, w, sv.substr(at, kVector));
      if (!r) [[unlikely]] return std::nullopt;
      w = *r;
      continue;
    }
    std::span<const char> from = in.subspan(at);
    if (at + 2 * kVector > n) {
      std::ranges::copy(std::bit_cast<std::array<char, kVector>>(b), block.begin());
      std::ranges::fill(std::span(block).subspan(kVector), '\0');
      from = block;
    }
    size_t done = 0;
    while (mask != 0) {
      const size_t i = static_cast<size_t>(std::countr_zero(mask));
      w += i - done;
      const uint64_t word = mustache::kEntities.word[static_cast<unsigned char>(from[i])];
      const auto ow = fixed_at<kWord>(out, w);
      if (!ow) [[unlikely]] return std::nullopt;
      stored_word(*ow, word);
      w += mustache::entity_length(word);
      done = i + 1;
      const auto ov = fixed_at<kVector>(out, w);
      if (!ov) [[unlikely]] return std::nullopt;
      stored(*ov, loaded(from.subspan(done).first<kVector>()));
      mask &= mask - 1;
    }
    w += kVector - done;
  }
  return scalar_checked(out, w, sv.substr(at));
}

// Arm 62: one check above the loop.
inline std::optional<size_t> escaped_checked_once(const std::span<char> out, const std::string_view sv)
{
  if (out.size() / mustache::kEntityMax < sv.size() || out.size() - sv.size() * mustache::kEntityMax < kVector) [[unlikely]]
    return std::nullopt;
  return escaped_h(out, sv);
}

#if ARM == 51
}
#endif
}
