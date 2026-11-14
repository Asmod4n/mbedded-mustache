// The final form of arm H: the HTML escape of mustache::escape_into, written with
// std::span of fixed extent, std::bit_cast and std::copy_n. One change against
// escaped_h of sink.cpp: std::copy_n stands where std::ranges::copy stood in
// loaded, stored and stored_word.
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

__m256i loaded(const std::span<const char, 32> b)
{
  std::array<char, 32> a;
  std::copy_n(b.begin(), b.size(), a.begin());
  return std::bit_cast<__m256i>(a);
}

void stored(const std::span<char, 32> o, const __m256i v)
{
  const std::array<char, 32> a = std::bit_cast<std::array<char, 32>>(v);
  std::copy_n(a.begin(), a.size(), o.begin());
}

void stored_word(const std::span<char, 8> o, const uint64_t word)
{
  const std::array<char, 8> a = std::bit_cast<std::array<char, 8>>(word);
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
    got = escaped_h(probe, in);
    if (std::string_view(probe.data(), got) != ref) { state.SkipWithError("the arm gives other bytes"); return; }
  }
  for (auto _ : state) {
    const size_t written = escaped_h(buffer, in);
    benchmark::DoNotOptimize(written);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(in.size()));
}

}

BENCHMARK(arm)->Args({64, 0})->Args({64, 5})->Args({1024, 0})->Args({1024, 5});
BENCHMARK_MAIN();
