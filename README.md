# mustache

Fast, safe [Mustache](https://mustache.github.io/) templates for modern C++.

```cpp
#include <mustache/mustache.hpp>
#include <map>
#include <print>
#include <string>

int main() {
  auto tpl = mustache::compile("Hello, {{name}}! You have {{count}} new messages.", "text/html");
  std::map<std::string, std::string> data{{"name", "Ada <admin>"}, {"count", "3"}};
  std::println("{}", *mustache::render(*tpl, data));
}
```

```
Hello, Ada &lt;admin&gt;! You have 3 new messages.
```

## Features

- **Header-only C++23.** One include, one CMake target.
- **Compile once, render many times.** Parsing happens once; rendering is a tight loop over the compiled template.
- **Your data, as it is.** Renders straight from `std::map`, `std::unordered_map`, `std::vector`, `std::variant`, `std::optional`, strings and numbers. No wrapper types to fill in.
- **Escaping for the format you produce.** HTML by default, plus XML, JSON, JavaScript, CSS and troff (man pages). SIMD-accelerated on x86-64 (AVX2) and ARM (NEON).
- **Compile-time templates with C++26 reflection.** Render a plain struct; a typo in a key is a compile error.
- **Safe by design.** No lambdas, no dynamic partial names: data never runs code or picks a template. Every limit (nesting, partial depth, output size, work) is checked, and errors come back as `std::expected`.
- **Spec compliant.** Passes all 163 cases of the core [Mustache specification](https://github.com/mustache/spec).
- **Tested on** g++, clang, Apple clang and MSVC, on Linux, macOS and Windows.

## Installation

You need a C++23 compiler and CMake 3.25 or newer.

### CMake FetchContent

```cmake
include(FetchContent)
FetchContent_Declare(mustache
  GIT_REPOSITORY https://github.com/Asmod4n/mbedded-mustache.git
  GIT_TAG main)
FetchContent_MakeAvailable(mustache)

target_link_libraries(app PRIVATE mustache::mustache)
```

### Installed package

```sh
git clone https://github.com/Asmod4n/mbedded-mustache.git
cmake -S mbedded-mustache -B build -DBUILD_TESTING=OFF
cmake --install build --prefix /usr/local
```

```cmake
find_package(mustache REQUIRED)
target_link_libraries(app PRIVATE mustache::mustache)
```

## Usage

### Data

`render` accepts any of these types, nested as deep as you like:

| C++ type | in the template |
|---|---|
| `std::string`, `std::string_view`, anything convertible to `std::string_view` | text |
| `int`, `double`, other arithmetic types | text (via `std::to_chars`) |
| `bool` | true / false |
| `std::nullptr_t`, `std::monostate` | false |
| `std::optional<T>` | false when empty, otherwise the `T` |
| `std::variant<...>` (or a type derived from one) | the active alternative |
| a map with string keys (`std::map`, `std::unordered_map`, ...) | a map, so `{{a.b}}` works |
| a random-access range (`std::vector`, ...) | a list |

A key that isn't in the data renders as nothing, as the specification says.

For data that contains itself, such as a JSON tree, derive a type from the variant:

```cpp
struct Node : std::variant<std::nullptr_t, bool, double, std::string,
                           std::vector<Node>, std::map<std::string, Node>> {
  using variant::variant;
};
```

### Sections, lists and partials

```cpp
#include <mustache/mustache.hpp>
#include <map>
#include <print>
#include <string>
#include <variant>
#include <vector>

using Item  = std::map<std::string, std::variant<std::string, double>>;
using Value = std::variant<std::string, bool, std::vector<Item>>;

int main() {
  auto page = mustache::compile(
      "<h1>{{title}}</h1>\n"
      "{{#items}}{{> item}}{{/items}}"
      "{{^items}}<p>Your cart is empty.</p>\n{{/items}}"
      "{{#member}}<p>Free shipping for members.</p>\n{{/member}}");
  auto item = mustache::compile("<li>{{name}}: {{price}} EUR</li>\n");

  mustache::Partials partials{{"item", *item}};

  std::map<std::string, Value> data{
      {"title", std::string("Cart")},
      {"items", std::vector<Item>{{{"name", std::string("Tea")}, {"price", 4.5}},
                                  {{"name", std::string("Mug")}, {"price", 12.0}}}},
      {"member", true}};

  std::print("{}", *mustache::render(*page, data, partials));
}
```

```
<h1>Cart</h1>
<li>Tea: 4.5 EUR</li>
<li>Mug: 12 EUR</li>
<p>Free shipping for members.</p>
```

### Escaping

`{{name}}` escapes, `{{{name}}}` and `{{&name}}` don't. The media type you pass to `compile` picks the escaping. Without one, the escaping is HTML, as the Mustache specification says.

```cpp
auto tpl = mustache::compile(R"({"greeting": "Hello, {{name}}"})", "application/json");
```

| media type | escaping |
|---|---|
| `text/html`, `application/xhtml+xml` | HTML entities |
| `application/xml`, `text/xml`, `*+xml` | the five XML entities |
| `application/json`, `*+json` | JSON string (RFC 8259) |
| `text/javascript` | JavaScript string |
| `text/css` | CSS string |
| `text/troff` | man page text lines |

Escaping makes a value safe *inside text or a string literal* of that format. Where you put the tag is up to you. An unknown media type is an error, and so is `image/svg+xml`, because SVG can carry scripts that XML escaping doesn't stop.

### Structs with C++26 reflection

With a compiler that supports C++26 reflection (for example g++ 16 with `-std=c++26 -freflection`), you can render a plain struct, and the template is compiled at compile time:

```cpp
#include <mustache/mustache.hpp>
#include <optional>
#include <print>
#include <string>
#include <vector>

struct Item { std::string name; double price; };
struct Page {
  std::string                title;
  std::vector<Item>          items;
  std::optional<std::string> note;
};

int main() {
  Page page{"Cart", {{"Tea", 4.5}, {"Mug", 12}}, std::nullopt};

  auto html = mustache::render<"<h1>{{title}}</h1>\n"
                               "{{#items}}<li>{{name}}: {{price}} EUR</li>\n{{/items}}">(page);
  std::print("{}", *html);

  auto json = mustache::render<"{\"title\": \"{{title}}\"}\n">(page, "application/json");
  std::print("{}", *json);
}
```

A key that names no member, like `{{titel}}`, fails to compile. This is also the fastest way to render.

### Run-time templates for a struct

When the template is only known at run time, compile it for your struct type. Keys are resolved to members once, in `compile`:

```cpp
auto tpl = mustache::compile<Page>("<h1>{{title}}</h1>\n{{#items}}<li>{{name}}</li>\n{{/items}}");
auto out = mustache::render(*tpl, page);
```

This works with C++26 reflection, or on C++23 with [Boost.PFR](https://github.com/boostorg/pfr) 1.84 or newer. With Boost.PFR the struct must be an aggregate (no constructors, no base classes, no private members).

## Errors

`compile` and `render` return `std::expected`. On failure you get a `mustache::Error`:

```cpp
auto tpl = mustache::compile("{{#open}} never closed");
if (!tpl) {
  std::println("{} at offset {}", tpl.error().what, tpl.error().asked);
}
```

| `Error::fault` | meaning |
|---|---|
| `parse` | the template has a syntax error; `asked` is the offset |
| `not_text` | `{{x}}` names a list or a map |
| `too_deep` | sections nest deeper than 32, or partials deeper than 64 |
| `over_work` | the render did more work than allowed (for example, partials that call each other) |
| `over_limit` | the template or the output is larger than allowed; `allowed` is the limit |
| `no_memory` | an allocation failed |
| `illegal_control_character` | a `text/troff` value contains a control character |

## Not supported

- **Lambdas** (optional in the spec). Data never runs code.
- **Dynamic names** (optional in the spec). Data never chooses which partial to render.

## Performance

Measurements, with their sources and the full context of each run, are in [`bench/`](bench/README.md).

## Building the tests

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

The tests use GoogleTest and simdjson. Without installed packages, point CMake at their sources with `-DMUSTACHE_GOOGLETEST_SOURCE_DIR=...` and `-DMUSTACHE_SIMDJSON_SOURCE_DIR=...`.

## License

[Apache-2.0](LICENSE)
