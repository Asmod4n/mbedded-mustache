// One Google Benchmark case for mstch 1.0.2: the Mustache form of the page. mstch::render parses the template at each call.
#include "harness.hpp"
#include "templates.hpp"
#include "values.hpp"

#include <mstch/mstch.hpp>

namespace {

#if WORKLOAD == 1
constexpr std::string_view kSource = probe::kMustache1;

mstch::node
root()
{
  const probe::Simple d = probe::simple();
  return mstch::map{{"title", d.title}, {"body", d.body}};
}

#elif WORKLOAD == 2
constexpr std::string_view kSource = probe::kMustache2;

mstch::node
root()
{
  mstch::array items;
  for (const probe::Item &it : probe::stock_items()) {
    items.push_back(mstch::map{{"rowClass", it.rowClass},
                               {"index", it.index},
                               {"negativeClass", it.negativeClass},
                               {"value",
                                mstch::map{{"symbol", it.value.symbol},
                                           {"url", it.value.url},
                                           {"name", it.value.name},
                                           {"price", it.value.price},
                                           {"change", it.value.change},
                                           {"ratio", it.value.ratio}}}});
  }
  return mstch::map{{"items", items}};
}

#else
constexpr std::string_view kSource = probe::kMustache3;

mstch::node
root()
{
  const probe::News d = probe::news();
  mstch::array      articles;
  for (const probe::Article &a : d.articles) {
    mstch::array tags;
    for (const probe::Tag &t : a.tags) tags.push_back(mstch::map{{"name", t.name}});
    articles.push_back(mstch::map{{"title", a.title},
                                  {"url", a.url},
                                  {"author", a.author},
                                  {"date", a.date},
                                  {"teaser", a.teaser},
                                  {"tags", tags}});
  }
  return mstch::map{{"title", d.title}, {"articles", articles}};
}

#endif

}

int
main(int argc, char **argv)
{
  const std::string source(kSource);
  const mstch::node data = root();
  return run_engine(argc, argv, [&] { return mstch::render(source, data); });
}
