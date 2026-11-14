#include <mustache/mustache.hpp>

#include <benchmark/benchmark.h>

#include "node.hpp"

#include <exception>
#include <print>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

// Workload 2 of bench/render.cpp at ac4de90 (the data of the stocks page in std::unordered_map, from bench/node.hpp), the
// render of a template compiled at run time. One source for every arm. The arm is the build:
//   fresh:  every render goes into a new std::string (form 1, mustache::render)
//   twin:   the same source and flags as fresh, built again (the A/A arm of fresh)
//   reused: every render appends to one std::string that the loop keeps and clears (form 2, mustache::render_to)
// The benchmark of every arm is called stocks, so compare.py pairs the rows.

namespace {

Node
text(const std::string_view s)
{
  return Node{std::string(s)};
}

// mbosecke/template-benchmark, stocks.mustache.html and Stock.java:
// twenty stocks, the row class and the minus class computed the way
// its StockView computes them: the index starts at 1, an even index
// gives "even", and a change that is not above zero gives the minus class.
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
constexpr std::string_view kSource = SOURCE;

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

Node
value_of(const Data &d)
{
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

const Data kData = data();
const Node kRoot = value_of(kData);

mustache::Template
compiled()
{
  std::expected<mustache::Template, mustache::Error> t = mustache::compile(kSource);
  if (!t) std::terminate();
  return std::move(*t);
}

const mustache::Template kTemplate = compiled();

std::string
rendered_into_a_fresh_string()
{
  std::expected<std::string, mustache::Error> r = mustache::render(kTemplate, kRoot);
  if (!r) std::terminate();
  return std::move(*r);
}

void
rendered_into(std::string &out)
{
  if (!mustache::render_to(out, kTemplate, kRoot)) std::terminate();
}

void
stocks(benchmark::State &state)
{
#ifdef ARM_REUSED
  std::string answer;
  for (auto _ : state) {
    answer.clear();
    rendered_into(answer);
    benchmark::DoNotOptimize(answer.data());
    benchmark::ClobberMemory();
  }
#else
  for (auto _ : state) {
    std::string answer = rendered_into_a_fresh_string();
    benchmark::DoNotOptimize(answer.data());
    benchmark::ClobberMemory();
  }
#endif
}

}

BENCHMARK(stocks);

// Both forms must give the same bytes, from a string that already holds a render, before one of them is timed.
// --output_file=PATH writes the bytes of the form that this binary times and ends without timing.
int
main(int argc, char **argv)
{
  const std::string expected = rendered_into_a_fresh_string();
  std::string reused;
  for (int i = 0; i < 3; i++) {
    reused.clear();
    rendered_into(reused);
    if (reused != expected) {
      std::println(stderr, "the forms render different bytes");
      return 1;
    }
  }
  constexpr std::string_view output_flag = "--output_file=";
  for (int i = 1; i < argc; i++) {
    const std::string_view argument = argv[i];
    if (argument.starts_with(output_flag)) {
      std::ofstream out(std::string(argument.substr(output_flag.size())), std::ios::binary);
#ifdef ARM_REUSED
      out << reused;
#else
      out << expected;
#endif
      return out ? 0 : 1;
    }
  }
  std::println(stderr, "output: {} bytes", expected.size());
  benchmark::Initialize(&argc, argv);
  benchmark::RunSpecifiedBenchmarks();
  benchmark::Shutdown();
  return 0;
}
