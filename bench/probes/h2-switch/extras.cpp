// The extra inputs of the byte check of the h2-switch probe: the cases that the stocks page does not reach, among them the
// kinds of a Node, a variant that holds an optional, and an optional that holds a variant.
// One source for every arm. The program writes the renders of every case to the file of --output_file=PATH, one section for each
// case and each render path, and ends. A section is a run-time render of a template compiled at run time and, where the compiler
// has reflection, a render through the reflection render.
#include <mustache/mustache.hpp>

#include "node.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <flat_map>
#include <fstream>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace {

std::string output;

void
section(const std::string_view title, const std::string_view bytes)
{
  output += "== ";
  output += title;
  output += " (";
  output += std::to_string(bytes.size());
  output += " bytes)\n";
  output += bytes;
  output += "\n";
}

std::string
text_of(const std::expected<std::string, mustache::Error> &r)
{
  if (r) return *r;
  return "ERROR " + std::to_string((int)r.error().fault) + " " + std::string(r.error().what);
}

template <class Data>
void
at_run_time(const std::string_view title, const std::string_view source, const Data &data,
            const mustache::Partials &partials = {})
{
  const std::expected<mustache::Template, mustache::Error> t = mustache::compile(source);
  if (!t) {
    section(title, "COMPILE ERROR");
    return;
  }
  section(title, text_of(mustache::render(*t, data, partials)));
}

Node
text(const std::string_view s)
{
  return Node{std::string(s)};
}

Node
map_of(std::initializer_list<std::pair<const char *, Node>> pairs)
{
  NodeMap m;
  for (const auto &[k, v] : pairs) m.insert_or_assign(k, v);
  return Node{std::move(m)};
}

// A value that the batch path of the reflection render handles: no mark, a mark that escapes to at most 32 bytes, a mark that
// escapes to more than 32 bytes, a value of 33 bytes and more, and the sizes around 16 and 32.
std::vector<std::string>
values()
{
  std::vector<std::string> v = {"",
                                "a",
                                "&",
                                "<>",
                                std::string(15, 'x'),
                                std::string(16, 'x'),
                                std::string(17, 'x'),
                                std::string(31, 'x'),
                                std::string(32, 'x'),
                                std::string(33, 'x'),
                                std::string(15, 'x') + "&",
                                std::string(16, 'x') + "<",
                                std::string(26, 'x') + ">",
                                std::string(27, 'x') + "&",
                                std::string(28, 'x') + "&",
                                std::string(26, 'x') + "<>",
                                std::string(5, '"'),
                                std::string(6, '"'),
                                std::string(32, '"'),
                                std::string(7, '\''),
                                std::string(6, '\''),
                                "class=\"minus\"",
                                std::string(100, 'x') + "<&>\"'",
                                std::string(33, '&'),
                                "a&b<c>d\"e'f&g<h>i\"j'k&l<m>n\"o'p&q<r>s\"t'u"};
  return v;
}

struct Row1 {
  std::string name;
  std::string note;
};

struct Data1 {
  std::vector<Row1> rows;
};

#define SOURCE1 "{{#rows}}<td>{{name}}</td><td>{{note}}</td>\n{{/rows}}"

Data1
data1()
{
  Data1 d;
  const std::vector<std::string> v = values();
  for (size_t i = 0; i < v.size(); i++) d.rows.push_back(Row1{v.at(i), v.at((i * 7 + 3) % v.size())});
  return d;
}

Node
node1(const Data1 &d)
{
  NodeList rows;
  for (const Row1 &r : d.rows) rows.push_back(map_of({{"name", text(r.name)}, {"note", text(r.note)}}));
  return map_of({{"rows", Node{std::move(rows)}}});
}

struct Row2 {
  std::int64_t n;
  std::string  s;
  double       d;
};

struct Data2 {
  std::vector<Row2> rows;
};

#define SOURCE2 "{{#rows}}<td>{{n}}</td><td>{{s}}</td><td>{{d}}</td><td>{{{s}}}</td>\n{{/rows}}"

Data2
data2()
{
  Data2 d;
  const std::vector<std::string> v = values();
  for (size_t i = 0; i < 60; i++) d.rows.push_back(Row2{(std::int64_t)i * 37 - 500, v.at(i % v.size()), (double)i / 8 - 3.25});
  return d;
}

Node
node2(const Data2 &d)
{
  NodeList rows;
  for (const Row2 &r : d.rows) rows.push_back(map_of({{"n", Node{r.n}}, {"s", text(r.s)}, {"d", Node{r.d}}}));
  return map_of({{"rows", Node{std::move(rows)}}});
}

#define SOURCE3 "{{x.y}}|{{#a}}[{{x.y}}{{#b}}({{x.y}},{{x.z}}){{/b}}{{x.y}}]{{/a}}|{{x.y}}{{^none}}{{x.y}}{{/none}}"

