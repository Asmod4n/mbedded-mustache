#include <mustache/escape.hpp>
#include <benchmark/benchmark.h>
#include <cstring>
#include <bit>
#include <array>
#include <span>
#include <utility>
#include <iterator>
#include <ranges>
#include <string>
#include <vector>

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

void append_escaped(std::string &out, const std::string_view s)
{
  size_t from = 0;
  while (from < s.size()) {
    const auto rest = s.substr(from);
    const auto hit = std::ranges::find_if(rest, mustache::is_marked);
    const size_t k = static_cast<size_t>(hit - rest.begin());
    out.append(rest.substr(0, k));
    if (hit == rest.end()) break;
    const uint64_t word = mustache::kEntities.word[static_cast<unsigned char>(*hit)];
    for (size_t i = 0; i < mustache::entity_length(word); i++) out.push_back(static_cast<char>(word >> (8 * i)));
    from += k + 1;
  }
}


size_t escaped_by_runs(const std::span<char> out, const std::string_view s)
{
  size_t w = 0;
  size_t start = 0;
  const auto flush = [&](const size_t upto) {
    if (upto > start) {
      std::ranges::copy(s.substr(start, upto - start), out.subspan(w).begin());
      w += upto - start;
    }
  };
  const auto handle = [&](uint32_t mask, const size_t base) {
    while (mask != 0) {
      const size_t pos = base + static_cast<size_t>(std::countr_zero(mask));
      mask &= mask - 1;
      flush(pos);
      const uint64_t word = mustache::kEntities.word[static_cast<unsigned char>(s[pos])];
      for (size_t k = 0; k < mustache::entity_length(word); k++) out[w++] = static_cast<char>(word >> (8 * k));
      start = pos + 1;
    }
  };
  const size_t n = s.size();
  size_t at = 0;
  for (; at + 32 <= n; at += 32) {
    __m256i b;
    std::memcpy(&b, s.data() + at, sizeof b);
    handle(mustache::marks_of(b), at);
  }
  if (at < n) {
    if (n >= 32) {
      const size_t backs = 32 - (n - at);
      __m256i b;
      std::memcpy(&b, s.data() + n - 32, sizeof b);
      handle(mustache::marks_of(b) >> backs, at);
    }
    else {
      for (; at < n; at++) {
        if (mustache::is_marked(s[at])) handle(1u, at);
      }
    }
  }
  flush(n);
  return w;
}


__m256i loaded(const std::span<const char, 32> b)
{
  std::array<char, 32> a;
  std::copy_n(b.begin(), b.size(), a.begin());
  return std::bit_cast<__m256i>(a);
}

void stored(const std::span<char, 32> o, const __m256i v)
{
  const auto a = std::bit_cast<std::array<char, 32>>(v);
  std::copy_n(a.begin(), a.size(), o.begin());
}

void stored_word(const std::span<char, 8> o, const uint64_t word)
{
  const auto a = std::bit_cast<std::array<char, 8>>(word);
  std::copy_n(a.begin(), a.size(), o.begin());
}

size_t scalar_h(const std::span<char> out, size_t w, const std::string_view s)
{
  for (const char c : s) {
    const uint64_t word = mustache::kEntities.word[static_cast<unsigned char>(c)];
    stored_word(out.subspan(w).first<8>(), word);
    w += mustache::entity_length(word);
  }
  return w;
}

size_t escaped_h(const std::span<char> out, const std::string_view sv)
{
  const std::span<const char> in(sv);
  const size_t n = in.size();
  if (n < 32) return scalar_h(out, 0, sv);
  size_t w = 0;
  size_t at = 0;
  std::array<char, 64> block;
  for (; at + 32 <= n; at += 32) {
    const __m256i b = loaded(in.subspan(at).first<32>());
    uint32_t mask = mustache::marks_of(b);
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

size_t escaped_h2(const std::span<char> out, const std::string_view sv)
{
  const std::span<const char> in(sv);
  const size_t n = in.size();
  if (n < 32) return scalar_h(out, 0, sv);
  const __m256i amp = _mm256_set1_epi8('&'), lt = _mm256_set1_epi8('<'), gt = _mm256_set1_epi8('>'), quote = _mm256_set1_epi8('"'), apos = _mm256_set1_epi8('\'');
  const auto marks = [&](const __m256i x) {
    return static_cast<uint32_t>(_mm256_movemask_epi8(_mm256_or_si256(
        _mm256_or_si256(_mm256_cmpeq_epi8(x, amp), _mm256_cmpeq_epi8(x, lt)),
        _mm256_or_si256(_mm256_cmpeq_epi8(x, gt), _mm256_or_si256(_mm256_cmpeq_epi8(x, quote), _mm256_cmpeq_epi8(x, apos))))));
  };
  size_t w = 0;
  size_t at = 0;
  std::array<char, 64> block;
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
  std::string reused;
  std::vector<char> buffer(in.size() * 6 + mustache::kEscapeSlack);
  {
    std::string ref;
    escaped_by_char(std::back_inserter(ref), in);
    std::vector<char> probe(buffer.size());
    size_t got = 0;
#if ARM == 1
    got = static_cast<size_t>(mustache::escape_into(probe.data(), in) - probe.data());
#elif ARM == 6
    got = escaped_by_runs(probe, in);
#elif ARM == 7
    got = escaped_h(probe, in);
#elif ARM == 8
    got = escaped_h2(probe, in);
#endif
#if ARM == 1 || ARM == 6 || ARM == 7 || ARM == 8
    if (std::string_view(probe.data(), got) != ref) { state.SkipWithError("the arm gives other bytes"); return; }
#endif
  }
  for (auto _ : state) {
#if ARM == 1
    char *end = mustache::escape_into(buffer.data(), in);
    benchmark::DoNotOptimize(end);
#elif ARM == 2
    reused.clear();
    append_escaped(reused, in);
    benchmark::DoNotOptimize(reused.data());
#elif ARM == 3
    reused.clear();
    escaped_by_char(std::back_inserter(reused), in);
    benchmark::DoNotOptimize(reused.data());
#elif ARM == 4
    char *end = escaped_by_char(buffer.data(), in);
    benchmark::DoNotOptimize(end);
#elif ARM == 5
    auto end = escaped_by_char(std::span<char>(buffer).begin(), in);
    benchmark::DoNotOptimize(end);
#elif ARM == 6
    const size_t written = escaped_by_runs(buffer, in);
    benchmark::DoNotOptimize(written);
#elif ARM == 8
    const size_t written = escaped_h2(buffer, in);
    benchmark::DoNotOptimize(written);
#elif ARM == 7
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
