#include <benchmark/benchmark.h>
#include <cstdint>
static void alu(benchmark::State &st)
{
  uint64_t x = 88172645463325252ull;
  for (auto _ : st) {
    for (int i = 0; i < 700; i++) { x ^= x << 13; x ^= x >> 7; x ^= x << 17; }
    benchmark::DoNotOptimize(x);
  }
}
BENCHMARK(alu);
BENCHMARK_MAIN();
