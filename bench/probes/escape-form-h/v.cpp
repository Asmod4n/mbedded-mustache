#include <mustache/escape.hpp>
#include <benchmark/benchmark.h>
#include <algorithm>
#include <array>
#include <bit>
#include <iterator>
#include <ranges>
#include <span>
#include <string>
#include <utility>
#include <vector>

#ifndef ARM
#error ARM
#endif

#define HELPER_ATTR
#define SCALAR_ATTR
#define MAIN_ATTR
#define COPYN 0
#define ALIGNED_BLOCK 0
#define HOISTED 0

#if ARM == 20
#undef MAIN_ATTR
#define MAIN_ATTR [[gnu::flatten]]
#elif ARM == 21
#undef COPYN
#define COPYN 1
#elif ARM == 22
#undef HELPER_ATTR
#define HELPER_ATTR [[gnu::always_inline]] inline
#elif ARM == 23
#undef SCALAR_ATTR
#define SCALAR_ATTR [[gnu::always_inline]] inline
#elif ARM == 24
#undef ALIGNED_BLOCK
#define ALIGNED_BLOCK 1
#elif ARM == 25
#undef SCALAR_ATTR
#define SCALAR_ATTR [[gnu::noinline]]
#elif ARM == 26
#undef COPYN
#define COPYN 1
#undef SCALAR_ATTR
#define SCALAR_ATTR [[gnu::always_inline]] inline
#elif ARM == 27
#undef MAIN_ATTR
#define MAIN_ATTR [[gnu::flatten]]
#undef HOISTED
#define HOISTED 1
#elif ARM == 30
#define COPYN_LOADED 1
#elif ARM == 31
#define COPYN_STORED 1
#elif ARM == 32
#define COPYN_WORD 1
#elif ARM == 28
#undef HOISTED
#define HOISTED 1
#undef SCALAR_ATTR
#define SCALAR_ATTR [[gnu::always_inline]] inline
#endif

#ifndef COPYN_LOADED
#define COPYN_LOADED COPYN
#endif
#ifndef COPYN_STORED
#define COPYN_STORED COPYN
#endif
#ifndef COPYN_WORD
#define COPYN_WORD COPYN
#endif

