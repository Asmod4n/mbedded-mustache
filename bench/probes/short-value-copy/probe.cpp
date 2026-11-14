// Probe: forms of copied_string for a value under 16 bytes, against the form of dbd4cac.
// One binary times one arm, chosen with -DARM=<number>; the arm is held to the scalar form first.
// Arms: 0 and 1 the form of dbd4cac (the same code twice, for the noise), 2 the form of d13b6ca,
// 3 a block built in a zeroed std::array from overlapping pieces inside the length,
// 4 a block built from 64-bit words in registers inside the length, 5 the pieces stored one by one,
// 6 one 16-byte read through a span over capacity() + 1 chars, 7 the same read through a span over
// capacity() chars (a short string of libstdc++ then takes arm 4).
// Each arm stores into a room of at least 32 bytes, as the callers in render.hpp prove.
#include <immintrin.h>

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <print>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace probe {

struct Entities {
  std::array<uint64_t, 256> word;
};

consteval Entities
entities_of()
{
  Entities e{};
  for (size_t c = 0; c < 256; c++) e.word.at(c) = uint64_t{c} | (uint64_t{1} << 56);
  auto put = [&e](const unsigned char c, const std::string_view s) {
    uint64_t v = uint64_t{s.size()} << 56;
    for (size_t i = 0; i < s.size(); i++) v |= uint64_t{static_cast<unsigned char>(s.at(i))} << (8 * i);
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
inline constexpr size_t kCopiedStringMax = 32;
inline constexpr size_t kBlock = sizeof(__m128i);
inline constexpr size_t kWord = sizeof(uint64_t);
inline constexpr size_t kHalfWord = sizeof(uint32_t);
inline constexpr size_t kBitsPerByte = 8;

constexpr bool
is_marked(const char c)
{
  return (kEntities.word.at(static_cast<unsigned char>(c)) >> 56) != 1;
}

struct Copied {
  size_t   count;
  uint64_t marks;
};

inline uint32_t
marks16_of(const __m128i b)
{
  const __m128i hit = _mm_or_si128(
      _mm_or_si128(_mm_cmpeq_epi8(b, _mm_set1_epi8('&')), _mm_cmpeq_epi8(b, _mm_set1_epi8('<'))),
      _mm_or_si128(_mm_cmpeq_epi8(b, _mm_set1_epi8('>')),
                   _mm_or_si128(_mm_cmpeq_epi8(b, _mm_set1_epi8('"')), _mm_cmpeq_epi8(b, _mm_set1_epi8('\'')))));
  return static_cast<uint32_t>(_mm_movemask_epi8(hit));
}

[[gnu::always_inline, gnu::flatten]] inline __m128i
loaded(const std::span<const char, kBlock> b)
{
  std::array<char, kBlock> a;
  std::copy_n(b.begin(), b.size(), a.begin());
  return std::bit_cast<__m128i>(a);
}

[[gnu::always_inline, gnu::flatten]] inline void
stored(const std::span<char, kBlock> o, const __m128i v)
{
  const auto a = std::bit_cast<std::array<char, kBlock>>(v);
  std::copy_n(a.begin(), a.size(), o.begin());
}

[[gnu::always_inline, gnu::flatten]] inline uint64_t
word_of(const std::span<const char, kWord> b)
{
  std::array<char, kWord> a;
  std::copy_n(b.begin(), b.size(), a.begin());
  return std::bit_cast<uint64_t>(a);
}

[[gnu::always_inline, gnu::flatten]] inline uint32_t
half_word_of(const std::span<const char, kHalfWord> b)
{
  std::array<char, kHalfWord> a;
  std::copy_n(b.begin(), b.size(), a.begin());
  return std::bit_cast<uint32_t>(a);
}

[[gnu::always_inline, gnu::flatten]] inline void
word_stored(const std::span<char, kWord> o, const uint64_t v)
{
  const auto a = std::bit_cast<std::array<char, kWord>>(v);
  std::copy_n(a.begin(), a.size(), o.begin());
}

[[gnu::always_inline, gnu::flatten]] inline void
half_word_stored(const std::span<char, kHalfWord> o, const uint32_t v)
{
  const auto a = std::bit_cast<std::array<char, kHalfWord>>(v);
  std::copy_n(a.begin(), a.size(), o.begin());
}

inline Copied
copied_string_scalar(const std::span<char> out, const std::string &s)
{
  if (s.size() > kCopiedStringMax) [[unlikely]] return {0, 0};
  std::copy_n(s.begin(), s.size(), out.first(s.size()).begin());
  return {s.size(), std::ranges::any_of(s, is_marked) ? uint64_t{1} : uint64_t{0}};
}

[[gnu::always_inline]] inline Copied
copied_long(const std::span<char> out, const std::span<const char> in)
{
  const size_t n = in.size();
  const __m128i a = loaded(in.first<kBlock>());
  const __m128i b = loaded(in.subspan(n - kBlock).first<kBlock>());
  stored(out.first<kBlock>(), a);
  stored(out.subspan(n - kBlock).first<kBlock>(), b);
  return {n, marks16_of(a) | marks16_of(b)};
}

// The form of dbd4cac, as it stood in include/mustache/escape.hpp (char *w in place of the count).
namespace before {

inline constexpr size_t kShortStringRead = 16;

constexpr bool
reads_past_size_safely(const std::string &s)
{
  return s.size() <= kShortStringRead && s.capacity() + 1 >= kShortStringRead;
}

struct Copied {
  char    *w;
  uint64_t marks;
};

inline Copied
copied_string_scalar(const std::span<char> out, const std::string &s)
{
  if (s.size() > kCopiedStringMax || s.size() > out.size()) [[unlikely]] return {nullptr, 0};
  const bool marked = std::ranges::any_of(s, is_marked);
  const auto copied = std::ranges::copy(s, out.begin());
  return {std::to_address(copied.out), marked ? uint64_t{1} : uint64_t{0}};
}

[[gnu::always_inline]] inline Copied
copied_string(const std::span<char> out, const std::string &s)
{
  if (out.size() < kCopiedStringMax) [[unlikely]] return copied_string_scalar(out, s);
  const size_t n = s.size();
  if (reads_past_size_safely(s)) {
    const __m128i b = _mm_loadu_si128((const __m128i *)s.data());
    _mm_storeu_si128((__m128i *)out.data(), b);
    return {out.data() + n, marks16_of(b) & ((1u << n) - 1)};
  }
  if (n >= kShortStringRead && n <= kCopiedStringMax) {
    const __m128i a = _mm_loadu_si128((const __m128i *)s.data());
    const __m128i b = _mm_loadu_si128((const __m128i *)s.substr(n - kShortStringRead).data());
    _mm_storeu_si128((__m128i *)out.data(), a);
    _mm_storeu_si128((__m128i *)out.subspan(n - kShortStringRead).data(), b);
    return {out.data() + n, marks16_of(a) | marks16_of(b)};
  }
  return copied_string_scalar(out, s);
}

}

// The form of d13b6ca.
[[gnu::always_inline]] inline Copied
copied_d13b6ca(const std::span<char> out, const std::string &s)
{
  const size_t n = s.size();
  if (n < kBlock || n > kCopiedStringMax) return copied_string_scalar(out, s);
  return copied_long(out, s);
}

// Arm 3: one block in a zeroed std::array from overlapping pieces inside the length.
[[gnu::always_inline]] inline Copied
copied_short_in_array(const std::span<char> out, const std::span<const char> in)
{
  const size_t n = in.size();
  if (n == 0) return {0, 0};
  std::array<char, kBlock> block{};
  const std::span<char, kBlock> b(block);
  if (n >= kWord) {
    std::copy_n(in.first<kWord>().begin(), kWord, b.first<kWord>().begin());
    std::copy_n(in.subspan(n - kWord).first<kWord>().begin(), kWord, b.subspan(n - kWord).first<kWord>().begin());
  }
  else if (n >= kHalfWord) {
    std::copy_n(in.first<kHalfWord>().begin(), kHalfWord, b.first<kHalfWord>().begin());
    std::copy_n(in.subspan(n - kHalfWord).first<kHalfWord>().begin(), kHalfWord,
                b.subspan(n - kHalfWord).first<kHalfWord>().begin());
  }
  else {
    b.front() = in.front();
    b.subspan(n / 2).front() = in.subspan(n / 2).front();
    b.subspan(n - 1).front() = in.subspan(n - 1).front();
  }
  const __m128i v = std::bit_cast<__m128i>(block);
  stored(out.first<kBlock>(), v);
  return {n, marks16_of(v)};
}

// The bytes of a value of 1 to 3 bytes in the low bytes of a word, the rest zero.
[[gnu::always_inline]] inline uint64_t
word_of_up_to_3(const std::span<const char> in)
{
  const size_t n = in.size();
  return uint64_t{static_cast<unsigned char>(in.front())} |
         uint64_t{static_cast<unsigned char>(in.subspan(n / 2).front())} << (kBitsPerByte * (n / 2)) |
         uint64_t{static_cast<unsigned char>(in.subspan(n - 1).front())} << (kBitsPerByte * (n - 1));
}

// Arm 4: one block from 64-bit words in registers, inside the length.
[[gnu::always_inline]] inline Copied
copied_short_in_words(const std::span<char> out, const std::span<const char> in)
{
  const size_t n = in.size();
  if (n == 0) return {0, 0};
  std::array<uint64_t, 2> words{};
  if (n >= kWord) {
    words.front() = word_of(in.first<kWord>());
    words.back() = (word_of(in.subspan(n - kWord).first<kWord>()) >> (kBitsPerByte * (2 * kWord - 1 - n))) >> kBitsPerByte;
  }
  else if (n >= kHalfWord) {
    const uint64_t high = (half_word_of(in.subspan(n - kHalfWord).first<kHalfWord>()) >> (kBitsPerByte * (kWord - 1 - n))) >>
                          kBitsPerByte;
    words.front() = half_word_of(in.first<kHalfWord>()) | high << (kBitsPerByte * kHalfWord);
  }
  else {
    words.front() = word_of_up_to_3(in);
  }
  const __m128i v = std::bit_cast<__m128i>(words);
  stored(out.first<kBlock>(), v);
  return {n, marks16_of(v)};
}

// Arm 5: the pieces stored one by one; nothing is stored past the length.
[[gnu::always_inline]] inline Copied
copied_short_in_pieces(const std::span<char> out, const std::span<const char> in)
{
  const size_t n = in.size();
  if (n == 0) return {0, 0};
  std::array<uint64_t, 2> words{};
  if (n >= kWord) {
    words.front() = word_of(in.first<kWord>());
    words.back() = word_of(in.subspan(n - kWord).first<kWord>());
    word_stored(out.first<kWord>(), words.front());
    word_stored(out.subspan(n - kWord).first<kWord>(), words.back());
  }
  else if (n >= kHalfWord) {
    const uint32_t low = half_word_of(in.first<kHalfWord>());
    const uint32_t high = half_word_of(in.subspan(n - kHalfWord).first<kHalfWord>());
    half_word_stored(out.first<kHalfWord>(), low);
    half_word_stored(out.subspan(n - kHalfWord).first<kHalfWord>(), high);
    words.front() = low | uint64_t{high} << (kBitsPerByte * kHalfWord);
  }
  else {
    out.front() = in.front();
    out.subspan(n / 2).front() = in.subspan(n / 2).front();
    out.subspan(n - 1).front() = in.subspan(n - 1).front();
    words.front() = word_of_up_to_3(in);
  }
  return {n, marks16_of(std::bit_cast<__m128i>(words))};
}

// Arms 6 and 7: the chars that the string owns, as one span. Arm 6 counts the terminator.
#if ARM == 6
inline constexpr size_t kOwnedPastCapacity = 1;
#else
inline constexpr size_t kOwnedPastCapacity = 0;
#endif

inline std::span<const char>
owned_chars_of(const std::string &s)
{
  return {s.data(), s.capacity() + kOwnedPastCapacity};
}

[[gnu::always_inline]] inline Copied
copied_in_capacity(const std::span<char> out, const std::string &s)
{
  const size_t n = s.size();
  if (n > kCopiedStringMax) return copied_string_scalar(out, s);
  if (n >= kBlock) return copied_long(out, s);
  if (n == 0) return {0, 0};
  const std::span<const char> owned = owned_chars_of(s);
  if (owned.size() < kBlock) return copied_short_in_words(out, s);
  const __m128i v = loaded(owned.first<kBlock>());
  stored(out.first<kBlock>(), v);
  return {n, marks16_of(v) & ((uint32_t{1} << n) - 1)};
}

enum class Short { in_array, in_words, in_pieces };

template <Short form>
[[gnu::always_inline]] inline Copied
copied_with(const std::span<char> out, const std::string &s)
{
  const size_t n = s.size();
  if (n > kCopiedStringMax) return copied_string_scalar(out, s);
  if (n >= kBlock) return copied_long(out, s);
  if constexpr (form == Short::in_array) return copied_short_in_array(out, s);
  else if constexpr (form == Short::in_words) return copied_short_in_words(out, s);
  else return copied_short_in_pieces(out, s);
}

[[gnu::always_inline]] inline Copied
copied(const std::span<char> out, const std::string &s)
{
#if ARM == 0 || ARM == 1
  const before::Copied c = before::copied_string(out, s);
  return {c.w == nullptr ? 0 : static_cast<size_t>(std::distance(out.data(), c.w)), c.marks};
#elif ARM == 2
  return copied_d13b6ca(out, s);
#elif ARM == 3
  return copied_with<Short::in_array>(out, s);
#elif ARM == 4
  return copied_with<Short::in_words>(out, s);
#elif ARM == 5
  return copied_with<Short::in_pieces>(out, s);
#elif ARM == 6 || ARM == 7
  return copied_in_capacity(out, s);
#endif
}

inline constexpr size_t kRoom = kCopiedStringMax;
inline constexpr size_t kBufferBytes = 1 << 16;
inline constexpr size_t kValuesPerRepetition = 1 << 22;
inline constexpr int kRepetitions = 5;

struct Sum {
  size_t   count;
  uint64_t marked;
};

[[gnu::noinline]] Sum
copied_values(const std::span<char> buffer, const std::vector<std::string> &values, const size_t passes)
{
  Sum sum{0, 0};
  size_t at = 0;
  for (size_t p = 0; p < passes; p++) {
    for (const std::string &s : values) {
      const Copied c = copied(buffer.subspan(at, kRoom), s);
      at += c.count;
      sum.count += c.count;
      sum.marked += c.marks != 0;
      if (at > buffer.size() - kRoom) at = 0;
    }
  }
  return sum;
}

std::string
with_marks_behind(const std::string_view value)
{
  std::string s;
  s.resize_and_overwrite(kBlock - 1, [value](char *const p, const size_t n) {
    const std::span<char> all(p, n);
    std::ranges::fill(all, '<');
    std::ranges::copy(value, all.begin());
    return value.size();
  });
  return s;
}

bool
held_to_the_scalar_form(const std::string &s)
{
  std::array<char, 2 * kCopiedStringMax> a{};
  std::array<char, 2 * kCopiedStringMax> b{};
  const Copied w = copied(std::span<char>(a).first(kRoom), s);
  const Copied c = copied_string_scalar(b, s);
  const bool same = w.count == c.count && (w.marks != 0) == (c.marks != 0) &&
                    std::ranges::equal(std::span(a).first(c.count), std::span(b).first(c.count)) &&
                    std::ranges::all_of(std::span(a).subspan(kRoom), [](const char x) { return x == 0; });
  if (!same) std::println(stderr, "arm {} differs from the scalar form at a value of {} bytes", ARM, s.size());
  return same;
}

bool
every_case_held()
{
  constexpr std::string_view kMarks = "&<>\"'";
  bool all = true;
  for (size_t n = 0; n <= kCopiedStringMax + 8; n++) {
    all = held_to_the_scalar_form(std::string(n, 'x')) && all;
    if (n < kBlock) all = held_to_the_scalar_form(with_marks_behind(std::string(n, 'x'))) && all;
    for (size_t at = 0; at < n; at++) {
      for (const char m : kMarks) {
        std::string s(n, 'x');
        s.at(at) = m;
        all = held_to_the_scalar_form(s) && all;
        if (n < kBlock) all = held_to_the_scalar_form(with_marks_behind(s)) && all;
      }
    }
  }
  return all;
}

// The values under 16 bytes of bench/render.cpp, in the order of the render: workload 2, then workload 3.
// Workload 1 has none (its title has 16 bytes).
std::vector<std::string>
values_of_the_render()
{
  struct Stock {
    std::string_view symbol, name, price, change, ratio;
  };
  const std::array<Stock, 20> stocks = {{
      {"ADBE", "Adobe Systems", "39.26", "0.13", "0.33"},       {"AMD", "Advanced Micro Devices", "16.22", "0.17", "1.06"},
      {"AMZN", "Amazon.com", "36.85", "-0.23", "-0.62"},        {"AAPL", "Apple", "85.38", "-0.87", "-1.01"},
      {"BEAS", "BEA Systems", "12.46", "0.09", "0.73"},         {"CA", "CA", "24.66", "0.38", "1.57"},
      {"CSCO", "Cisco Systems", "26.35", "0.13", "0.5"},        {"DELL", "Dell", "23.73", "-0.42", "-1.74"},
      {"EBAY", "eBay", "31.65", "-0.8", "-2.47"},               {"GOOG", "Google", "495.84", "7.75", "1.59"},
      {"HPQ", "Hewlett-Packard", "41.69", "-0.02", "-0.05"},    {"IBM", "IBM", "97.45", "-0.06", "-0.06"},
      {"INTC", "Intel", "20.53", "-0.07", "-0.34"},             {"JNPR", "Juniper Networks", "18.96", "0.5", "2.71"},
      {"MSFT", "Microsoft", "30.6", "0.15", "0.49"},            {"ORCL", "Oracle", "17.15", "0.17", "1.1"},
      {"SAP", "SAP", "46.2", "-0.16", "-0.35"},                 {"STX", "Seagate Technology", "27.35", "-0.36", "-1.3"},
      {"SUNW", "Sun Microsystems", "6.33", "-0.01", "-0.16"},   {"YHOO", "Yahoo", "28.04", "-0.17", "-0.6"},
  }};
  std::vector<std::string> v;
  const auto put = [&v](const std::string_view s) {
    if (s.size() < kBlock) v.emplace_back(s);
  };
  for (size_t i = 0; i < stocks.size(); i++) {
    const Stock &s = stocks.at(i);
    const bool up = s.change.front() != '-' && s.change != "0";
    const std::string_view negative = up ? "" : "class=\"minus\"";
    put(i % 2 == 0 ? "even" : "odd");
    put(std::to_string(i + 1));
    put(s.symbol);
    put(s.symbol);
    put(s.name);
    put(s.price);
    put(negative);
    put(s.change);
    put(negative);
    put(s.ratio);
  }
  put("Today's news");
  put("Today's news");
  for (int i = 0; i < 20; i++) {
    const std::string n = std::to_string(i + 1);
    put("Author " + n);
    put("2026-09-" + std::string(i < 9 ? "0" : "") + n);
    put("world");
    put("tech");
    put("tag-" + n);
  }
  return v;
}

std::vector<std::string>
plain(std::vector<std::string> v)
{
  for (std::string &s : v) std::ranges::replace_if(s, is_marked, 'x');
  return v;
}

std::vector<std::string>
fixed(const size_t n, const bool marked)
{
  std::string s(n, 'x');
  if (marked) s.back() = '<';
  return std::vector<std::string>(64, s);
}

}

int
main(const int argc, const char *const *const argv)
{
  using namespace probe;
  if (!every_case_held()) return 1;
  const std::vector<std::string> render = values_of_the_render();
  const std::array<std::pair<std::string_view, std::vector<std::string>>, 8> inputs = {{
      {"render", render},
      {"render-plain", plain(render)},
      {"fixed-5", fixed(5, false)},
      {"fixed-5-marked", fixed(5, true)},
      {"fixed-8", fixed(8, false)},
      {"fixed-8-marked", fixed(8, true)},
      {"fixed-12", fixed(12, false)},
      {"fixed-12-marked", fixed(12, true)},
  }};
  for (const auto &[name, values] : inputs) {
    for (const std::string &s : values) {
      if (s.capacity() + 1 < kBlock) {
        std::println(stderr, "a value of {} bytes has capacity {}", s.size(), s.capacity());
        return 1;
      }
      if (!held_to_the_scalar_form(s)) return 1;
    }
  }
  const std::string_view label = argc > 2 ? argv[2] : "";
  const std::string_view round = argc > 1 ? argv[1] : "0";
  std::vector<char> buffer(kBufferBytes);
  for (const auto &[name, values] : inputs) {
    const size_t passes = kValuesPerRepetition / values.size();
    for (int r = 0; r < kRepetitions; r++) {
      const auto start = std::chrono::steady_clock::now();
      const Sum sum = copied_values(buffer, values, passes);
      const std::chrono::duration<double, std::nano> took = std::chrono::steady_clock::now() - start;
      std::println("round={} {} input={} ns_per_value={:.4f}", round, label, name,
                   took.count() / static_cast<double>(passes * values.size()));
      std::println(stderr, "count {} marked {}", sum.count, sum.marked);
    }
  }
  return 0;
}
