// The extra inputs of the byte check of the engine-work probe: the cases that the stocks page does not reach. The first part is
// the extras of the register-returns probe. The cases of the engine work follow: a text op before a tag op (E2), the output that
// grows (E3a), the limits of the output and the contexts of the media types.
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
}

std::string
full_text_of(const std::expected<std::string, mustache::Error> &r)
{
  if (r) return *r;
  return "ERROR " + std::to_string((int)r.error().fault) + " " + std::string(r.error().what) + " asked " +
         std::to_string(r.error().asked) + " allowed " + std::to_string(r.error().allowed);
}

std::expected<std::string, mustache::Error>
walked(const mustache::Template &t, const Node &data, const mustache::Partials &partials, const size_t initial_capacity,
       const size_t max_capacity)
{
  return mustache::Walk<>(mustache::detail::containers::Access{}, &partials, mustache::kMaxRenderScore, initial_capacity, max_capacity)
      .run(t, mustache::detail::containers::value_of(data));
}

void
text_before_tag()
{
  const Node list3 = map_of({{"a", Node{NodeList{map_of({{"b", text("1")}}), map_of({}), map_of({{"b", text("3<")}})}}}, {"b", text("B&")}});
  const Node absent = map_of({{"b", text("B&")}});
  const Node no = map_of({{"a", Node{false}}, {"b", text("B&")}});
  const Node nothing = map_of({{"a", Node{nullptr}}, {"b", text("B&")}});
  const Node empty_list = map_of({{"a", Node{NodeList{}}}, {"b", text("B&")}});
  const Node yes = map_of({{"a", Node{true}}, {"b", text("B&")}});
  const Node a_map = map_of({{"a", map_of({{"k", text("v")}})}, {"b", text("B&")}});
  const Node a_text = map_of({{"a", text("s")}, {"b", text("B&")}});
  const std::vector<std::pair<std::string, Node>> inputs = {{"list", list3}, {"absent", absent}, {"false", no}, {"null", nothing},
                                                            {"empty list", empty_list}, {"true", yes}, {"map", a_map}, {"text", a_text}};
  for (const char *source : {"{{#a}}x{{/a}}{{b}}", "{{#a}}x{{/a}}{{{b}}}", "{{#a}}x{{b}}y{{/a}}{{b}}", "{{^a}}x{{/a}}{{b}}",
                              "{{^a}}x{{/a}}z{{b}}", "{{#a}}{{#a}}x{{/a}}{{/a}}{{b}}", "{{b}}x{{b}}y{{{b}}}z{{&b}}", "x{{b}}",
                              "x{{b}}y", "{{b}}", "x{{a}}", "x{{nope}}y{{nope}}", "[{{#a}}x{{/a}}]{{b}}",
                              "{{$blk}}x{{/blk}}{{b}}", "{{$blk}}x{{/blk}}{{b}}{{$blk}}y{{/blk}}z{{b}}"}) {
    for (const auto &[name, data] : inputs) at_run_time(std::string("text before tag, ") + source + " with a as " + name, source, data);
  }
  mustache::Partials partials;
  const auto add = [&partials](const char *name, const char *source) {
    std::expected<mustache::Template, mustache::Error> p = mustache::compile(source);
    if (p) partials.emplace(name, std::move(*p));
  };
  add("p", "[{{$blk}}d{{/blk}}]{{b}}");
  add("q", "l1\n{{b}}\nl3 {{b}}\n{{$blk}}t{{/blk}}{{b}}\n");
  add("r", "{{#a}}x{{/a}}{{b}}");
  add("s", "tail");
  for (const char *source : {"{{<p}}{{$blk}}x{{/blk}}{{/p}}{{b}}", "{{<p}}{{$blk}}x{{/blk}}{{b}}{{/p}}", "{{<p}}{{/p}}{{b}}", " {{>q}}",
                              "  {{>q}}\nz{{b}}", "{{>r}}{{b}}", "x{{>r}}{{b}}", "{{>s}}{{b}}", "{{>s}}{{b}}x{{>s}}{{b}}", "{{>none}}{{b}}",
                              "{{<q}}{{$blk}}x{{/blk}}{{/q}}"}) {
    for (const auto &[name, data] : inputs) at_run_time(std::string("text before tag with partials, ") + source + " with a as " + name, source, data, partials);
  }
  const Node numbers = map_of({{"n", Node{std::int64_t{-42}}}, {"d", Node{2.5}}, {"t", text("")}, {"m", map_of({{"k", text("v")}})}, {"l", Node{NodeList{text("e")}}}});
  for (const char *source : {"n={{n}}, d={{d}}, t={{t}}.", "x{{t}}y{{t}}z", "x{{m}}y", "x{{l}}y", "x{{n}}{{d}}y"}) at_run_time(std::string("text before tag, scalars, ") + source, source, numbers);
}

void
growth_and_limits()
{
  const Data5 d5 = data5();
  const Node page = node5(d5);
  std::expected<mustache::Template, mustache::Error> t = mustache::compile(SOURCE5);
  if (!t) return;
  for (const size_t initial : {size_t{0}, size_t{1}, size_t{7}, size_t{8}, size_t{31}, size_t{32}, size_t{33}, size_t{1024}, size_t{8192}, size_t{1} << 20}) {
    const std::string bytes = text_of(mustache::render(*t, page, {}, initial));
    section("3000 rows with initial capacity " + std::to_string(initial) + ", size " + std::to_string(bytes.size()), bytes);
  }
  const Node data = map_of({{"x", text("<&>\"'")}, {"y", text("plain")}, {"z", text("a<b&c>d\"e'f<<<<<&&&&&>>>>>")}, {"e", text("")}, {"n", Node{std::int64_t{12345}}}});
  const char *sources[] = {"ab{{x}}<{{y}}cd{{{z}}}", "{{x}}{{x}}{{x}}", "{{e}}{{y}}{{e}}", "q{{z}}{{z}}", "{{n}}{{x}}t{{y}}", "{{y}}"};
  for (const char *source : sources) {
    std::expected<mustache::Template, mustache::Error> p = mustache::compile(source);
    if (!p) continue;
    for (const size_t initial : {size_t{0}, size_t{1}, size_t{5}, size_t{40}, size_t{1000}}) {
      for (size_t max = 0; max <= 150; max++) {
        const std::string label = std::string("limit sweep ") + source + " initial " + std::to_string(initial) + " max " + std::to_string(max);
        section(label, full_text_of(walked(*p, data, {}, initial, max)));
      }
    }
  }
  for (const char *media : {"application/json", "application/xml", "text/css", "text/javascript", "text/troff", "text/html"}) {
    std::expected<mustache::Template, mustache::Error> p = mustache::compile("v=\"{{x}}\" w={{z}} {{y}}{{{z}}}", media);
    if (!p) {
      section(std::string("context ") + media, "COMPILE ERROR");
      continue;
    }
    for (const size_t initial : {size_t{0}, size_t{16}, size_t{4096}}) {
      for (size_t max = 0; max <= 160; max += (max < 60 ? 1 : 10)) {
        section(std::string("context ") + media + " initial " + std::to_string(initial) + " max " + std::to_string(max),
                full_text_of(walked(*p, data, {}, initial, max)));
      }
    }
  }
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
  text_before_tag();
  growth_and_limits();
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
