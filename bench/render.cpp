#include <mustache/mustache.hpp>

#include <benchmark/benchmark.h>

#include "../test/node.hpp"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

// One binary answers one question for one workload: how long a render
// takes for a template compiled at run time on standard types and,
// where the compiler has reflection, through the reflection render on
// structs. The build picks the
// workload with -DWORKLOAD=1, 2 or 3.
//
// Every arm renders into a fresh std::string per render, so every arm
// pays the same allocation.

namespace {

Node
text(const std::string_view s)
{
  return Node{std::string(s)};
}

#if WORKLOAD == 1

// ramhorns, tests/benches/main.rs, the "simple" benchmark of its README.
constexpr std::string_view kSource = "<title>{{title}}</title><h1>{{ title }}</h1><div>{{{body}}}</div>";
#define SOURCE "<title>{{title}}</title><h1>{{ title }}</h1><div>{{{body}}}</div>"

struct Data {
  std::string title;
  std::string body;
};

Data
data()
{
  return {"Hello, Ramhorns!", "This is a really simple test of the rendering!"};
}

Node
value_of(const Data &d)
{
  NodeMap m;
  m.insert_or_assign("title", text(d.title));
  m.insert_or_assign("body", text(d.body));
  return Node{std::move(m)};
}

#elif WORKLOAD == 2

// mbosecke/template-benchmark, stocks.mustache.html and Stock.java:
// twenty stocks, the row class and the minus class computed the way
// its StockView computes them.
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
    d.items.push_back(Item{i % 2 == 0 ? "even" : "odd", std::to_string(i + 1), up ? "" : "class=\"minus\"", stocks.at(i)});
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

#elif WORKLOAD == 3

// A news page of this tree: twenty articles with three tags each. It
// has no outside source.
#define SOURCE                                                                                                         \
  "<!DOCTYPE html>\n<html>\n<head><title>{{title}}</title></head>\n<body>\n<h1>{{title}}</h1>\n{{#articles}}\n"      \
  "<article>\n  <h2><a href=\"{{url}}\">{{title}}</a></h2>\n  <p class=\"meta\">{{author}} - {{date}}</p>\n  "        \
  "<p>{{teaser}}</p>\n  <ul>{{#tags}}<li>{{name}}</li>{{/tags}}</ul>\n</article>\n{{/articles}}\n</body>\n</html>\n"
constexpr std::string_view kSource = SOURCE;

struct Tag {
  std::string name;
};

struct Article {
  std::string      title;
  std::string      url;
  std::string      author;
  std::string      date;
  std::string      teaser;
  std::vector<Tag> tags;
};

struct Data {
  std::string          title;
  std::vector<Article> articles;
};

Data
data()
{
  Data d{"Today's news", {}};
  for (int i = 0; i < 20; i++) {
    const std::string n = std::to_string(i + 1);
    d.articles.push_back(Article{"Article " + n + ": what happened & why",
                                 "https://news.example/articles/" + n,
                                 "Author " + n,
                                 "2026-09-" + std::string(i < 9 ? "0" : "") + n,
                                 "A teaser of about one line for article " + n + ", with <b>markup</b> to escape.",
                                 {{"world"}, {"tech"}, {"tag-" + n}}});
  }
  return d;
}

Node
value_of(const Data &d)
{
  NodeList articles;
  for (const Article &a : d.articles) {
    NodeList tags;
    for (const Tag &t : a.tags) {
      NodeMap tag;
      tag.insert_or_assign("name", text(t.name));
      tags.push_back(Node{std::move(tag)});
    }
    NodeMap m;
    m.insert_or_assign("title", text(a.title));
    m.insert_or_assign("url", text(a.url));
    m.insert_or_assign("author", text(a.author));
    m.insert_or_assign("date", text(a.date));
    m.insert_or_assign("teaser", text(a.teaser));
    m.insert_or_assign("tags", Node{std::move(tags)});
    articles.push_back(Node{std::move(m)});
  }
  NodeMap m;
  m.insert_or_assign("title", text(d.title));
  m.insert_or_assign("articles", Node{std::move(articles)});
  return Node{std::move(m)};
}

#endif

const Data kData = data();
const Node kRoot = value_of(kData);

mustache::Template
compiled()
{
  std::expected<mustache::Template, mustache::Error> t = mustache::compile(kSource);
  if (!t) std::abort();
  return std::move(*t);
}

const mustache::Template kTemplate = compiled();

std::string
rendered_at_run_time()
{
  std::expected<std::string, mustache::Error> r = mustache::render(kTemplate, kRoot);
  if (!r) std::abort();
  return std::move(*r);
}

#ifdef __cpp_impl_reflection
std::string
rendered_by_reflection()
{
  std::expected<std::string, mustache::Error> r = mustache::render<SOURCE>(kData);
  if (!r) std::abort();
  return std::move(*r);
}
#endif

void
run_time(benchmark::State &state)
{
  for (auto _ : state) {
    std::string answer = rendered_at_run_time();
    benchmark::DoNotOptimize(answer.data());
    benchmark::ClobberMemory();
  }
}

#ifdef __cpp_impl_reflection
void
reflection(benchmark::State &state)
{
  for (auto _ : state) {
    std::string answer = rendered_by_reflection();
    benchmark::DoNotOptimize(answer.data());
    benchmark::ClobberMemory();
  }
}
#endif

}

BENCHMARK(run_time);
#ifdef __cpp_impl_reflection
BENCHMARK(reflection);
#endif

// Every arm must give the same bytes before one of them is timed.
int
main(int argc, char **argv)
{
  const std::string expected = rendered_at_run_time();
#ifdef __cpp_impl_reflection
  if (rendered_by_reflection() != expected) {
    std::fprintf(stderr, "the arms render different bytes\n");
    return 1;
  }
#endif
  std::fprintf(stderr, "output: %zu bytes\n", expected.size());
  benchmark::Initialize(&argc, argv);
  benchmark::RunSpecifiedBenchmarks();
  benchmark::Shutdown();
  return 0;
}
