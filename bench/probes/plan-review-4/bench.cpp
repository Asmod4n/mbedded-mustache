#include "arms.hpp"
#include <benchmark/benchmark.h>
#include <iterator>
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

size_t escaped_arm(const std::span<char> out, const std::string_view in)
{
#if ARM == 1 || ARM == 2
  return static_cast<size_t>(mustache::escape_into(out.data(), in) - out.data());
#elif ARM == 50 || ARM == 51
  return probe::escaped_h(out, in);
#elif ARM == 60 || ARM == 63
  return probe::escaped_per_block(out, in).value_or(0);
#elif ARM == 61
  return probe::escaped_per_store(out, in).value_or(0);
#elif ARM == 62
  return probe::escaped_checked_once(out, in).value_or(0);
#endif
}

void arm(benchmark::State &state)
{
  const std::string in = input_of(static_cast<size_t>(state.range(0)), static_cast<size_t>(state.range(1)));
  std::vector<char> buffer(in.size() * 6 + mustache::kEscapeSlack);
  {
    std::string ref;
    escaped_by_char(std::back_inserter(ref), in);
    std::vector<char> probe(buffer.size());
    const size_t got = escaped_arm(probe, in);
    if (std::string_view(probe.data(), got) != ref) { state.SkipWithError("the arm gives other bytes"); return; }
  }
  for (auto _ : state) {
    const size_t written = escaped_arm(buffer, in);
    benchmark::DoNotOptimize(written);
    benchmark::ClobberMemory();
  }
  state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(in.size()));
}

}

BENCHMARK(arm)->Args({64, 0})->Args({64, 5})->Args({1024, 0})->Args({1024, 5});
BENCHMARK_MAIN();
