#include <mustache/mustache.hpp>

#include "node.hpp"

#include <simdjson.h>

#include <gtest/gtest.h>

#include <array>
#include <vector>
#include <string>
#include <string_view>
#include <unordered_map>
#include <cctype>

namespace {

// Each JSON type becomes the standard type that holds it. A number stays
// a number, so the render writes it with std::to_chars, and null stays
// null, a value of its own that is falsy.
Node
value_of(simdjson::ondemand::value v)
{
  switch (v.type().value()) {
    case simdjson::ondemand::json_type::object: {
      NodeMap m;
      for (auto field : v.get_object()) {
        const std::string key(field.unescaped_key().value());
        m.insert_or_assign(key, value_of(field.value()));
      }
      return Node{std::move(m)};
    }
    case simdjson::ondemand::json_type::array: {
      NodeList l;
      for (auto e : v.get_array()) l.push_back(value_of(e.value()));
      return Node{std::move(l)};
    }
    case simdjson::ondemand::json_type::string:
      return Node{std::string(v.get_string().value())};
    case simdjson::ondemand::json_type::number:
      if (v.get_number_type().value() == simdjson::ondemand::number_type::floating_point_number) {
        return Node{v.get_double().value()};
      }
      return Node{v.get_int64().value()};
    case simdjson::ondemand::json_type::boolean:
      return Node{v.get_bool().value()};
    default:
      return Node{nullptr};
  }
}

struct Case {
  std::string                                  name;
  std::string                                  source;
  std::string                                  expected;
  Node                                         data;
  std::unordered_map<std::string, std::string> partials;
};

Case
case_of(simdjson::ondemand::object t)
{
  Case c{};
  for (auto field : t) {
    const std::string_view key = field.unescaped_key().value();
    if (key == "name") c.name = std::string(field.value().get_string().value());
    else if (key == "template") c.source = std::string(field.value().get_string().value());
    else if (key == "expected") c.expected = std::string(field.value().get_string().value());
    else if (key == "data") c.data = value_of(field.value());
    else if (key == "partials") {
      for (auto p : field.value().get_object()) {
        c.partials.emplace(std::string(p.unescaped_key().value()), std::string(p.value().get_string().value()));
      }
    }
  }
  return c;
}

// Each case compiles its partials first, the way a caller hands them to
// a render. A template or a partial that does not compile is a failure
// of the case, because every source in the spec is valid Mustache.
bool
renders_as_expected(const Case &c)
{
  mustache::Partials partials;
  for (const auto &[name, source] : c.partials) {
    std::expected<mustache::Template, mustache::Error> p = mustache::compile(source);
    if (!p) return false;
    partials.emplace(name, std::move(*p));
  }
  const std::expected<mustache::Template, mustache::Error> t = mustache::compile(c.source);
  if (!t) return false;
  const std::expected<std::string, mustache::Error> r = mustache::render(*t, c.data, partials);
  return r && *r == c.expected;
}

// Every case of every module of the Mustache spec in refs/mustache-spec
// is one test.
// ~inheritance is the optional module this library implements.
std::vector<Case>
every_case()
{
  constexpr std::array kModules = {"comments", "delimiters", "interpolation", "inverted",
                                   "partials", "sections", "~inheritance"};
  std::vector<Case> all;
  for (const char *const module : kModules) {
    const std::string path = std::string(MUSTACHE_SPEC_DIR) + "/" + module + ".json";
    simdjson::padded_string json;
    if (simdjson::padded_string::load(path).get(json) != simdjson::SUCCESS) [[unlikely]] {
      all.push_back(Case{std::string(module) + ": cannot read " + path, "", "<unread>", Node{nullptr}, {}});
      continue;
    }
    simdjson::ondemand::parser parser;
    simdjson::ondemand::document doc = parser.iterate(json);
    for (auto t : doc["tests"].get_array()) {
      Case c = case_of(t.get_object());
      c.name = std::string(module) + ": " + c.name;
      all.push_back(std::move(c));
    }
  }
  return all;
}

#if defined(__cpp_impl_reflection) || __has_include(<boost/pfr/core_name.hpp>)
// The same case through a template compiled for the type of its data.
// The data are a Node, a variant, so every key is found at render time,
// and the typed path has to agree with the run-time path on every case.
bool
renders_as_expected_typed(const Case &c)
{
  mustache::Partials partials;
  for (const auto &[name, source] : c.partials) {
    std::expected<mustache::Template, mustache::Error> p = mustache::compile(source);
    if (!p) return false;
    partials.emplace(name, std::move(*p));
  }
  const std::expected<mustache::TypedTemplate<Node>, mustache::Error> t = mustache::compile<Node>(c.source);
  if (!t) return false;
  const std::expected<std::string, mustache::Error> r = mustache::render(*t, c.data, partials);
  return r && *r == c.expected;
}
#endif

class Spec : public testing::TestWithParam<Case> {};

TEST_P(Spec, RendersThroughTheWalk) { EXPECT_TRUE(renders_as_expected(GetParam())) << GetParam().name; }

#if defined(__cpp_impl_reflection) || __has_include(<boost/pfr/core_name.hpp>)
TEST_P(Spec, RendersThroughATypedTemplate) { EXPECT_TRUE(renders_as_expected_typed(GetParam())) << GetParam().name; }
#endif

std::string
name_of(const testing::TestParamInfo<Case> &info)
{
  std::string name = std::to_string(info.index) + "_";
  for (const char c : info.param.name) name += std::isalnum((unsigned char)c) != 0 ? c : '_';
  return name;
}

INSTANTIATE_TEST_SUITE_P(Mustache, Spec, testing::ValuesIn(every_case()), name_of);

}
