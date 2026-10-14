# mustache

Mustache templates for C++. The library offers a C++ API and nothing
else: a caller speaks C++.

The library compiles a template once and renders it many times. It
reads your data as standard C++ types, and it escapes HTML by default. It passes 163 cases of the Mustache specification. It
never runs code of the data it renders.

- C++23, header only: `include/mustache/mustache.hpp`.
- A reflection render for C++26 compilers with reflection: the
  template is compiled at compile time, and a key that names no member
  is a compile error.

## Contents

1. [Install](#install)
2. [A first render](#a-first-render)
3. [What the render reads](#what-the-render-reads)
4. [Sections, lists, partials](#sections-lists-partials)
5. [Escaping by media type](#escaping-by-media-type)
6. [Render a C++ struct with reflection](#render-a-c-struct-with-reflection)
7. [Compile a template for one struct type](#compile-a-template-for-one-struct-type)
8. [Errors](#errors)
9. [What the library does not do](#what-the-library-does-not-do)
10. [Tests and benchmarks](#tests-and-benchmarks)

## Install

The library needs a C++23 compiler and CMake 3.25 or later. The
reflection render needs a compiler with C++26 reflection, for example
g++ 16 with `-std=c++26 -freflection`.

CMake exports one target:

| target | what it is |
|---|---|
| `mustache::mustache` | the C++ headers, an INTERFACE library |

### With FetchContent

```cmake
include(FetchContent)
FetchContent_Declare(mustache
  GIT_REPOSITORY https://github.com/Asmod4n/mbedded-mustache.git
  GIT_TAG main)
FetchContent_MakeAvailable(mustache)

target_link_libraries(app PRIVATE mustache::mustache)
```

### As an installed package

```sh
git clone --recursive https://github.com/Asmod4n/mbedded-mustache.git mustache
cmake -S mustache -B build -DBUILD_TESTING=OFF
cmake --build build
cmake --install build --prefix /usr/local
```

```cmake
find_package(mustache REQUIRED)
target_link_libraries(app PRIVATE mustache::mustache)
```

## A first render

```cpp
#include <mustache/mustache.hpp>

#include <cstdio>
#include <map>
#include <string>

int main()
{
  const auto tpl = mustache::compile("Hello, {{name}}! You have {{count}} new {{what}}.");
  if (!tpl) {
    std::printf("the template does not compile: %.*s\n", (int)tpl.error().what.size(), tpl.error().what.data());
    return 1;
  }

  const std::map<std::string, std::string> data{{"name", "Ada <admin>"}, {"count", "3"}, {"what", "messages"}};

  const auto answer = mustache::render(*tpl, data);
  if (!answer) {
    std::printf("the render failed: %.*s\n", (int)answer.error().what.size(), answer.error().what.data());
    return 1;
  }
  std::printf("%s\n", answer->c_str());
}
```

Output:

```
Hello, Ada &lt;admin&gt;! You have 3 new messages.
```

- `compile` compiles the template once. Keep the template and render it
  as often as you need.
- `render` reads your data where it is, as standard C++ types, and gives
  back a `std::string`.
- `{{name}}` escapes `&`, `<`, `>`, `"` and `'`. `{{{name}}}` and
  `{{&name}}` write the value as it is.

## What the render reads

The render reads standard C++ types and asks each type with its own
interface. There is no type of this library to fill in.

| your data | what Mustache sees |
|---|---|
| `std::string`, `std::string_view`, anything that converts to `std::string_view` | text |
| `int`, `double` and the other arithmetic types | text, written with `std::to_chars` |
| `bool` | a condition |
| `std::nullptr_t`, `std::monostate` | false |
| `std::optional<T>` | false when empty, else the `T` |
| `std::variant<…>`, or a type derived from one | the active alternative |
| a map with string keys, e.g. `std::map`, `std::unordered_map` | a map; `{{a.b}}` looks keys up with `find` |
| a random-access range, e.g. `std::vector` | a list |

The map is yours to choose, and the render finds a key with the `find`
of that map. A `std::unordered_map` compares a key once after its hash;
a `std::map` compares it with several keys of its tree. On the stocks
page of `bench/`, with g++ 16 at `-O2`, the same data rendered in
8.9 µs from `std::unordered_map` and in 9.8 µs from `std::map`.

A value that holds itself needs a named type. Derive it from the
variant:

```cpp
struct Node : std::variant<std::nullptr_t, bool, double, std::string, std::vector<Node>, std::map<std::string, Node>> {
  using variant::variant;
};
```

## Sections, lists, partials

```cpp
#include <mustache/mustache.hpp>

#include <cstdio>
#include <map>
#include <string>
#include <variant>
#include <vector>

struct Item {
  std::string name;
  double      price;
};

using Field = std::variant<std::string, double, bool, std::vector<std::map<std::string, std::variant<std::string, double>>>>;

int main()
{
  const auto page = mustache::compile("<h1>{{title}}</h1>\n"
                                      "{{#items}}{{> item}}{{/items}}"
                                      "{{^items}}<p>The cart is empty.</p>{{/items}}"
                                      "{{#member}}<p>Members pay no shipping.</p>\n{{/member}}");
  const auto item = mustache::compile("<li>{{name}}: {{price}} EUR</li>\n");
  if (!page || !item) return 1;

  mustache::Partials partials;
  partials.emplace("item", *item);

  const std::map<std::string, Field> data{
      {"title", std::string("Cart")},
      {"items", std::vector<std::map<std::string, std::variant<std::string, double>>>{
                    {{"name", std::string("Tea")}, {"price", 4.5}}, {{"name", std::string("Mug")}, {"price", 12.0}}}},
      {"member", true}};

  const auto answer = mustache::render(*page, data, partials);
  if (!answer) return 1;
  std::printf("%s", answer->c_str());
}
```

Output:

```
<h1>Cart</h1>
<li>Tea: 4.5 EUR</li>
<li>Mug: 12 EUR</li>
<p>Members pay no shipping.</p>
```

- `{{#items}}` repeats its body for each element of a list, enters a
  map, or shows its body when a condition is true.
- `{{^items}}` shows its body when the value is false or an empty list.
- `{{> item}}` renders the partial of that name from `Partials`.

## Escaping by media type

`compile(source, media_type)` takes the media type the caller
negotiated and escapes every `{{tag}}` for that format:

| media type | escaping |
|---|---|
| `text/html`, `application/xhtml+xml` | HTML: `&amp; &lt; &gt; &quot; &#39;` |
| `application/xml`, `text/xml`, `…+xml` | XML: the five predefined entities |
| `application/json`, `…+json` | a JSON string (RFC 8259) |
| `text/javascript` | a JavaScript string: `\xHH`, `\u2028`, `\u2029` |
| `text/css` | a CSS string: hex escapes |

The escaping makes a value safe inside a text in HTML and XML, or
inside a string in JSON, JavaScript and CSS. Where a tag stands in the
template is the author's choice, as in Mustache. A media type that is
not in the table does not compile. `image/svg+xml` is refused, because
SVG carries script that XML escaping does not guard.

## Render a C++ struct with reflection

With a C++26 compiler with reflection, the template can be compiled at
compile time and a struct is read by its members:

```cpp
#include <mustache/mustache.hpp>

#include <cstdio>
#include <optional>
#include <string>
#include <vector>

struct Item {
  std::string name;
  double      price;
};

struct Page {
  std::string                title;
  std::vector<Item>          items;
  std::optional<std::string> note;
};

int main()
{
  const Page page{"Cart", {{"Tea", 4.5}, {"Mug", 12}}, std::nullopt};
  const auto answer = mustache::render<"<h1>{{title}}</h1>\n{{#items}}<li>{{name}}: {{price}} EUR</li>\n{{/items}}"
                                       "{{#note}}<p>{{note}}</p>\n{{/note}}">(page);
  if (!answer) return 1;
  std::printf("%s", answer->c_str());

  const auto json = mustache::render<"{\"title\":\"{{title}}\"}\n">(page, "application/json");
  if (json) std::printf("%s", json->c_str());
}
```

Output:

```
<h1>Cart</h1>
<li>Tea: 4.5 EUR</li>
<li>Mug: 12 EUR</li>
{"title":"Cart"}
```

- Every type of the table above works here too; a struct is read by its
  members.
- A key that names no member is a compile error that names the key.
- This render is the fast one: the text of the template is copied with
  lengths known at compile time, and a list element is written in one
  piece.

## Compile a template for one struct type

A template that arrives at run time can be compiled for the type of the
data it will read. Every key is then resolved to a member once, and the
render reads the member without a search:

```cpp
#include <mustache/mustache.hpp>

#include <cstdio>
#include <string>
#include <vector>

struct Item {
  std::string name;
  double      price;
};

struct Page {
  std::string       title;
  std::vector<Item> items;
};

int main()
{
  const auto t = mustache::compile<Page>("<h1>{{title}}</h1>\n{{#items}}<li>{{name}}: {{price}} EUR</li>\n{{/items}}");
  if (!t) return 1;
  const Page page{"Cart", {{"Tea", 4.5}, {"Mug", 12}}};
  const auto answer = mustache::render(*t, page);
  if (answer) std::printf("%s", answer->c_str());
}
```

- `compile<T>` gives a `mustache::TypedTemplate<T>`, and `render` takes
  it with a `const T &`.
- A key that names no member is an error of `compile<T>`, with the op
  where the key stands.
- A key in a map, in a variant or in a partial is found at render time,
  as in the run-time render.
- The members come from reflection where the compiler has it, and from
  Boost.PFR 1.84 or later where it does not. With Boost.PFR, a struct
  must be an aggregate: no constructor of its own, no base, no private
  member.
- On the stocks page, with clang 22 at `-O2`, a typed template rendered
  in 4.3 µs, the run-time render with `std::unordered_map` in 6.9 µs.

## Errors

`compile` and `render` give back a `std::expected`. The error holds:

| field | meaning |
|---|---|
| `fault` | a `mustache::Fault`, below |
| `what` | a message, a static string |
| `asked` | the offset in the source for `parse`; the size asked for otherwise |
| `allowed` | the limit, for `over_limit` |

| Fault | when |
|---|---|
| `parse` | the template does not compile |
| `not_text` | `{{x}}` names a list or a map |
| `too_deep` | sections nest deeper than 32, or partials deeper than 64 |
| `over_work` | the render does more work than its limit |
| `over_limit` | the answer is larger than its limit |
| `no_memory` | an allocation failed |

The work limit counts each run of a template body. It stops a small
template with partials that call each other before it takes the
machine.

## What the library does not do

- Lambdas (`~lambdas` in the specification). Data never runs code in a
  render.
- Dynamic names (`~dynamic-names`). The data never picks a template:
  a compiled template fixes what it renders. With reflection, a name
  that the data gives would also need every partial built at every
  such tag, and the keys of each one checked against a context it may
  never see.

The specification is in `refs/mustache-spec/`, byte for byte.

## Tests and benchmarks

```sh
cmake -S . -B build -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined"
cmake --build build
ctest --test-dir build
```

The tests need simdjson and GoogleTest. They run every case of the specification
through the C++ render.

`bench/build.sh` builds one benchmark binary for each workload and each
compiler, and `bench/run.sh` runs them. They need Google Benchmark.

## License

Apache-2.0.
