#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <variant>
#include <vector>

// The data of a test is built from standard types only, the way a user
// of the library builds theirs. A value that holds itself needs a named
// type, so Node derives from the variant; std::visit and the library's
// variant_like concept both accept it.
struct Node : std::variant<std::nullptr_t, bool, std::int64_t, double, std::string, std::vector<Node>,
                           std::map<std::string, Node, std::less<>>> {
  using variant::variant;
};

using NodeList = std::vector<Node>;
using NodeMap = std::map<std::string, Node, std::less<>>;
