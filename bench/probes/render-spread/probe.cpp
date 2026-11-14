#include <mustache/mustache.hpp>

#include <benchmark/benchmark.h>

#include "../../node.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <memory>
#include <memory_resource>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

// Probe of the spread of the run-time render on the stocks page. One
// binary per arm; the build picks the arm with -DARM=NAME:
//   heap   Node data as bench/render.cpp builds it, initial_capacity 1024.
//   cap    The same data, initial_capacity 8192, so that the output
//          needs no growth on the stocks page.
//   warm   The data built after 3000 allocations of other sizes, of
//          which every second one is freed, so the nodes lie elsewhere.
//   pmr    The data built from std::pmr containers in one
//          monotonic_buffer_resource, so the nodes lie in one block.
//   typed  The struct data rendered through compile<Data>, the data
//          model of ramhorns.
// Before the timing, the binary writes to stderr the addresses of a
// stack local, a fresh malloc block, the data and the output, so that
// the time of a process can be related to its layout.

#define ARM_heap 1
#define ARM_cap 2
#define ARM_warm 3
#define ARM_pmr 4
#define ARM_typed 5
#define CAT(a, b) a##b
#define ARM_ID(a) CAT(ARM_, a)
#define THIS_ARM ARM_ID(ARM)

namespace {

constexpr std::string_view kSource =
    "<!DOCTYPE html>\n<html>\n<head>\n\t<title>Stock Prices</title>\n\t<meta http-equiv=\"Content-Type\" "
    "content=\"text/html; charset=UTF-8\" />\n\t<meta http-equiv=\"Content-Style-Type\" content=\"text/css\" />\n\t"
    "<meta http-equiv=\"Content-Script-Type\" content=\"text/javascript\" />\n\t<link rel=\"shortcut icon\" "
    "href=\"/images/favicon.ico\" />\n\t<link rel=\"stylesheet\" type=\"text/css\" href=\"/css/style.css\" "
    "media=\"all\" />\n\t<script type=\"text/javascript\" src=\"/js/util.js\"></script>\n\t<style type=\"text/css\">\n"
    "\t\t/*<![CDATA[*/\n\t\tbody {\n\t\t\tcolor: #333333;\n\t\t\tline-height: 150%;\n\t\t}\n\n\t\tthead {\n\t\t\t"
    "font-weight: bold;\n\t\t\tbackground-color: #CCCCCC;\n\t\t}\n\n\t\t.odd {\n\t\t\tbackground-color: #FFCCCC;\n\t\t}"
    "\n\n\t\t.even {\n\t\t\tbackground-color: #CCCCFF;\n\t\t}\n\n\t\t.minus {\n\t\t\tcolor: #FF0000;\n\t\t}\n\n\t\t"
    "/*]]>*/\n\t</style>\n\n</head>\n\n<body>\n\n\t<h1>Stock Prices</h1>\n\n\t<table>\n\t\t<thead>\n    \t\t<tr>\n     "
    "\t\t\t<th>#</th>\n     \t\t\t<th>symbol</th>\n     \t\t\t<th>name</th>\n     \t\t\t<th>price</th>\n     \t\t\t"
    "<th>change</th>\n     \t\t\t<th>ratio</th>\n    \t\t</tr>\n   \t\t</thead>\n   \t\t<tbody>\n\t\t\t{{#items}}\n    "
    "\t\t<tr class=\"{{rowClass}}\">\n     \t\t\t<td>{{index}}</td>\n    \t\t\t<td>\n      \t\t\t\t<a "
    "href=\"/stocks/{{value.symbol}}\">{{value.symbol}}</a>\n     \t\t\t</td>\n     \t\t\t<td>\n      \t\t\t\t<a "
    "href=\"{{value.url}}\">{{value.name}}</a>\n     \t\t\t</td>\n     \t\t\t<td>\n      \t\t\t\t<strong>{{value.price}}"
    "</strong>\n     \t\t\t</td>\n\t\t\t     <td{{negativeClass}}>{{value.change}}</td>\n\t\t\t     "
    "<td{{negativeClass}}>{{value.ratio}}</td>\n    \t\t</tr>\n\t\t\t{{/items}}\n   \t\t</tbody>\n\t</table>\n\n</body>\n"
    "</html>\n";

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

#if THIS_ARM == ARM_cap
constexpr size_t kCapacity = 8192;
#else
constexpr size_t kCapacity = mustache::kInitialCapacity;
#endif

#if THIS_ARM == ARM_pmr

struct TransparentHash {
  using is_transparent = void;
  size_t operator()(const std::string_view s) const { return std::hash<std::string_view>{}(s); }
};

struct TransparentEqual {
  using is_transparent = void;
  bool operator()(const std::string_view a, const std::string_view b) const { return a == b; }
};

struct PNode : std::variant<std::nullptr_t, std::pmr::string, std::pmr::vector<PNode>,
                            std::pmr::unordered_map<std::pmr::string, PNode, TransparentHash, TransparentEqual>> {
  using variant::variant;
};

using PMap = std::pmr::unordered_map<std::pmr::string, PNode, TransparentHash, TransparentEqual>;
using Root = PNode;

constexpr size_t kArenaBytes = 1 << 20;

std::pmr::monotonic_buffer_resource &
arena()
{
  static std::array<std::byte, kArenaBytes> bytes;
  static std::pmr::monotonic_buffer_resource r(bytes.data(), bytes.size(), std::pmr::null_memory_resource());
  return r;
}

PNode
text(const std::string_view s)
{
  return PNode{std::pmr::string(s)};
}

Root
root_of(const Data &d)
{
  std::pmr::memory_resource *const old = std::pmr::set_default_resource(&arena());
  std::pmr::vector<PNode> items;
  for (const Item &it : d.items) {
    PMap stock;
    stock.insert_or_assign("symbol", text(it.value.symbol));
    stock.insert_or_assign("url", text(it.value.url));
    stock.insert_or_assign("name", text(it.value.name));
    stock.insert_or_assign("price", text(it.value.price));
    stock.insert_or_assign("change", text(it.value.change));
    stock.insert_or_assign("ratio", text(it.value.ratio));
    PMap item;
    item.insert_or_assign("rowClass", text(it.rowClass));
    item.insert_or_assign("index", text(it.index));
    item.insert_or_assign("negativeClass", text(it.negativeClass));
    item.insert_or_assign("value", PNode{std::move(stock)});
    items.push_back(PNode{std::move(item)});
  }
  PMap m;
  m.insert_or_assign("items", PNode{std::move(items)});
  PNode r{std::move(m)};
  std::pmr::set_default_resource(old);
  return r;
}

const void *
data_address(const Root &r)
{
  return std::get<std::pmr::vector<PNode>>(std::get<PMap>(r).find(std::string_view("items"))->second).data();
}

#elif THIS_ARM != ARM_typed

using Root = Node;

Node
text(const std::string_view s)
{
  return Node{std::string(s)};
}

#if THIS_ARM == ARM_warm
constexpr int kScatterBlocks = 3000;
constexpr size_t kScatterSizeStep = 37;
constexpr size_t kScatterSizeRange = 480;
constexpr size_t kScatterSizeMin = 8;
#endif

#if THIS_ARM == ARM_warm
std::vector<std::unique_ptr<char[]>> &
scattered_blocks()
{
  static std::vector<std::unique_ptr<char[]>> blocks;
  return blocks;
}

void
scatter_the_heap()
{
  std::vector<std::unique_ptr<char[]>> &blocks = scattered_blocks();
  for (int i = 0; i < kScatterBlocks; i++) {
    const size_t n = (size_t)i * kScatterSizeStep % kScatterSizeRange + kScatterSizeMin;
    blocks.push_back(std::make_unique<char[]>(n));
    benchmark::DoNotOptimize(blocks.back().get());
  }
  for (size_t i = 0; i < blocks.size(); i += 2) blocks.at(i).reset();
}
#endif

Root
root_of(const Data &d)
{
#if THIS_ARM == ARM_warm
  scatter_the_heap();
#endif
  NodeList items;
  for (const Item &it : d.items) {
    NodeMap stock;
    stock.insert_or_assign("symbol", text(it.value.symbol));
    stock.insert_or_assign("url", text(it.value.url));
    stock.insert_or_assign("name", text(it.value.name));
    stock.insert_or_assign("price", text(it.value.price));
    stock.insert_or_assign("change", text(it.value.change));
    stock.insert_or_assign("ratio", text(it.value.ratio));
    NodeMap item;
    item.insert_or_assign("rowClass", text(it.rowClass));
    item.insert_or_assign("index", text(it.index));
    item.insert_or_assign("negativeClass", text(it.negativeClass));
    item.insert_or_assign("value", Node{std::move(stock)});
    items.push_back(Node{std::move(item)});
  }
  NodeMap m;
  m.insert_or_assign("items", Node{std::move(items)});
  return Node{std::move(m)};
}

const void *
data_address(const Root &r)
{
  return std::get<NodeList>(std::get<NodeMap>(r).at("items")).data();
}

#endif

#if THIS_ARM == ARM_typed

mustache::TypedTemplate<Data>
compiled()
{
  std::expected<mustache::TypedTemplate<Data>, mustache::Error> t = mustache::compile<Data>(kSource);
  if (!t) std::abort();
  return std::move(*t);
}

const mustache::TypedTemplate<Data> kTemplate = compiled();

std::string
rendered()
{
  std::expected<std::string, mustache::Error> r = mustache::render(kTemplate, kData, {}, kCapacity);
  if (!r) std::abort();
  return std::move(*r);
}

const void *
data_address()
{
  return kData.items.data();
}

#else

const Root kRoot = root_of(kData);

mustache::Template
compiled()
{
  std::expected<mustache::Template, mustache::Error> t = mustache::compile(kSource);
  if (!t) std::abort();
  return std::move(*t);
}

const mustache::Template kTemplate = compiled();

std::string
rendered()
{
  std::expected<std::string, mustache::Error> r = mustache::render(kTemplate, kRoot, {}, kCapacity);
  if (!r) std::abort();
  return std::move(*r);
}

const void *
data_address()
{
  return data_address(kRoot);
}

#endif

void
run_time(benchmark::State &state)
{
  for (auto _ : state) {
    std::string answer = rendered();
    benchmark::DoNotOptimize(answer.data());
    benchmark::ClobberMemory();
  }
}

}

BENCHMARK(run_time);

// --output_file=PATH writes the render to PATH and ends without timing.
int
main(int argc, char **argv)
{
  const std::string expected = rendered();
  constexpr std::string_view output_flag = "--output_file=";
  for (int i = 1; i < argc; i++) {
    const std::string_view argument = argv[i];
    if (argument.starts_with(output_flag)) {
      std::ofstream out(std::string(argument.substr(output_flag.size())), std::ios::binary);
      out << expected;
      return out ? 0 : 1;
    }
  }
  const int stack_local = 0;
  const std::unique_ptr<char[]> fresh = std::make_unique<char[]>(64);
  benchmark::DoNotOptimize(fresh.get());
  std::fprintf(stderr, "output_bytes=%zu output_capacity=%zu\n", expected.size(), expected.capacity());
  std::fprintf(stderr, "address stack=%p malloc64=%p data=%p output=%p text=%p\n", (const void *)&stack_local,
               (const void *)fresh.get(), data_address(), (const void *)expected.data(), (const void *)&rendered);
  benchmark::Initialize(&argc, argv);
  benchmark::RunSpecifiedBenchmarks();
  benchmark::Shutdown();
  return 0;
}