Node
node3()
{
  const Node leaf_b1 = map_of({{"x", map_of({{"y", text("B1")}, {"z", text("BZ")}})}});
  const Node leaf_b2 = map_of({});
  const Node leaf_b3 = map_of({{"x", map_of({{"z", text("onlyz")}})}});
  return map_of({
      {"x", map_of({{"y", text("R")}, {"z", text("RZ")}})},
      {"a", Node{NodeList{map_of({{"x", map_of({{"y", text("A1")}})}, {"b", Node{NodeList{leaf_b1, leaf_b2}}}}),
                          map_of({{"b", Node{NodeList{leaf_b3}}}}), map_of({{"x", text("str")}}),
                          map_of({{"x", map_of({{"y", text("A4")}})}, {"b", Node{NodeList{leaf_b1, leaf_b3, leaf_b2}}}})}}},
  });
}

#define SOURCE4 "{{#rows}}{{name.first}} {{name.last}} {{name.first}};{{/rows}}{{name.first}}{{^rows}}x{{name.first}}{{/rows}}"
#define SOURCE4P "{{#rows}}{{>p}}{{name.first}}{{>p}};{{/rows}}"

Node
node4()
{
  return map_of({
      {"name", map_of({{"first", text("RF")}, {"last", text("RL")}})},
      {"rows", Node{NodeList{map_of({{"name", map_of({{"first", text("A1")}, {"last", text("A2")}})}}), map_of({}),
                             map_of({{"name", map_of({{"first", text("B1")}})}}), map_of({{"name", text("s")}}),
                             map_of({{"name", map_of({{"first", text("C1")}, {"last", text("C2")}})}})}}},
  });
}

struct Stock5 {
  std::string symbol;
  std::string change;
};

struct Item5 {
  std::string rowClass;
  std::string index;
  std::string negativeClass;
  Stock5      value;
};

struct Data5 {
  std::vector<Item5> items;
};

#define SOURCE5                                                                                                        \
  "{{#items}}<tr class=\"{{rowClass}}\"><td>{{index}}</td><td><a href=\"/stocks/{{value.symbol}}\">{{value.symbol}}</a></td>"  \
  "<td{{negativeClass}}>{{value.change}}</td></tr>\n{{/items}}"

Data5
data5()
{
  Data5 d;
  for (size_t i = 0; i < 3000; i++) {
    d.items.push_back(Item5{i % 2 == 0 ? "even" : "odd", std::to_string(i + 1), i % 3 == 0 ? "class=\"minus\"" : "",
                            Stock5{"S" + std::to_string(i % 97) + (i % 5 == 0 ? "&<" : ""), std::to_string((int)(i % 11) - 5) + ".5"}});
  }
  return d;
}

Node
node5(const Data5 &d)
{
  NodeList items;
  for (const Item5 &it : d.items) {
    items.push_back(map_of({{"rowClass", text(it.rowClass)},
                            {"index", text(it.index)},
                            {"negativeClass", text(it.negativeClass)},
                            {"value", map_of({{"symbol", text(it.value.symbol)}, {"change", text(it.value.change)}})}}));
  }
  return map_of({{"items", Node{std::move(items)}}});
}

#ifdef __cpp_impl_reflection
template <mustache::fixed_string Source, class Data>
void
by_reflection(const std::string_view title, const Data &data)
{
  section(title, text_of(mustache::render<Source>(data)));
}
#endif

void
types_at_run_time()
{
  using Mixed = std::variant<std::string, std::vector<std::optional<std::string>>, int, bool, std::monostate>;
  const std::map<std::string, Mixed> m = {
      {"s", std::string("a<b")},
      {"l", std::vector<std::optional<std::string>>{std::string("x&y"), std::nullopt, std::string("z")}},
      {"i", 42},
      {"b", true},
      {"n", std::monostate{}},
  };
  at_run_time("types: std::map of a variant", "{{s}}|{{#l}}[{{.}}]{{/l}}|{{i}}|{{#b}}yes{{/b}}{{^b}}no{{/b}}|{{n}}|{{#n}}t{{/n}}{{^n}}f{{/n}}", m);
  const std::flat_map<std::string, double> f = {{"a", 1.5}, {"b", -0.25}};
  at_run_time("types: std::flat_map of double", "{{a}} {{b}} {{c}}", f);
  const std::unordered_map<std::string, std::vector<std::string>> u = {{"list", {"p", "q&", "<r>"}}, {"empty", {}}};
  at_run_time("types: std::unordered_map of vector", "{{#list}}{{.}},{{/list}}{{^empty}}none{{/empty}}", u);
  const std::vector<std::string> v = {"u", "v<", "w"};
  at_run_time("types: root list", "{{#.}}{{.}};{{/.}}", v);
  const std::optional<std::map<std::string, std::string>> o = std::map<std::string, std::string>{{"k", "v&"}};
  at_run_time("types: optional root", "{{k}}", o);
  const std::optional<std::map<std::string, std::string>> none;
  at_run_time("types: empty optional root", "[{{k}}]", none);
  using Inner = std::variant<int, std::string>;
  using Nested = std::variant<std::monostate, std::optional<Inner>, std::vector<std::variant<std::optional<std::string>, double>>>;
  const std::map<std::string, Nested> n = {
      {"a", std::optional<Inner>(std::string("q<"))},
      {"b", std::optional<Inner>()},
      {"c", std::vector<std::variant<std::optional<std::string>, double>>{std::optional<std::string>("e&"), std::optional<std::string>(), 2.5}},
      {"d", std::monostate{}},
      {"i", std::optional<Inner>(7)},
  };
  at_run_time("types: variant of optional of variant", "{{a}}|{{b}}|{{#c}}[{{.}}]{{/c}}|{{d}}|{{i}}|{{#a}}A{{/a}}{{^b}}nob{{/b}}", n);
}

