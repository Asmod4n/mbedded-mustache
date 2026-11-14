#include <mustache/mustache.hpp>
#include "node.hpp"
#include <cstdio>
#include <flat_map>
#include <string>
#include <string_view>

struct Fixed {
  std::flat_map<std::string_view, std::string_view> names;
};
constexpr Fixed fixed() { return Fixed{{{"title", "Hello"}, {"lang", "en"}}}; }

int main()
{
  constexpr auto page = mustache::static_page_of<"{{#names}}{{title}}/{{lang}}{{/names}}", fixed>;
  std::printf("static_page: [%s] (want [Hello/en])\n", page.data());

  NodeMap m;
  m.insert_or_assign("a", Node{std::string(100, '"')});
  const auto t = mustache::compile("{{a}}");
  for (const size_t max : {size_t{599}, size_t{600}, size_t{631}, size_t{632}}) {
    auto r = mustache::Walk<>(mustache::detail::containers::Access{}, nullptr, mustache::kMaxRenderScore, 16, max)
                 .run(*t, mustache::detail::containers::value_of(m));
    if (r) std::printf("max_capacity %zu: ok, %zu bytes\n", max, r->size());
    else std::printf("max_capacity %zu: fault %d asked %zu allowed %zu\n", max, (int)r.error().fault, r.error().asked, r.error().allowed);
  }
}
