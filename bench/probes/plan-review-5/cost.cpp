// First call with a good hint: zero fill (today, Answer::opened and closed) against resize_and_overwrite.
// ARM 0 and 1: zero fill (the same code twice, for the noise). ARM 2: resize_and_overwrite.
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <span>
#include <string>
static const size_t kHint = 1024 + 32;
static const size_t kRuns = 4000000;
[[gnu::noinline]] static size_t written(const std::span<char> room, const std::string &s)
{
  std::copy_n(s.begin(), s.size(), room.begin());
  return s.size();
}
[[gnu::noinline]] static std::string rendered(const std::string &text)
{
  std::string bytes;
#if ARM == 2
  bytes.resize_and_overwrite(kHint, [&](char *const p, const size_t n) { return written(std::span<char>(p, n), text); });
#else
  bytes.resize(kHint);
  bytes.resize(written(bytes, text));
#endif
  return bytes;
}
int main()
{
  const std::string text(200, 'x');
  for (int repetition = 0; repetition < 3; repetition++) {
    size_t sum = 0;
    const auto start = std::chrono::steady_clock::now();
    for (size_t i = 0; i < kRuns; i++) sum += rendered(text).size();
    const std::chrono::duration<double, std::nano> took = std::chrono::steady_clock::now() - start;
    printf("%.2f %zu\n", took.count() / kRuns, sum / kRuns);
  }
}
