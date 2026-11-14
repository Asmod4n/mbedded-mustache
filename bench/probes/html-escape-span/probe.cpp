// Probe: the html escape of include/mustache/escape.hpp on the escape alone, before and after it takes a proven
// std::span<char> and returns the count. One binary times one arm, chosen with -DARM=<number>; each arm is held
// to the arm "before" first. The headers of 738ff29 stand in /w/old/include.
// Arms: 0 and 1 escape_into(char *, std::string_view) of 738ff29 as it stands (the same code twice, for the
// noise); 2 and 3 escape_into(std::span<char>, std::string_view) of escape_span.hpp, an inline function of a
// header in namespace mustache; 2 has [[gnu::always_inline, gnu::flatten]] on the 32-byte helpers, 3 has
// [[gnu::flatten]] on escape_into and no attribute on the 32-byte helpers; 4 is 3 with the scalar parts over a
// std::span that each stored word advances.
// Each arm writes into a room of the exact escaped size + kBytesPastTheWrittenCount, as the callers in out.hpp
// prove with room_holds_the_escaped_value.
#include "escape_span.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <print>
#include <random>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#ifndef ARM
#error "ARM names the arm"
#endif

namespace probe {

[[gnu::always_inline]] inline size_t
before(const std::span<char> room, const std::string_view s)
{
  char *const w = room.data();
  return static_cast<size_t>(std::distance(w, mustache::escape_into(w, s)));
}

[[gnu::always_inline]] inline size_t
escaped(const std::span<char> room, const std::string_view s)
{
#if ARM == 0 || ARM == 1
  return before(room, s);
#else
  return mustache::escape_into(room, s);
#endif
}

inline constexpr size_t kLengthMax = 1024;
inline constexpr size_t kRoomMax = mustache::kEntityMax * kLengthMax + mustache::kBytesPastTheWrittenCount;
inline constexpr size_t kBufferBytes = 1 << 16;
inline constexpr size_t kInputBytesPerRepetition = 1 << 22;
inline constexpr int kRepetitions = 5;
inline constexpr size_t kValuesPerInput = 64;
inline constexpr uint32_t kSeed = 20261114;
inline constexpr char kGuard = '\x5a';
inline constexpr std::string_view kMarks = "&<>\"'";

// The room of the rule for each value: the exact size plus the bytes past the count. The bytes of the arm must
// equal the bytes of the arm "before", and no byte behind the room may change.
bool
held_to_before(const std::string_view s)
{
  std::vector<char> b(kRoomMax + kRoomMax, kGuard);
  const size_t exact = before(b, s);
  const size_t room = exact + mustache::kBytesPastTheWrittenCount;
  std::vector<char> a(room + mustache::kBytesPastTheWrittenCount, kGuard);
  const size_t n = escaped(std::span<char>(a).first(room), s);
  const bool same = n == exact && std::ranges::equal(std::span(a).first(n), std::span(b).first(exact)) &&
                    std::ranges::all_of(std::span(a).subspan(room), [](const char x) { return x == kGuard; });
  if (!same) std::println(stderr, "arm {} differs from the arm before at a value of {} bytes", ARM, s.size());
  return same;
}

bool
every_case_held()
{
  const size_t v = mustache::kVectorBytes;
  bool all = held_to_before("");
  for (int b = 0; b < 256; b++) {
    const char x = static_cast<char>(static_cast<unsigned char>(b));
    all = held_to_before(std::string(1, x)) && all;
    for (size_t at = 0; at < v; at++) {
      std::string s(v, 'x');
      s.replace(at, 1, 1, x);
      all = held_to_before(s) && all;
    }
  }
  for (size_t n = 0; n <= 3 * v + 1; n++) {
    all = held_to_before(std::string(n, 'x')) && all;
    for (size_t at = 0; at < n; at++) {
      for (const char m : kMarks) {
        std::string s(n, 'x');
        s.replace(at, 1, 1, m);
        all = held_to_before(s) && all;
      }
    }
  }
  for (const size_t n : {v, 2 * v - 1, 2 * v, 2 * v + 1, 3 * v}) {
    for (size_t block = 0; block + v <= n; block += v) {
      for (size_t marks = 0; marks <= v; marks++) {
        for (const bool at_the_start : {true, false}) {
          std::string mixed(n, 'x');
          const size_t first = at_the_start ? block : block + v - marks;
          for (size_t i = 0; i < marks; i++) mixed.replace(first + i, 1, 1, kMarks.at(i % kMarks.size()));
          all = held_to_before(mixed) && all;
        }
      }
    }
  }
  return all;
}

// kValuesPerInput values of n bytes: plain letters, digits and a space; each byte is one of the five marks of
// html with the probability percent / 100.
std::vector<std::string>
values_of(const size_t n, const uint32_t percent)
{
  constexpr std::string_view kPlain = "abcdefghijklmnopqrstuvwxyz0123456789 ";
  std::mt19937 random(kSeed + static_cast<uint32_t>(n) + percent);
  std::uniform_int_distribution<uint32_t> hundred(0, 99);
  std::uniform_int_distribution<size_t> plain(0, kPlain.size() - 1);
  std::uniform_int_distribution<size_t> mark(0, kMarks.size() - 1);
  std::vector<std::string> v;
  for (size_t k = 0; k < kValuesPerInput; k++) {
    std::string s;
    for (size_t i = 0; i < n; i++) s += hundred(random) < percent ? kMarks.at(mark(random)) : kPlain.at(plain(random));
    v.push_back(s);
  }
  return v;
}

// The escaped values of workload 2 of bench/render.cpp in the order of the render.
std::vector<std::string>
values_of_workload_2()
{
  struct Stock {
    std::string_view symbol, url, name, price, change, ratio;
  };
  const std::array<Stock, 20> stocks = {{
      {"ADBE", "http://www.adobe.com", "Adobe Systems", "39.26", "0.13", "0.33"},
      {"AMD", "http://www.amd.com", "Advanced Micro Devices", "16.22", "0.17", "1.06"},
      {"AMZN", "http://www.amazon.com", "Amazon.com", "36.85", "-0.23", "-0.62"},
      {"AAPL", "http://www.apple.com", "Apple", "85.38", "-0.87", "-1.01"},
      {"BEAS", "http://www.bea.com", "BEA Systems", "12.46", "0.09", "0.73"},
      {"CA", "http://www.ca.com", "CA", "24.66", "0.38", "1.57"},
      {"CSCO", "http://www.cisco.com", "Cisco Systems", "26.35", "0.13", "0.5"},
      {"DELL", "http://www.dell.com/", "Dell", "23.73", "-0.42", "-1.74"},
      {"EBAY", "http://www.ebay.com", "eBay", "31.65", "-0.8", "-2.47"},
      {"GOOG", "http://www.google.com", "Google", "495.84", "7.75", "1.59"},
      {"HPQ", "http://www.hp.com", "Hewlett-Packard", "41.69", "-0.02", "-0.05"},
      {"IBM", "http://www.ibm.com", "IBM", "97.45", "-0.06", "-0.06"},
      {"INTC", "http://www.intel.com", "Intel", "20.53", "-0.07", "-0.34"},
      {"JNPR", "http://www.juniper.net/", "Juniper Networks", "18.96", "0.5", "2.71"},
      {"MSFT", "http://www.microsoft.com", "Microsoft", "30.6", "0.15", "0.49"},
      {"ORCL", "http://www.oracle.com", "Oracle", "17.15", "0.17", "1.1"},
      {"SAP", "http://www.sap.com", "SAP", "46.2", "-0.16", "-0.35"},
      {"STX", "http://www.seagate.com/", "Seagate Technology", "27.35", "-0.36", "-1.3"},
      {"SUNW", "http://www.sun.com", "Sun Microsystems", "6.33", "-0.01", "-0.16"},
      {"YHOO", "http://www.yahoo.com", "Yahoo", "28.04", "-0.17", "-0.6"},
  }};
  std::vector<std::string> v;
  for (size_t i = 0; i < stocks.size(); i++) {
    const Stock &s = stocks.at(i);
    const bool up = s.change.front() != '-' && s.change != "0";
    const std::string_view negative = up ? "" : "class=\"minus\"";
    const std::string index = std::to_string(i + 1);
    for (const std::string_view value : {std::string_view(i % 2 == 0 ? "even" : "odd"), std::string_view(index),
                                         s.symbol, s.symbol, s.url, s.name, s.price, negative, s.change, negative, s.ratio})
      v.emplace_back(value);
  }
  return v;
}

// The escaped values of workload 3 of bench/render.cpp in the order of the render.
std::vector<std::string>
values_of_workload_3()
{
  std::vector<std::string> v = {"Today's news", "Today's news"};
  for (int i = 0; i < 20; i++) {
    const std::string n = std::to_string(i + 1);
    for (const std::string &value : {"https://news.example/articles/" + n, "Article " + n + ": what happened & why", "Author " + n,
                                     "2026-09-" + std::string(i < 9 ? "0" : "") + n,
                                     "A teaser of about one line for article " + n + ", with <b>markup</b> to escape.",
                                     std::string("world"), std::string("tech"), "tag-" + n})
      v.push_back(value);
  }
  return v;
}

struct Input {
  std::string              name;
  std::vector<std::string> values;
};

std::vector<Input>
inputs()
{
  std::vector<Input> in;
  for (const size_t n : {size_t{16}, size_t{31}, size_t{32}, size_t{33}, size_t{64}, size_t{1024}}) {
    for (const uint32_t p : {0u, 5u}) in.push_back({std::to_string(n) + "-" + std::to_string(p), values_of(n, p)});
  }
  in.push_back({"workload-2", values_of_workload_2()});
  in.push_back({"workload-3", values_of_workload_3()});
  return in;
}

size_t
bytes_of(const std::vector<std::string> &values)
{
  size_t n = 0;
  for (const std::string &s : values) n += s.size();
  return n;
}

[[gnu::noinline]] size_t
escaped_values(const std::span<char> buffer, const std::vector<std::string> &values, const size_t passes)
{
  size_t sum = 0;
  size_t at = 0;
  for (size_t p = 0; p < passes; p++) {
    for (const std::string &s : values) {
      const size_t n = escaped(buffer.subspan(at), s);
      at += n;
      sum += n;
      if (at > buffer.size() - kRoomMax) at = 0;
    }
  }
  return sum;
}

}

int
main(const int argc, const char *const *const argv)
{
  using namespace probe;
  if (!every_case_held()) return 1;
  const std::vector<Input> in = inputs();
  for (const Input &i : in) {
    for (const std::string &s : i.values) {
      if (!held_to_before(s)) return 1;
    }
  }
  const std::string_view label = argc > 2 ? argv[2] : "";
  const std::string_view round = argc > 1 ? argv[1] : "0";
  std::vector<char> buffer(kBufferBytes);
  for (const Input &i : in) {
    const size_t passes = std::max(size_t{1}, kInputBytesPerRepetition / bytes_of(i.values));
    for (int r = 0; r < kRepetitions; r++) {
      const auto start = std::chrono::steady_clock::now();
      const size_t sum = escaped_values(buffer, i.values, passes);
      const std::chrono::duration<double, std::nano> took = std::chrono::steady_clock::now() - start;
      std::println("round={} {} input={} ns_per_value={:.4f}", round, label, i.name,
                   took.count() / static_cast<double>(passes * i.values.size()));
      std::println(stderr, "sum {}", sum);
    }
  }
  return 0;
}
