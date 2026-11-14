// One Google Benchmark case for kainjow/Mustache 4.1: the Mustache form of the page, compiled once.
#include "harness.hpp"
#include "templates.hpp"
#include "values.hpp"

#include <kainjow/mustache.hpp>

namespace {

using kainjow::mustache::data;

#if WORKLOAD == 1
constexpr std::string_view kSource = probe::kMustache1;

data
root()
{
  const probe::Simple d = probe::simple();
  data                m{data::type::object};
  m.set("title", data{d.title});
  m.set("body", data{d.body});
  return m;
}

#elif WORKLOAD == 2
constexpr std::string_view kSource = probe::kMustache2;

data
root()
{
  data items{data::type::list};
  for (const probe::Item &it : probe::stock_items()) {
    data stock{data::type::object};
    stock.set("symbol", data{it.value.symbol});
    stock.set("url", data{it.value.url});
    stock.set("name", data{it.value.name});
    stock.set("price", data{it.value.price});
    stock.set("change", data{it.value.change});
    stock.set("ratio", data{it.value.ratio});
    data item{data::type::object};
    item.set("rowClass", data{it.rowClass});
    item.set("index", data{it.index});
    item.set("negativeClass", data{it.negativeClass});
    item.set("value", stock);
    items.push_back(item);
  }
  data m{data::type::object};
  m.set("items", items);
  return m;
}

#else
constexpr std::string_view kSource = probe::kMustache3;

data
root()
{
  const probe::News d = probe::news();
  data              articles{data::type::list};
  for (const probe::Article &a : d.articles) {
    data tags{data::type::list};
    for (const probe::Tag &t : a.tags) {
      data tag{data::type::object};
      tag.set("name", data{t.name});
      tags.push_back(tag);
    }
    data article{data::type::object};
    article.set("title", data{a.title});
    article.set("url", data{a.url});
    article.set("author", data{a.author});
    article.set("date", data{a.date});
    article.set("teaser", data{a.teaser});
    article.set("tags", tags);
    articles.push_back(article);
  }
  data m{data::type::object};
  m.set("title", data{d.title});
  m.set("articles", articles);
  return m;
}

#endif

}

int
main(int argc, char **argv)
{
  kainjow::mustache::mustache compiled{std::string(kSource)};
  const data                  root_data = root();
  return run_engine(argc, argv, [&] { return compiled.render(root_data); });
}
