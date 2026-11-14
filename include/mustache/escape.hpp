#pragma once

#if defined(__AVX2__)
#include <immintrin.h>
#elif defined(__ARM_NEON)
#include <arm_neon.h>
#endif

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace mustache {

struct Entities {
  std::array<uint64_t, 256> word;
};

consteval Entities
entities_of()
{
  Entities e{};
  for (size_t c = 0; c < 256; c++) e.word.at(c) = (uint64_t)c | (uint64_t{1} << 56);
  auto put = [&e](const unsigned char c, const std::string_view s) {
    uint64_t v = (uint64_t)s.size() << 56;
    for (size_t i = 0; i < s.size(); i++) v |= (uint64_t)(unsigned char)s.at(i) << (8 * i);
    e.word.at(c) = v;
  };
  put('&', "&amp;");
  put('<', "&lt;");
  put('>', "&gt;");
  put('"', "&quot;");
  put('\'', "&#39;");
  return e;
}

inline constexpr Entities kEntities = entities_of();

inline constexpr size_t kEntityMax = 6;

inline constexpr size_t kEntityWordBytes = sizeof(uint64_t);

inline constexpr size_t kCopiedStringMax = 32;

inline constexpr size_t kEscapeSlack = kCopiedStringMax;

inline constexpr size_t kBytesPastTheWrittenCount = kEscapeSlack;

constexpr size_t
entity_length(const uint64_t word)
{
  return (size_t)(word >> 56);
}

constexpr bool
is_marked(const char c)
{
  return entity_length(kEntities.word.at((unsigned char)c)) != 1;
}

constexpr char *
entity_into(char *const w, const uint64_t word)
{
  const std::array<char, kEntityWordBytes> bytes = std::bit_cast<std::array<char, kEntityWordBytes>>(word);
  std::copy_n(bytes.begin(), bytes.size(), w);
  return w + entity_length(word);
}

constexpr char *
escape_bytes(char *const out, const std::string_view s)
{
  char *w = out;
  for (const char c : s) w = entity_into(w, kEntities.word.at((unsigned char)c));
  return w;
}

constexpr char *
bytes_into(char *w, const std::string_view s)
{
  return std::ranges::copy(s, w).out;
}

constexpr char *
xml_escaped_into(char *w, const std::string_view s)
{
  for (const char c : s) {
    if (c == '&') w = bytes_into(w, "&amp;");
    else if (c == '<') w = bytes_into(w, "&lt;");
    else if (c == '>') w = bytes_into(w, "&gt;");
    else if (c == '"') w = bytes_into(w, "&quot;");
    else if (c == '\'') w = bytes_into(w, "&apos;");
    else *w++ = c;
  }
  return w;
}

inline constexpr std::string_view kHexDigits = "0123456789abcdef";

constexpr char *
json_string_escaped_into(char *w, const std::string_view s)
{
  for (const char c : s) {
    const unsigned char u = (unsigned char)c;
    if (c == '"') w = bytes_into(w, "\\\"");
    else if (c == '\\') w = bytes_into(w, "\\\\");
    else if (c == '\b') w = bytes_into(w, "\\b");
    else if (c == '\f') w = bytes_into(w, "\\f");
    else if (c == '\n') w = bytes_into(w, "\\n");
    else if (c == '\r') w = bytes_into(w, "\\r");
    else if (c == '\t') w = bytes_into(w, "\\t");
    else if (u < 0x20) {
      w = bytes_into(w, "\\u00");
      *w++ = kHexDigits.at(u >> 4);
      *w++ = kHexDigits.at(u & 15);
    }
    else *w++ = c;
  }
  return w;
}

constexpr bool
is_js_plain(const char c)
{
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == ' ' || c == '_' ||
         c == ',' || c == '.' || (unsigned char)c >= 0x80;
}

constexpr char *
js_string_escaped_into(char *w, const std::string_view s)
{
  for (size_t i = 0; i < s.size(); i++) {
    const unsigned char u = (unsigned char)s.at(i);
    if (u == 0xe2 && i + 2 < s.size() && (unsigned char)s.at(i + 1) == 0x80 &&
        ((unsigned char)s.at(i + 2) == 0xa8 || (unsigned char)s.at(i + 2) == 0xa9)) {
      w = bytes_into(w, (unsigned char)s.at(i + 2) == 0xa8 ? "\\u2028" : "\\u2029");
      i += 2;
    }
    else if (is_js_plain(s.at(i))) *w++ = s.at(i);
    else {
      w = bytes_into(w, "\\x");
      *w++ = kHexDigits.at(u >> 4);
      *w++ = kHexDigits.at(u & 15);
    }
  }
  return w;
}

constexpr bool
is_css_plain(const char c)
{
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == ' ' || c == '_' ||
         c == '-' || c == ',' || c == '.' || (unsigned char)c >= 0x80;
}

constexpr char *
css_string_escaped_into(char *w, const std::string_view s)
{
  for (const char c : s) {
    const unsigned char u = (unsigned char)c;
    if (is_css_plain(c)) *w++ = c;
    else {
      *w++ = '\\';
      *w++ = kHexDigits.at(u >> 4);
      *w++ = kHexDigits.at(u & 15);
      *w++ = ' ';
    }
  }
  return w;
}

constexpr bool
is_sentence_mark(const char c)
{
  return c == '.' || c == '!' || c == '?';
}

constexpr bool
is_closing_delimiter(const char c)
{
  return c == ')' || c == ']' || c == '\'' || c == '"';
}

inline constexpr unsigned char kFirstGraphic = 0x20;

inline constexpr unsigned char kDelete = 0x7f;

constexpr bool
is_troff_control_character(const char c)
{
  const unsigned char u = (unsigned char)c;
  return (u < kFirstGraphic && c != '\n') || u == kDelete;
}

constexpr bool
has_troff_control_character(const std::string_view s)
{
  return std::ranges::any_of(s, is_troff_control_character);
}

constexpr char *
troff_line_escaped_into(char *w, const std::string_view line)
{
  for (size_t j = 0; j < line.size(); j++) {
    const char c = line.at(j);
    const bool first = j == 0;
    const bool last = j + 1 == line.size();
    if (c == ' ') {
      if (first) w = bytes_into(w, "\\&");
      w = bytes_into(w, last ? "\\ " : " ");
    }
    else if (c == '.' && first) w = bytes_into(w, "\\&.");
    else if (c == '\\') w = bytes_into(w, "\\e");
    else if (c == '"') w = bytes_into(w, "\\(dq");
    else if (c == '\'') w = bytes_into(w, "\\(aq");
    else if (c == '`') w = bytes_into(w, "\\(ga");
    else if (c == '~') w = bytes_into(w, "\\(ti");
    else if (c == '^') w = bytes_into(w, "\\(ha");
    else *w++ = c;
  }
  size_t core = line.size();
  while (core > 0 && is_closing_delimiter(line.at(core - 1))) core--;
  const bool ends_in_space = !line.empty() && line.back() == ' ';
  const bool ends_a_sentence = core > 0 && is_sentence_mark(line.at(core - 1));
  if (ends_in_space || ends_a_sentence) w = bytes_into(w, "\\&");
  return w;
}

constexpr char *
troff_text_escaped_into(char *w, const std::string_view s)
{
  size_t from = 0;
  for (;;) {
    const size_t nl = s.find('\n', from);
    const std::string_view line = s.substr(from, nl == std::string_view::npos ? std::string_view::npos : nl - from);
    w = troff_line_escaped_into(w, line);
    if (nl == std::string_view::npos) return w;
    *w++ = '\n';
    from = nl + 1;
  }
}

struct Copied {
  size_t   count;
  uint64_t marks;
};

constexpr std::span<const char>
owned_chars_of(const std::string &s)
{
  return {s.data(), s.capacity() + 1};
}

template <class String, size_t Count>
constexpr bool
owned_chars_are_readable()
{
  const auto readable = [](const String &s) {
    const std::span<const char> owned = owned_chars_of(s);
    return std::ranges::count(owned, char{}) >= 1 && owned.first<Count>().size() == Count;
  };
  if (!readable(String{})) return false;
  for (const size_t n : {size_t{1}, Count - 1, Count, Count + 1, kCopiedStringMax, kCopiedStringMax + 1})
    if (!readable(String(n, char{}))) return false;
  return true;
}

template <class String, size_t Count>
inline constexpr bool kBufferHoldsCapacityAndTerminator =
    requires { typename std::enable_if_t<owned_chars_are_readable<String, Count>()>; };

inline Copied
copied_string_scalar(const std::span<char> out, const std::string &s)
{
  if (s.size() > kCopiedStringMax) [[unlikely]] return {0, 0};
  std::copy_n(s.begin(), s.size(), out.first(s.size()).begin());
  return {s.size(), std::ranges::any_of(s, is_marked) ? uint64_t{1} : uint64_t{0}};
}

#if defined(__AVX2__)

inline constexpr size_t kVectorBytes = sizeof(__m256i);

inline constexpr size_t kMarksPerVectorMax = kVectorBytes / 4;

static_assert(kEscapeSlack >= kVectorBytes, "a vector store at the end of the output stays in the slack");

inline uint32_t
marks_of(const __m256i b)
{
  const __m256i hit = _mm256_or_si256(
      _mm256_or_si256(_mm256_cmpeq_epi8(b, _mm256_set1_epi8('&')), _mm256_cmpeq_epi8(b, _mm256_set1_epi8('<'))),
      _mm256_or_si256(_mm256_cmpeq_epi8(b, _mm256_set1_epi8('>')),
                      _mm256_or_si256(_mm256_cmpeq_epi8(b, _mm256_set1_epi8('"')),
                                      _mm256_cmpeq_epi8(b, _mm256_set1_epi8('\'')))));
  return (uint32_t)_mm256_movemask_epi8(hit);
}

inline char *
escape_into(char *const out, const std::string_view s)
{
  if (s.size() < kVectorBytes) return escape_bytes(out, s);
  char *w = out;
  const auto *src = reinterpret_cast<const unsigned char *>(s.data());
  alignas(kVectorBytes) unsigned char block[2 * kVectorBytes];
  size_t at = 0;
  for (; at + kVectorBytes <= s.size(); at += kVectorBytes) {
    const __m256i b = _mm256_loadu_si256((const __m256i *)(src + at));
    uint32_t mask = marks_of(b);
    _mm256_storeu_si256((__m256i *)w, b);
    if (mask == 0) {
      w += kVectorBytes;
      continue;
    }
    if (std::cmp_greater(std::popcount(mask), kMarksPerVectorMax)) [[unlikely]] {
      w = escape_bytes(w, s.substr(at, kVectorBytes));
      continue;
    }
    const unsigned char *from = src + at;
    if (at + 2 * kVectorBytes > s.size()) {
      _mm256_store_si256((__m256i *)block, b);
      _mm256_store_si256((__m256i *)(block + kVectorBytes), _mm256_setzero_si256());
      from = block;
    }
    size_t done = 0;
    while (mask != 0) {
      const size_t i = static_cast<size_t>(std::countr_zero(mask));
      w += i - done;
      w = entity_into(w, kEntities.word.at(from[i]));
      done = i + 1;
      _mm256_storeu_si256((__m256i *)w, _mm256_loadu_si256((const __m256i *)(from + done)));
      mask &= mask - 1;
    }
    w += kVectorBytes - done;
  }
  return escape_bytes(w, s.substr(at));
}

inline uint32_t
marks16_of(const __m128i b)
{
  const __m128i hit = _mm_or_si128(
      _mm_or_si128(_mm_cmpeq_epi8(b, _mm_set1_epi8('&')), _mm_cmpeq_epi8(b, _mm_set1_epi8('<'))),
      _mm_or_si128(_mm_cmpeq_epi8(b, _mm_set1_epi8('>')),
                   _mm_or_si128(_mm_cmpeq_epi8(b, _mm_set1_epi8('"')), _mm_cmpeq_epi8(b, _mm_set1_epi8('\'')))));
  return (uint32_t)_mm_movemask_epi8(hit);
}

[[gnu::always_inline, gnu::flatten]] inline __m128i
loaded(const std::span<const char, sizeof(__m128i)> b)
{
  std::array<char, sizeof(__m128i)> a;
  std::copy_n(b.begin(), b.size(), a.begin());
  return std::bit_cast<__m128i>(a);
}

[[gnu::always_inline, gnu::flatten]] inline void
stored(const std::span<char, sizeof(__m128i)> o, const __m128i v)
{
  const auto a = std::bit_cast<std::array<char, sizeof(__m128i)>>(v);
  std::copy_n(a.begin(), a.size(), o.begin());
}

[[gnu::always_inline]] inline Copied
copied_string(const std::span<char> out, const std::string &s)
{
  const size_t n = s.size();
  if (n <= sizeof(__m128i)) {
    if constexpr (kBufferHoldsCapacityAndTerminator<std::string, sizeof(__m128i)>) {
      if (n == 0) return {0, 0};
      const std::span<const char> owned = owned_chars_of(s);
      if (owned.size() < sizeof(__m128i)) [[unlikely]] return copied_string_scalar(out, s);
      const __m128i v = loaded(owned.first<sizeof(__m128i)>());
      stored(out.first<sizeof(__m128i)>(), v);
      return {n, marks16_of(v) & ((uint32_t{1} << n) - 1)};
    } else {
      return copied_string_scalar(out, s);
    }
  }
  if (n > kCopiedStringMax) return copied_string_scalar(out, s);
  const std::span<const char> in(s);
  const __m128i a = loaded(in.first<sizeof(__m128i)>());
  const __m128i b = loaded(in.subspan(n - sizeof(__m128i)).first<sizeof(__m128i)>());
  stored(out.first<sizeof(__m128i)>(), a);
  stored(out.subspan(n - sizeof(__m128i)).first<sizeof(__m128i)>(), b);
  return {n, marks16_of(a) | marks16_of(b)};
}

#elif defined(__ARM_NEON)

inline constexpr size_t kVectorBytes = sizeof(uint8x16_t);

inline constexpr size_t kBitsPerLane = 4;

inline constexpr uint64_t kLaneMarkBits = 0x8888888888888888ull;

inline constexpr size_t kMarksPerVectorMax = kVectorBytes / 4;

static_assert(kEscapeSlack >= 2 * kVectorBytes, "a vector store at the end of the output stays in the slack");

inline uint64_t
marks_of(const uint8x16_t b)
{
  const uint8x16_t hit =
      vorrq_u8(vorrq_u8(vceqq_u8(b, vdupq_n_u8('&')), vceqq_u8(b, vdupq_n_u8('<'))),
               vorrq_u8(vceqq_u8(b, vdupq_n_u8('>')),
                        vorrq_u8(vceqq_u8(b, vdupq_n_u8('"')), vceqq_u8(b, vdupq_n_u8('\'')))));
  return vget_lane_u64(vreinterpret_u64_u8(vshrn_n_u16(vreinterpretq_u16_u8(hit), 4)), 0);
}

inline char *
escape_into(char *const out, const std::string_view s)
{
  if (s.size() < kVectorBytes) return escape_bytes(out, s);
  char *w = out;
  const auto *src = reinterpret_cast<const unsigned char *>(s.data());
  alignas(kVectorBytes) unsigned char block[2 * kVectorBytes];
  size_t at = 0;
  for (; at + kVectorBytes <= s.size(); at += kVectorBytes) {
    const uint8x16_t b = vld1q_u8(src + at);
    uint64_t mask = marks_of(b) & kLaneMarkBits;
    vst1q_u8((uint8_t *)w, b);
    if (mask == 0) {
      w += kVectorBytes;
      continue;
    }
    if (std::cmp_greater(std::popcount(mask), kMarksPerVectorMax)) [[unlikely]] {
      w = escape_bytes(w, s.substr(at, kVectorBytes));
      continue;
    }
    const unsigned char *from = src + at;
    if (at + 2 * kVectorBytes > s.size()) {
      vst1q_u8(block, b);
      vst1q_u8(block + kVectorBytes, vdupq_n_u8(0));
      from = block;
    }
    size_t done = 0;
    while (mask != 0) {
      const size_t i = static_cast<size_t>(std::countr_zero(mask)) / kBitsPerLane;
      w += i - done;
      w = entity_into(w, kEntities.word.at(from[i]));
      done = i + 1;
      vst1q_u8((uint8_t *)w, vld1q_u8(from + done));
      mask &= mask - 1;
    }
    w += kVectorBytes - done;
  }
  return escape_bytes(w, s.substr(at));
}

[[gnu::always_inline, gnu::flatten]] inline uint8x16_t
loaded(const std::span<const char, kVectorBytes> b)
{
  std::array<char, kVectorBytes> a;
  std::copy_n(b.begin(), b.size(), a.begin());
  return std::bit_cast<uint8x16_t>(a);
}

[[gnu::always_inline, gnu::flatten]] inline void
stored(const std::span<char, kVectorBytes> o, const uint8x16_t v)
{
  const auto a = std::bit_cast<std::array<char, kVectorBytes>>(v);
  std::copy_n(a.begin(), a.size(), o.begin());
}

[[gnu::always_inline]] inline Copied
copied_string(const std::span<char> out, const std::string &s)
{
  const size_t n = s.size();
  if (n <= kVectorBytes) {
    if constexpr (kBufferHoldsCapacityAndTerminator<std::string, kVectorBytes>) {
      if (n == 0) return {0, 0};
      const std::span<const char> owned = owned_chars_of(s);
      if (owned.size() < kVectorBytes) [[unlikely]] return copied_string_scalar(out, s);
      const uint8x16_t v = loaded(owned.first<kVectorBytes>());
      stored(out.first<kVectorBytes>(), v);
      return {n, marks_of(v) & kLaneMarkBits & (~uint64_t{0} >> (kBitsPerLane * (kVectorBytes - n)))};
    } else {
      return copied_string_scalar(out, s);
    }
  }
  if (n > kCopiedStringMax) return copied_string_scalar(out, s);
  const std::span<const char> in(s);
  const uint8x16_t a = loaded(in.first<kVectorBytes>());
  const uint8x16_t b = loaded(in.subspan(n - kVectorBytes).first<kVectorBytes>());
  stored(out.first<kVectorBytes>(), a);
  stored(out.subspan(n - kVectorBytes).first<kVectorBytes>(), b);
  return {n, (marks_of(a) | marks_of(b)) & kLaneMarkBits};
}

#else

inline constexpr size_t kVectorBytes = kEntityWordBytes;

constexpr char *
escape_into(char *const out, const std::string_view s)
{
  return escape_bytes(out, s);
}

inline Copied
copied_string(const std::span<char> out, const std::string &s)
{
  return copied_string_scalar(out, s);
}

#endif

[[gnu::always_inline]] inline size_t
escape_string_into(const std::span<char> out, const std::string &s)
{
  const Copied c = copied_string(out, s);
  if (s.size() <= kCopiedStringMax && c.marks == 0) [[likely]] return c.count;
  return static_cast<size_t>(std::distance(out.data(), escape_into(out.data(), s)));
}

}
