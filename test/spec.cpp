#include <mustache/std.hpp>

#include "c_host.hpp"

#include <simdjson.h>

#include <gtest/gtest.h>

#include <array>
#include <vector>
#include <string>
#include <string_view>
#include <unordered_map>
#include <cctype>

namespace {

using mustache::std_host::Host;
using mustache::std_host::Key;
using mustache::std_host::KeyEqual;
using mustache::std_host::KeyHash;
using mustache::std_host::List;
using mustache::std_host::Map;
using mustache::std_host::Value;
using Program = mustache::Program<Key>;
using Partials = std::unordered_map<std::string, const Program *, KeyHash, KeyEqual>;

// Each JSON type becomes the C++ type that holds it. A number stays a
// number, so the render writes it with std::to_chars, and null stays
// null, a value of its own that is falsy.
Value
value_of(simdjson::ondemand::value v)
{
  switch (v.type().value()) {
    case simdjson::ondemand::json_type::object: {
      Map m;
      for (auto field : v.get_object()) {
        const std::string key(field.unescaped_key().value());
        m.insert_or_assign(key, value_of(field.value()));
      }
      return Value{std::move(m)};
    }
    case simdjson::ondemand::json_type::array: {
      List l;
      for (auto e : v.get_array()) l.push_back(value_of(e.value()));
      return Value{std::move(l)};
    }
    case simdjson::ondemand::json_type::string:
      return Value{std::string(v.get_string().value())};
    case simdjson::ondemand::json_type::number:
      if (v.get_number_type().value() == simdjson::ondemand::number_type::floating_point_number) {
        return Value{v.get_double().value()};
      }
      return Value{v.get_int64().value()};
    case simdjson::ondemand::json_type::boolean:
      return Value{v.get_bool().value()};
    default:
      return Value{nullptr};
  }
}

struct Case {
  std::string                                  name;
  std::string                                  source;
  std::string                                  expected;
  Value                                        data;
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
  Host h;
  std::unordered_map<std::string, Program> compiled;
  Partials partials;
  for (const auto &[name, source] : c.partials) {
    std::optional<Program> p = mustache::program_of<Key>(h, source, mustache::kSourceMax);
    if (!p) return false;
    partials.emplace(name, &compiled.emplace(name, std::move(*p)).first->second);
  }
  h.partials = &partials;
  const std::optional<Program> p = mustache::program_of<Key>(h, c.source, mustache::kSourceMax);
  if (!p) return false;
  const mustache::Fault f = mustache::Walk<Host, const Value *, Key>(h).run(*p, &c.data);
  return f == mustache::Fault::none && h.answer == c.expected;
}

// The same cases run through the C API, partials included: the partial
// callback gives the handle of a compiled partial.
bool
renders_as_expected_through_c(const Case &c)
{
  std::unordered_map<std::string, mustache_template *> partials;
  bool compiled = true;
  for (const auto &[name, source] : c.partials) {
    mustache_template *p = nullptr;
    compiled = mustache_compile(source.data(), source.size(), &p) == 0 && compiled;
    partials.emplace(name, p);
  }
  mustache_template *tpl = nullptr;
  compiled = mustache_compile(c.source.data(), c.source.size(), &tpl) == 0 && compiled;
  CUser user;
  user.partials = &partials;
  void *string = nullptr;
  int rendered = -1;
  if (compiled) {
    c_set_callbacks(tpl);
    rendered = mustache_render(tpl, &c.data, c_release, &user, &string);
  }
  mustache_dispose_template(tpl);
  for (const auto &[name, p] : partials) mustache_dispose_template(p);
  return rendered == 0 && string == &user && user.answer == c.expected;
}

// Every case of every module of the Mustache spec in refs/mustache-spec
// is one test, run through the C++ Walk and through the C API.
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
      all.push_back(Case{std::string(module) + ": cannot read " + path, "", "<unread>", Value{nullptr}, {}});
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

class Spec : public testing::TestWithParam<Case> {};

TEST_P(Spec, RendersThroughTheWalk) { EXPECT_TRUE(renders_as_expected(GetParam())) << GetParam().name; }

TEST_P(Spec, RendersThroughTheCApi) { EXPECT_TRUE(renders_as_expected_through_c(GetParam())) << GetParam().name; }

std::string
name_of(const testing::TestParamInfo<Case> &info)
{
  std::string name = std::to_string(info.index) + "_";
  for (const char c : info.param.name) name += std::isalnum((unsigned char)c) != 0 ? c : '_';
  return name;
}

INSTANTIATE_TEST_SUITE_P(Mustache, Spec, testing::ValuesIn(every_case()), name_of);

}
