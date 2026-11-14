// The data forms of the runtime-speed probe: one Node type for each map type that holds the children of a node.
#pragma once

#include <cstddef>
#include <cstdint>
#include <flat_map>
#include <functional>
#include <map>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

template <template <class> class MapOf>
struct BasicNode;

template <template <class> class MapOf>
using NodeVariant = std::variant<std::nullptr_t, bool, std::int64_t, double, std::string, std::vector<BasicNode<MapOf>>,
                                 MapOf<BasicNode<MapOf>>>;

template <template <class> class MapOf>
struct BasicNode : NodeVariant<MapOf> {
  using Base = NodeVariant<MapOf>;
  using Base::Base;
};

template <class Node>
using OrderedMap = std::map<std::string, Node, std::less<>>;

template <class Node>
using FlatMap = std::flat_map<std::string, Node>;

template <class Node>
using HashedMap = std::unordered_map<std::string, Node>;