void
kinds_of_a_node()
{
  const Node k = map_of({{"t", Node{true}},
                         {"f", Node{false}},
                         {"z", Node{nullptr}},
                         {"i", Node{std::int64_t{0}}},
                         {"d", Node{-0.5}},
                         {"e", map_of({})},
                         {"el", Node{NodeList{}}},
                         {"s", text("a&b<c")},
                         {"l", Node{NodeList{Node{1.0}, text("s&"), Node{nullptr}, Node{NodeList{text("in"), text("<2>")}},
                                             Node{std::int64_t{-3}}, Node{false}}}},
                         {"lm", Node{NodeList{map_of({{"x", text("X")}}), map_of({}), text("t"), Node{nullptr}, map_of({{"x", Node{2.5}}})}}},
                         {"m", map_of({{"n", map_of({{"o", map_of({{"p", text("deep")}})}})}, {"q", Node{NodeList{text("lq")}}}})}});
  at_run_time("kinds: numbers and text", "{{i}}|{{d}}|{{s}}|{{{s}}}|{{&s}}|{{missing}}|{{z}}|{{f}}", k);
  at_run_time("kinds: sections", "{{#t}}T{{/t}}{{#f}}F{{/f}}{{#z}}Z{{/z}}{{#i}}I{{/i}}{{#d}}D{{/d}}{{#e}}E{{/e}}{{#el}}L{{/el}}{{^el}}noel{{/el}}{{^e}}noe{{/e}}{{^missing}}nm{{/missing}}", k);
  at_run_time("kinds: list elements", "{{#l}}[{{#.}}<{{.}}>{{/.}}{{^.}}-{{/.}}]{{/l}}", k);
  at_run_time("kinds: list of maps and others", "{{#lm}}[{{x}}{{^x}}-{{/x}}]{{/lm}}", k);
  at_run_time("kinds: dotted names through a list, text and a map", "{{m.n.o.p}}|{{m.q.x}}|{{s.x}}|{{e.x}}|{{l.x}}|{{#m}}{{n.o.p}}{{#q}}{{.}}{{/q}}{{/m}}", k);
  at_run_time("kinds: a boolean as a tag is a fault", "{{t}}", k);
  at_run_time("kinds: a map as a tag is a fault", "{{e}}x{{m}}", k);
  at_run_time("kinds: a list as a tag is a fault", "{{l}}", k);
}

}

int
main(int argc, char **argv)
{
  constexpr std::string_view output_flag = "--output_file=";
  std::string path;
  for (int i = 1; i < argc; i++) {
    const std::string_view argument = argv[i];
    if (argument.starts_with(output_flag)) path = std::string(argument.substr(output_flag.size()));
  }
  if (path.empty()) return 2;

  const Data1 d1 = data1();
  at_run_time("escape over 32 bytes, run time", SOURCE1, node1(d1));
  const Data2 d2 = data2();
  at_run_time("number in a batched row, run time", SOURCE2, node2(d2));
  at_run_time("dotted name, first part changes with the stack, run time", SOURCE3, node3());
  at_run_time("dotted name, names in rows, run time", SOURCE4, node4());
  {
    mustache::Partials partials;
    std::expected<mustache::Template, mustache::Error> p = mustache::compile("{{name.first}}/{{name.last}}");
    if (!p) return 3;
    partials.emplace("p", std::move(*p));
    at_run_time("dotted name in a partial, run time", SOURCE4P, node4(), partials);
  }
  const Data5 d5 = data5();
  at_run_time("3000 rows, run time", SOURCE5, node5(d5));
  types_at_run_time();
  kinds_of_a_node();
#ifdef __cpp_impl_reflection
  by_reflection<SOURCE1>("escape over 32 bytes, reflection", d1);
  by_reflection<SOURCE2>("number in a batched row, reflection", d2);
  by_reflection<SOURCE3>("dotted name, first part changes with the stack, reflection", node3());
  by_reflection<SOURCE4>("dotted name, names in rows, reflection", node4());
  by_reflection<SOURCE5>("3000 rows, reflection", d5);
#endif
  std::ofstream out(path, std::ios::binary);
  out << output;
  return out ? 0 : 1;
}
