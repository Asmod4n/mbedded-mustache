// Registers one Google Benchmark case for one engine and writes its output to a file on request.
#pragma once

#include <benchmark/benchmark.h>

#include <fstream>
#include <string>
#include <string_view>

template <class Render>
int
run_engine(int argc, char **argv, Render render)
{
  constexpr std::string_view output_flag = "--output_file=";
  for (int i = 1; i < argc; i++) {
    const std::string_view argument = argv[i];
    if (argument.starts_with(output_flag)) {
      std::ofstream out(std::string(argument.substr(output_flag.size())), std::ios::binary);
      out << render();
      return out ? 0 : 1;
    }
  }
  benchmark::RegisterBenchmark("render", [&render](benchmark::State &state) {
    for (auto _ : state) {
      std::string answer = render();
      benchmark::DoNotOptimize(answer.data());
      benchmark::ClobberMemory();
    }
  });
  benchmark::Initialize(&argc, argv);
  benchmark::RunSpecifiedBenchmarks();
  benchmark::Shutdown();
  return 0;
}
