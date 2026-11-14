// The html escape over a proven std::span<char> (task c08c8baf, plan A4, commit 6), with the text that enters
// include/mustache/escape.hpp. It is included after mustache/escape.hpp of 738ff29 and adds overloads beside
// the forms that take char *. Arm 2: the two 32-byte helpers carry [[gnu::always_inline, gnu::flatten]], as the
// 16-byte helpers of 738ff29 do, and escape_into carries no attribute. Arm 3: the two 32-byte helpers carry no
// attribute, and escape_into carries [[gnu::flatten]]. Arm 4: arm 3, and the scalar parts store each word at the
// start of a std::span that each word advances (std::span::subspan), so that the store takes a base address and no
// index; the count is std::ranges::distance from the start of the room.
#pragma once

#include <mustache/escape.hpp>

namespace mustache {

#if ARM == 4
[[gnu::always_inline, gnu::flatten]] constexpr size_t
advanced_escaped_into(const std::span<char> room, const size_t count, const std::string_view s, const Entities &table)
{
  std::span<char> rest = room.subspan(count);
  for (const char c : s) {
    const uint64_t word = entity_word_of(table, static_cast<unsigned char>(c));
    stored_word(rest.first<kEntityWordBytes>(), word);
    rest = rest.subspan(entity_length(word));
  }
  return static_cast<size_t>(std::ranges::distance(room.begin(), rest.begin()));
}
#define SCALAR_ESCAPED_INTO advanced_escaped_into
#else
#define SCALAR_ESCAPED_INTO table_escaped_into
#endif

constexpr size_t
escape_bytes(const std::span<char> room, const std::string_view s)
{
  return SCALAR_ESCAPED_INTO(room, 0, s, kEntities);
}

#if ARM == 2
[[gnu::always_inline, gnu::flatten]]
#endif
inline __m256i
loaded(const std::span<const char, sizeof(__m256i)> b)
{
  std::array<char, sizeof(__m256i)> a;
  std::copy_n(b.begin(), b.size(), a.begin());
  return std::bit_cast<__m256i>(a);
}

#if ARM == 2
[[gnu::always_inline, gnu::flatten]]
#endif
inline void
stored(const std::span<char, sizeof(__m256i)> o, const __m256i v)
{
  const auto a = std::bit_cast<std::array<char, sizeof(__m256i)>>(v);
  std::copy_n(a.begin(), a.size(), o.begin());
}

#if ARM == 3 || ARM == 4
[[gnu::flatten]]
#endif
inline size_t
escape_into(const std::span<char> room, const std::string_view s)
{
  if (s.size() < kVectorBytes) return escape_bytes(room, s);
  const std::span<const char> in(s);
  std::array<char, 2 * kVectorBytes> block;
  size_t w = 0;
  size_t at = 0;
  for (; at + kVectorBytes <= in.size(); at += kVectorBytes) {
    const __m256i b = loaded(in.subspan(at).first<kVectorBytes>());
    uint32_t mask = marks_of(b);
    stored(room.subspan(w).first<kVectorBytes>(), b);
    if (mask == 0) {
      w += kVectorBytes;
      continue;
    }
    if (std::cmp_greater(std::popcount(mask), kMarksPerVectorMax)) [[unlikely]] {
      w = SCALAR_ESCAPED_INTO(room, w, s.substr(at, kVectorBytes), kEntities);
      continue;
    }
    std::span<const char> from = in.subspan(at);
    if (at + 2 * kVectorBytes > in.size()) {
      stored(std::span(block).first<kVectorBytes>(), b);
      stored(std::span(block).last<kVectorBytes>(), _mm256_setzero_si256());
      from = block;
    }
    size_t done = 0;
    while (mask != 0) {
      const size_t i = static_cast<size_t>(std::countr_zero(mask));
      w = word_appended(room, w + (i - done), entity_word_of(kEntities, static_cast<unsigned char>(*std::ranges::next(from.begin(), i))));
      done = i + 1;
      stored(room.subspan(w).first<kVectorBytes>(), loaded(from.subspan(done).first<kVectorBytes>()));
      mask &= mask - 1;
    }
    w += kVectorBytes - done;
  }
  return SCALAR_ESCAPED_INTO(room, w, s.substr(at), kEntities);
}

}
