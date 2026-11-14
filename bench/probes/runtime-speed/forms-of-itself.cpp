// Step 0 of the runtime-speed probe: does a Node that holds a std::flat_map or a std::unordered_map of itself compile,
// does it pass the map_like concept of containers.hpp, and does it render?
// Build: <compiler> -std=c++26 [-freflection] -I<include> forms-of-itself.cpp
#include <mustache/mustache.hpp>

#include "nodes.hpp"

#include <cstdio>
#include <string>
#include <utility>

template <template <class> class MapOf>
bool
renders_the_page()
{
  using Node = BasicNode<MapOf>;
  using Map = MapOf<Node>;
  static_assert(mustache::detail::containers::map_like<Map>);
  static_assert(mustache::detail::containers::variant_like<Node>);
  Map inner;
  inner.insert_or_assign("symbol", Node{std::string("A&B")});
  Map outer;
  outer.insert_or_assign("value", Node{std::move(inner)});
  const Node root{std::move(outer)};
  const auto t = mustache::compile("<{{value.symbol}}>");
  if (!t) return false;
  const auto r = mustache::render(*t, root);
  return r && *r == "<A&amp;B>";
}

int
main()
{
  const bool ordered = renders_the_page<OrderedMap>();
  const bool flat = renders_the_page<FlatMap>();
  const bool hashed = renders_the_page<HashedMap>();
  std::printf("std::map %d, std::flat_map %d, std::unordered_map %d\n", ordered, flat, hashed);
  return ordered && flat && hashed ? 0 : 1;
}
