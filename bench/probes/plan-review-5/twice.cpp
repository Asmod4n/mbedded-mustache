// A range that can be read once, rendered two times by the reflection render of the tree.
#include <mustache/mustache.hpp>
#include <cstdio>
#include <iterator>
#include <ranges>
#include <sstream>
#include <string>
struct Page {
  std::ranges::subrange<std::istream_iterator<std::string>> names;
};
int main()
{
  std::istringstream in("a b c d");
  const Page page{{std::istream_iterator<std::string>(in), std::istream_iterator<std::string>()}};
  const auto first = mustache::render<"{{#names}}{{.}},{{/names}}">(page);
  const auto second = mustache::render<"{{#names}}{{.}},{{/names}}">(page);
  printf("first '%s' second '%s'\n", first ? first->c_str() : "error", second ? second->c_str() : "error");
}
