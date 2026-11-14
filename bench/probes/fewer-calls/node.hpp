#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

// The data of a bench is built from the map type that the README
// recommends. std::map stays in the tests only.
struct Node : std::variant<std::nullptr_t, bool, std::int64_t, double, std::string, std::vector<Node>,
                           std::unordered_map<std::string, Node>> {
  using variant::variant;
};

using NodeList = std::vector<Node>;
using NodeMap = std::unordered_map<std::string, Node>;
