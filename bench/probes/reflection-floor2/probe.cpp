#include <mustache/mustache.hpp>

#include <benchmark/benchmark.h>

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <exception>
#include <fstream>
#include <print>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

// Probe of the floor of the reflection render on the stocks page of bench/render.cpp, workload 2. One binary per arm; the
// build picks the arm with -DARM=NAME:
//   R   The reflection render of the stocks page into a fresh std::string, as bench/render.cpp does.
//   R2  The same code as R, built as its own binary: the A/A twin of R.
//   S2  A scan of every string value of the data with mustache::copied_string of include/mustache/escape.hpp, the function
//       that the escape of a value calls first: marks16_of for a value of up to 16 bytes, two of them for up to 32 bytes.
//       It writes the copy into a scratch array of 32 bytes. No output is kept. The popcount of its marks is summed and kept.
// Every arm renders once before the timing, and the binary ends if the output is not 7737 bytes or if the count of S2 differs
// from the count of a scalar scan of the data.
// --output_file=PATH writes what the arm produces (the bytes for R and R2, the count for S2) to PATH and ends without timing.

#define ARM_R 1
#define ARM_R2 2
#define ARM_S2 3
#define CAT(a, b) a##b
#define ARM_ID(a) CAT(ARM_, a)
#define THIS_ARM ARM_ID(ARM)
#define STRINGIFY_VALUE(a) STRINGIFY(a)
#define STRINGIFY(a) #a

