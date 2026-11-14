// Compiles the public templates for arm-none-eabi, where std::uint32_t is unsigned long.
// The file exists because a fault of the type of an integer showed on this target only.
// Each media type, a flat map, a nested map, a map of vectors and a partial are instantiated.
// The CI compiles the file to an object and does not link or run it.
#include <mustache/mustache.hpp>

#include <map>
#include <string>
#include <string_view>
#include <vector>

int
main()
{
  const std::map<std::string, std::string> data{{"name", "Ada <admin>"}, {"count", "3"}};
  const std::map<std::string, std::map<std::string, std::string>> nested{{"a", data}};
  const std::map<std::string, std::vector<std::string>> rows{{"items", {"x", "y"}}};
  mustache::Partials partials;
  const auto p = mustache::compile("[{{name}}]");
  if (!p) return 1;
  partials.emplace("p", *p);
  for (const std::string_view media :
       {"text/html", "application/xml", "application/json", "text/javascript", "text/css", "text/troff"}) {
    const auto tpl =
        mustache::compile("Hello, {{name}}! {{a.name}}\n{{#items}}{{.}}{{/items}}{{#count}}  {{>p}}{{/count}}", media);
    if (!tpl) return 1;
    const auto one = mustache::render(*tpl, data, partials);
    const auto two = mustache::render(*tpl, nested, partials);
    const auto three = mustache::render(*tpl, rows, partials);
    if (!one || !two || !three) return 1;
  }
  return 0;
}
