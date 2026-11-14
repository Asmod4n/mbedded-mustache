// One Google Benchmark case for mbedded-mustache: the page of workload 2, compiled at run time for a struct type with
// mustache::compile<T>, rendered from the structs of values.hpp. The template text is the one of bench/render.cpp.
#include "harness.hpp"
#include "templates.hpp"
#include "values.hpp"

#include <mustache/mustache.hpp>

#include <exception>
#include <vector>

namespace {

struct Data {
  std::vector<probe::Item> items;
};

const Data kData{probe::stock_items()};

const mustache::TypedTemplate<Data> kTemplate = [] {
  std::expected<mustache::TypedTemplate<Data>, mustache::Error> t = mustache::compile<Data>(probe::kMustache2);
  if (!t) std::terminate();
  return std::move(*t);
}();

std::string
rendered()
{
  std::expected<std::string, mustache::Error> r = mustache::render(kTemplate, kData);
  if (!r) std::terminate();
  return std::move(*r);
}

}

int
main(int argc, char **argv)
{
  return run_engine(argc, argv, rendered);
}