namespace {

// Stocks page of bench/render.cpp, workload 2, the same template and the same data.
#define SOURCE                                                                                                         \
  "<!DOCTYPE html>\n<html>\n<head>\n\t<title>Stock Prices</title>\n\t<meta http-equiv=\"Content-Type\" "              \
  "content=\"text/html; charset=UTF-8\" />\n\t<meta http-equiv=\"Content-Style-Type\" content=\"text/css\" />\n\t"    \
  "<meta http-equiv=\"Content-Script-Type\" content=\"text/javascript\" />\n\t<link rel=\"shortcut icon\" "          \
  "href=\"/images/favicon.ico\" />\n\t<link rel=\"stylesheet\" type=\"text/css\" href=\"/css/style.css\" "            \
  "media=\"all\" />\n\t<script type=\"text/javascript\" src=\"/js/util.js\"></script>\n\t<style type=\"text/css\">\n" \
  "\t\t/*<![CDATA[*/\n\t\tbody {\n\t\t\tcolor: #333333;\n\t\t\tline-height: 150%;\n\t\t}\n\n\t\tthead {\n\t\t\t"      \
  "font-weight: bold;\n\t\t\tbackground-color: #CCCCCC;\n\t\t}\n\n\t\t.odd {\n\t\t\tbackground-color: #FFCCCC;\n\t\t}" \
  "\n\n\t\t.even {\n\t\t\tbackground-color: #CCCCFF;\n\t\t}\n\n\t\t.minus {\n\t\t\tcolor: #FF0000;\n\t\t}\n\n\t\t"     \
  "/*]]>*/\n\t</style>\n\n</head>\n\n<body>\n\n\t<h1>Stock Prices</h1>\n\n\t<table>\n\t\t<thead>\n    \t\t<tr>\n     " \
  "\t\t\t<th>#</th>\n     \t\t\t<th>symbol</th>\n     \t\t\t<th>name</th>\n     \t\t\t<th>price</th>\n     \t\t\t"    \
  "<th>change</th>\n     \t\t\t<th>ratio</th>\n    \t\t</tr>\n   \t\t</thead>\n   \t\t<tbody>\n\t\t\t{{#items}}\n    " \
  "\t\t<tr class=\"{{rowClass}}\">\n     \t\t\t<td>{{index}}</td>\n    \t\t\t<td>\n      \t\t\t\t<a "                  \
  "href=\"/stocks/{{value.symbol}}\">{{value.symbol}}</a>\n     \t\t\t</td>\n     \t\t\t<td>\n      \t\t\t\t<a "       \
  "href=\"{{value.url}}\">{{value.name}}</a>\n     \t\t\t</td>\n     \t\t\t<td>\n      \t\t\t\t<strong>{{value.price}}" \
  "</strong>\n     \t\t\t</td>\n\t\t\t     <td{{negativeClass}}>{{value.change}}</td>\n\t\t\t     "                     \
  "<td{{negativeClass}}>{{value.ratio}}</td>\n    \t\t</tr>\n\t\t\t{{/items}}\n   \t\t</tbody>\n\t</table>\n\n</body>\n" \
  "</html>\n"

struct Stock {
  std::string symbol;
  std::string url;
  std::string name;
  std::string price;
  std::string change;
  std::string ratio;
};

struct Item {
  std::string rowClass;
  std::string index;
  std::string negativeClass;
  Stock       value;
};

struct Data {
  std::vector<Item> items;
};

Data
data()
{
  const std::vector<Stock> stocks = {
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
  };
  Data d;
  for (size_t i = 0; i < stocks.size(); i++) {
    const bool up = stocks.at(i).change.front() != '-' && stocks.at(i).change != "0";
    d.items.push_back(Item{(i + 1) % 2 == 0 ? "even" : "odd", std::to_string(i + 1), up ? "" : "class=\"minus\"", stocks.at(i)});
  }
  return d;
}


const Data kData = data();

std::string
rendered_by_reflection()
{
  std::expected<std::string, mustache::Error> r = mustache::render<SOURCE>(kData);
  if (!r) std::terminate();
  return std::move(*r);
}

constexpr size_t kExpectedSize = 7737;
constexpr std::string_view kHtmlMarks = "&<>\"'";

const std::string      kExpected = rendered_by_reflection();
const std::string_view kExpectedView = kExpected;

size_t
marks_in(const std::string_view value)
{
  return std::ranges::count_if(value, [](const char c) { return kHtmlMarks.contains(c); });
}

size_t
marks_in_data(const Data &d)
{
  size_t count = 0;
  for (const Item &item : d.items) {
    count += marks_in(item.rowClass) + marks_in(item.index) + marks_in(item.negativeClass);
    count += marks_in(item.value.symbol) + marks_in(item.value.url) + marks_in(item.value.name);
    count += marks_in(item.value.price) + marks_in(item.value.change) + marks_in(item.value.ratio);
  }
  return count;
}

size_t
marks_in_data_by_library(const Data &d)
{
  std::array<char, mustache::kCopiedStringMax> scratch;
  size_t count = 0;
  const auto add = [&](const std::string &value) { count += std::popcount(mustache::copied_string(scratch, value).marks); };
  for (const Item &item : d.items) {
    add(item.rowClass);
    add(item.index);
    add(item.negativeClass);
    add(item.value.symbol);
    add(item.value.url);
    add(item.value.name);
    add(item.value.price);
    add(item.value.change);
    add(item.value.ratio);
  }
  return count;
}

void
work(benchmark::State &state)
{
  for (auto _ : state) {
#if THIS_ARM == ARM_R || THIS_ARM == ARM_R2
    std::string answer = rendered_by_reflection();
    benchmark::DoNotOptimize(answer.data());
    benchmark::ClobberMemory();
#elif THIS_ARM == ARM_S2
    size_t count = marks_in_data_by_library(kData);
    benchmark::DoNotOptimize(count);
    benchmark::ClobberMemory();
#endif
  }
}

}

BENCHMARK(work)->Name(STRINGIFY_VALUE(ARM));

int
main(int argc, char **argv)
{
  if (kExpected.size() != kExpectedSize) {
    std::println(stderr, "the output has {} bytes, not {}", kExpected.size(), kExpectedSize);
    return 1;
  }
  if (marks_in_data_by_library(kData) != marks_in_data(kData)) {
    std::println(stderr, "the library scan counts {} marks, the scalar scan {}", marks_in_data_by_library(kData), marks_in_data(kData));
    return 1;
  }
  constexpr std::string_view output_flag = "--output_file=";
  for (int i = 1; i < argc; i++) {
    const std::string_view argument = argv[i];
    if (argument.starts_with(output_flag)) {
      std::ofstream out(std::string(argument.substr(output_flag.size())), std::ios::binary);
#if THIS_ARM == ARM_R || THIS_ARM == ARM_R2
      out << rendered_by_reflection();
#elif THIS_ARM == ARM_S2
      out << marks_in_data_by_library(kData) << '\n';
#endif
      return out ? 0 : 1;
    }
  }
  std::println(stderr, "output: {} bytes, marks in the values: {} (library scan {})", kExpected.size(), marks_in_data(kData), marks_in_data_by_library(kData));
  benchmark::Initialize(&argc, argv);
  benchmark::RunSpecifiedBenchmarks();
  benchmark::Shutdown();
  return 0;
}
