// One Google Benchmark case for inja 3.5.0: the Jinja form of the page, HTML autoescape on, trim_blocks and lstrip_blocks on.
#include "harness.hpp"
#include "templates.hpp"
#include "values.hpp"

#include <inja/inja.hpp>

namespace {

using nlohmann::json;

#if WORKLOAD == 1
constexpr std::string_view kSource = probe::kJinja1;

json
root()
{
  const probe::Simple d = probe::simple();
  return json{{"title", d.title}, {"body", d.body}};
}

#elif WORKLOAD == 2
constexpr std::string_view kSource = probe::kJinja2;

json
root()
{
  json items = json::array();
  for (const probe::Item &it : probe::stock_items()) {
    items.push_back(json{{"rowClass", it.rowClass},
                         {"index", it.index},
                         {"negativeClass", it.negativeClass},
                         {"value",
                          json{{"symbol", it.value.symbol},
                               {"url", it.value.url},
                               {"name", it.value.name},
                               {"price", it.value.price},
                               {"change", it.value.change},
                               {"ratio", it.value.ratio}}}});
  }
  return json{{"items", items}};
}

#else
constexpr std::string_view kSource = probe::kJinja3;

json
root()
{
  const probe::News d = probe::news();
  json articles = json::array();
  for (const probe::Article &a : d.articles) {
    json tags = json::array();
    for (const probe::Tag &t : a.tags) tags.push_back(json{{"name", t.name}});
    articles.push_back(json{{"title", a.title},
                            {"url", a.url},
                            {"author", a.author},
                            {"date", a.date},
                            {"teaser", a.teaser},
                            {"tags", tags}});
  }
  return json{{"title", d.title}, {"articles", articles}};
}

#endif

}

int
main(int argc, char **argv)
{
  inja::Environment env;
  env.set_html_autoescape(true);
  env.set_trim_blocks(true);
  env.set_lstrip_blocks(true);
  const inja::Template compiled = env.parse(kSource);
  const json           data     = root();
  return run_engine(argc, argv, [&] { return env.render(compiled, data); });
}
