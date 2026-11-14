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
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace mustache {

inline constexpr size_t kEntityMax = 6;

inline constexpr size_t kEntityWordBytes = sizeof(uint64_t);

inline constexpr size_t kEntityLengthShift = (kEntityWordBytes - 1) * std::numeric_limits<unsigned char>::digits;

inline constexpr size_t kCopiedStringMax = 32;

inline constexpr size_t kEscapeSlack = kCopiedStringMax;

inline constexpr size_t kBytesPastTheWrittenCount = kEscapeSlack;

struct Entities {
  std::array<uint64_t, 256> word;
};

consteval uint64_t
word_of(const std::string_view piece)
{
  uint64_t word = uint64_t{piece.size()} << kEntityLengthShift;
  size_t shift = 0;
  for (const char c : piece) {
    word |= uint64_t{static_cast<unsigned char>(c)} << shift;
    shift += std::numeric_limits<unsigned char>::digits;
  }
  return word;
}

consteval Entities
entities_of()
{
  Entities e{};
  for (size_t c = 0; c < 256; c++) e.word.at(c) = (uint64_t)c | (uint64_t{1} << 56);
  auto put = [&e](const unsigned char c, const std::string_view s) consteval { e.word.at(c) = word_of(s); };
  put('&', "&amp;");
  put('<', "&lt;");
  put('>', "&gt;");
  put('"', "&quot;");
  put('\'', "&#39;");
  return e;
}

inline constexpr Entities kEntities = entities_of();

constexpr uint64_t
entity_word_of(const Entities &table, const unsigned char c)
{
  static_assert(std::cmp_less(std::numeric_limits<unsigned char>::max(), std::tuple_size_v<decltype(Entities::word)>),
                "every value of unsigned char indexes a word of the table");
  return table.word[c];
}

constexpr size_t
entity_length(const uint64_t word)
{
  return static_cast<size_t>(word >> kEntityLengthShift);
}

constexpr bool
is_marked(const char c)
{
  return entity_length(entity_word_of(kEntities, static_cast<unsigned char>(c))) != 1;
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
  for (const char c : s) w = entity_into(w, entity_word_of(kEntities, static_cast<unsigned char>(c)));
  return w;
}

[[gnu::always_inline, gnu::flatten]] constexpr void
stored_word(const std::span<char, kEntityWordBytes> o, const uint64_t word)
{
  const auto bytes = std::bit_cast<std::array<char, kEntityWordBytes>>(word);
  std::copy_n(bytes.begin(), bytes.size(), o.begin());
}

[[gnu::always_inline, gnu::flatten]] constexpr size_t
word_appended(const std::span<char> room, const size_t count, const uint64_t word)
{
  static_assert(kBytesPastTheWrittenCount >= kEntityWordBytes,
                "a word stored at the written count stays in the bytes that the caller proved past the count");
  stored_word(room.subspan(count).first<kEntityWordBytes>(), word);
  return count + entity_length(word);
}

[[gnu::always_inline, gnu::flatten]] constexpr size_t
table_escaped_into(const std::span<char> room, const size_t count, const std::string_view s, const Entities &table)
{
  size_t at = count;
  for (const char c : s) at = word_appended(room, at, entity_word_of(table, static_cast<unsigned char>(c)));
  return at;
}

template <class WordOf>
consteval Entities
entities_by(const WordOf &word_of_byte)
{
  Entities e{};
  for (size_t c = 0; c < e.word.size(); c++) e.word.at(c) = word_of_byte(static_cast<char>(static_cast<unsigned char>(c)));
  return e;
}

consteval uint64_t
byte_word_of(const char c)
{
  return uint64_t{static_cast<unsigned char>(c)} | (uint64_t{1} << kEntityLengthShift);
}

inline constexpr std::string_view kHexDigits = "0123456789abcdef";

consteval uint64_t
with_hex_digits_at(const uint64_t word, const size_t at, const unsigned char u)
{
  const size_t shift = at * std::numeric_limits<unsigned char>::digits;
  return word | uint64_t{static_cast<unsigned char>(kHexDigits.at(u >> 4))} << shift |
         uint64_t{static_cast<unsigned char>(kHexDigits.at(u & 15))} << (shift + std::numeric_limits<unsigned char>::digits);
}

consteval uint64_t
xml_word_of(const char c)
{
  switch (c) {
    case '&':  return word_of("&amp;");
    case '<':  return word_of("&lt;");
    case '>':  return word_of("&gt;");
    case '"':  return word_of("&quot;");
    case '\'': return word_of("&apos;");
    default:   return byte_word_of(c);
  }
}

consteval uint64_t
json_word_of(const char c)
{
  using namespace std::string_view_literals;
  switch (c) {
    case '"':  return word_of("\\\"");
    case '\\': return word_of("\\\\");
    case '\b': return word_of("\\b");
    case '\f': return word_of("\\f");
    case '\n': return word_of("\\n");
    case '\r': return word_of("\\r");
    case '\t': return word_of("\\t");
    default:   break;
  }
  const unsigned char u = static_cast<unsigned char>(c);
  if (u < 0x20) return with_hex_digits_at(word_of("\\u00\0\0"sv), 4, u);
  return byte_word_of(c);
}

constexpr bool
is_js_plain(const char c)
{
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == ' ' || c == '_' ||
         c == ',' || c == '.' || (unsigned char)c >= 0x80;
}

consteval uint64_t
js_word_of(const char c)
{
  using namespace std::string_view_literals;
  if (is_js_plain(c)) return byte_word_of(c);
  return with_hex_digits_at(word_of("\\x\0\0"sv), 2, static_cast<unsigned char>(c));
}

constexpr bool
is_css_plain(const char c)
{
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == ' ' || c == '_' ||
         c == '-' || c == ',' || c == '.' || (unsigned char)c >= 0x80;
}

consteval uint64_t
css_word_of(const char c)
{
  using namespace std::string_view_literals;
  if (is_css_plain(c)) return byte_word_of(c);
  return with_hex_digits_at(word_of("\\\0\0 "sv), 1, static_cast<unsigned char>(c));
}

consteval uint64_t
troff_word_of(const char c)
{
  switch (c) {
    case '\\': return word_of("\\e");
    case '"':  return word_of("\\(dq");
    case '\'': return word_of("\\(aq");
    case '`':  return word_of("\\(ga");
    case '~':  return word_of("\\(ti");
    case '^':  return word_of("\\(ha");
    default:   return byte_word_of(c);
  }
}

inline constexpr Entities kXmlEntities = entities_by(xml_word_of);

inline constexpr Entities kJsonEntities = entities_by(json_word_of);

inline constexpr Entities kJsEntities = entities_by(js_word_of);

inline constexpr Entities kCssEntities = entities_by(css_word_of);

inline constexpr Entities kTroffEntities = entities_by(troff_word_of);

inline constexpr std::string_view kLineSeparator = "\xe2\x80\xa8";

inline constexpr std::string_view kParagraphSeparator = "\xe2\x80\xa9";

constexpr size_t
xml_escaped_into(const std::span<char> room, const std::string_view s)
{
  return table_escaped_into(room, 0, s, kXmlEntities);
}

constexpr size_t
json_string_escaped_into(const std::span<char> room, const std::string_view s)
{
  return table_escaped_into(room, 0, s, kJsonEntities);
}

constexpr size_t
js_string_escaped_into(const std::span<char> room, const std::string_view s)
{
  size_t count = 0;
  for (auto at = s.begin(); at != s.end(); ++at) {
    const std::string_view rest(at, s.end());
    if (*at != kLineSeparator.front()) count = word_appended(room, count, entity_word_of(kJsEntities, static_cast<unsigned char>(*at)));
    else if (rest.starts_with(kLineSeparator)) {
      count = word_appended(room, count, word_of("\\u2028"));
      at = std::next(at, kLineSeparator.size() - 1);
    }
    else if (rest.starts_with(kParagraphSeparator)) {
      count = word_appended(room, count, word_of("\\u2029"));
      at = std::next(at, kParagraphSeparator.size() - 1);
    }
    else count = word_appended(room, count, entity_word_of(kJsEntities, static_cast<unsigned char>(*at)));
  }
  return count;
}

constexpr size_t
css_string_escaped_into(const std::span<char> room, const std::string_view s)
{
  return table_escaped_into(room, 0, s, kCssEntities);
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

constexpr size_t
troff_line_escaped_into(const std::span<char> room, const std::string_view line)
{
  size_t count = 0;
  if (line.starts_with(' ') || line.starts_with('.')) count = word_appended(room, count, word_of("\\&"));
  const bool ends_in_space = line.ends_with(' ');
  count = table_escaped_into(room, count, ends_in_space ? line.substr(0, line.size() - 1) : line, kTroffEntities);
  if (ends_in_space) count = word_appended(room, count, word_of("\\ "));
  const auto core = std::find_if_not(line.rbegin(), line.rend(), is_closing_delimiter);
  const bool ends_a_sentence = core != line.rend() && is_sentence_mark(*core);
  if (ends_in_space || ends_a_sentence) count = word_appended(room, count, word_of("\\&"));
  return count;
}

constexpr size_t
troff_text_escaped_into(const std::span<char> room, const std::string_view s)
{
  size_t count = 0;
  std::string_view rest = s;
  for (;;) {
    const size_t nl = rest.find('\n');
    count += troff_line_escaped_into(room.subspan(count), rest.substr(0, nl));
    if (nl == std::string_view::npos) return count;
    count = word_appended(room, count, word_of("\n"));
    rest.remove_prefix(nl + 1);
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
      w = entity_into(w, entity_word_of(kEntities, from[i]));
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
      w = entity_into(w, entity_word_of(kEntities, from[i]));
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