namespace {

template <class It>
It escaped_by_char(It out, const std::string_view s)
{
  for (const char c : s) {
    const uint64_t word = mustache::kEntities.word[static_cast<unsigned char>(c)];
    const size_t n = mustache::entity_length(word);
    for (size_t i = 0; i < n; i++) *out++ = static_cast<char>(word >> (8 * i));
  }
  return out;
}

#if ARM != 1 && ARM != 9

HELPER_ATTR __m256i loaded(const std::span<const char, 32> b)
{
  std::array<char, 32> a;
#if COPYN_LOADED
  std::copy_n(b.begin(), b.size(), a.begin());
#else
  std::ranges::copy(b, a.begin());
#endif
  return std::bit_cast<__m256i>(a);
}

HELPER_ATTR void stored(const std::span<char, 32> o, const __m256i v)
{
#if COPYN_STORED
  const std::array<char, 32> a = std::bit_cast<std::array<char, 32>>(v);
  std::copy_n(a.begin(), a.size(), o.begin());
#else
  std::ranges::copy(std::bit_cast<std::array<char, 32>>(v), o.begin());
#endif
}

HELPER_ATTR void stored_word(const std::span<char, 8> o, const uint64_t word)
{
#if COPYN_WORD
  const std::array<char, 8> a = std::bit_cast<std::array<char, 8>>(word);
  std::copy_n(a.begin(), a.size(), o.begin());
#else
  std::ranges::copy(std::bit_cast<std::array<char, 8>>(word), o.begin());
#endif
}

SCALAR_ATTR size_t scalar_h(const std::span<char> out, size_t w, const std::string_view s)
{
  for (const char c : s) {
    const uint64_t word = mustache::kEntities.word[static_cast<unsigned char>(c)];
    stored_word(out.subspan(w).first<8>(), word);
    w += mustache::entity_length(word);
  }
  return w;
}

MAIN_ATTR size_t escaped_h(const std::span<char> out, const std::string_view sv)
{
  const std::span<const char> in(sv);
  const size_t n = in.size();
  if (n < 32) return scalar_h(out, 0, sv);
#if HOISTED
  const __m256i amp = _mm256_set1_epi8('&'), lt = _mm256_set1_epi8('<'), gt = _mm256_set1_epi8('>'), quote = _mm256_set1_epi8('"'), apos = _mm256_set1_epi8('\'');
  const auto marks = [&](const __m256i x) {
    return static_cast<uint32_t>(_mm256_movemask_epi8(_mm256_or_si256(
        _mm256_or_si256(_mm256_cmpeq_epi8(x, amp), _mm256_cmpeq_epi8(x, lt)),
        _mm256_or_si256(_mm256_cmpeq_epi8(x, gt), _mm256_or_si256(_mm256_cmpeq_epi8(x, quote), _mm256_cmpeq_epi8(x, apos))))));
  };
#else
  const auto marks = [](const __m256i x) { return mustache::marks_of(x); };
#endif
  size_t w = 0;
  size_t at = 0;
#if ALIGNED_BLOCK
  alignas(32) std::array<char, 64> block;
#else
  std::array<char, 64> block;
#endif
  for (; at + 32 <= n; at += 32) {
    const __m256i b = loaded(in.subspan(at).first<32>());
    uint32_t mask = marks(b);
    stored(out.subspan(w).first<32>(), b);
    if (mask == 0) {
      w += 32;
      continue;
    }
    if (std::cmp_greater(std::popcount(mask), 8)) [[unlikely]] {
      w = scalar_h(out, w, sv.substr(at, 32));
      continue;
    }
    std::span<const char> from = in.subspan(at);
    if (at + 64 > n) {
      std::ranges::copy(std::bit_cast<std::array<char, 32>>(b), block.begin());
      std::ranges::fill(std::span(block).subspan(32), '\0');
      from = block;
    }
    size_t done = 0;
    while (mask != 0) {
      const size_t i = static_cast<size_t>(std::countr_zero(mask));
      w += i - done;
      const uint64_t word = mustache::kEntities.word[static_cast<unsigned char>(from[i])];
      stored_word(out.subspan(w).first<8>(), word);
      w += mustache::entity_length(word);
      done = i + 1;
      stored(out.subspan(w).first<32>(), loaded(from.subspan(done).first<32>()));
      mask &= mask - 1;
    }
    w += 32 - done;
  }
  return scalar_h(out, w, sv.substr(at));
}
#endif

std::string input_of(const size_t length, const size_t percent)
{
  std::string s;
  uint32_t x = 12345;
  for (size_t i = 0; i < length; i++) {
    x = x * 1664525u + 1013904223u;
    s.push_back((x >> 16) % 100 < percent ? "<>&\"'"[(x >> 8) % 5] : static_cast<char>('a' + (x >> 20) % 26));
  }
  return s;
}

void arm(benchmark::State &state)
{
  const std::string in = input_of(static_cast<size_t>(state.range(0)), static_cast<size_t>(state.range(1)));
  std::vector<char> buffer(in.size() * 6 + mustache::kEscapeSlack);
  {
    std::string ref;
    escaped_by_char(std::back_inserter(ref), in);
    std::vector<char> probe(buffer.size());
    size_t got = 0;
#if ARM == 1 || ARM == 9
    got = static_cast<size_t>(mustache::escape_into(probe.data(), in) - probe.data());
#else
    got = escaped_h(probe, in);
#endif
    if (std::string_view(probe.data(), got) != ref) { state.SkipWithError("the arm gives other bytes"); return; }
  }
  for (auto _ : state) {
#if ARM == 1
    char *end = mustache::escape_into(buffer.data(), in);
    benchmark::DoNotOptimize(end);
#elif ARM == 9
    const size_t written = static_cast<size_t>(mustache::escape_into(buffer.data(), in) - buffer.data());
    benchmark::DoNotOptimize(written);
#else
    const size_t written = escaped_h(buffer, in);
    benchmark::DoNotOptimize(written);
#endif
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(in.size()));
}

}

BENCHMARK(arm)->Args({64, 0})->Args({64, 5})->Args({1024, 0})->Args({1024, 5});
BENCHMARK_MAIN();
