# mustache

Mustache templates for C++, with a C API for every other language.

The library compiles a template once and renders it many times. It
reads your data as standard C++ types, and it escapes HTML by default. It passes 163 cases of the Mustache specification. It
never runs code of the data it renders.

- C++23, header only: `include/mustache/mustache.hpp`.
- A reflection render for C++26 compilers with reflection: the
  template is compiled at compile time, and a key that names no member
  is a compile error.
- A C API for other languages: `include/mustache-c/mustache.h` and the
  library `mustache-c`.

## Contents

1. [Install](#install)
2. [A first render](#a-first-render)
3. [What the render reads](#what-the-render-reads)
4. [Sections, lists, partials](#sections-lists-partials)
5. [Escaping by media type](#escaping-by-media-type)
6. [Render a C++ struct with reflection](#render-a-c-struct-with-reflection)
7. [Errors](#errors)
8. [The C API](#the-c-api)
9. [What the library does not do](#what-the-library-does-not-do)
10. [Tests and benchmarks](#tests-and-benchmarks)

## Install

The library needs a C++20 compiler and CMake 3.25 or later. The
reflection render needs a compiler with C++26 reflection, for example
g++ 16 with `-std=c++26 -freflection`.

CMake exports two targets:

| target | what it is |
|---|---|
| `mustache::mustache` | the C++ headers, an INTERFACE library |
| `mustache::mustache-c` | the C API, a compiled library |

### With FetchContent

```cmake
include(FetchContent)
FetchContent_Declare(mustache
  GIT_REPOSITORY https://github.com/Asmod4n/mruby-mustache.git
  GIT_TAG mustache-cpp)
FetchContent_MakeAvailable(mustache)

target_link_libraries(app PRIVATE mustache::mustache)
```

### As an installed package

```sh
git clone --recursive -b mustache-cpp https://github.com/Asmod4n/mruby-mustache.git mustache
cmake -S mustache -B build -DBUILD_TESTING=OFF
cmake --build build
cmake --install build --prefix /usr/local
```

```cmake
find_package(mustache REQUIRED)
target_link_libraries(app PRIVATE mustache::mustache)      # C++
target_link_libraries(app-c PRIVATE mustache::mustache-c)  # C
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

## The C API

A function gives 0 or a code, or -1 with `errno` set. The template is a
handle of an incomplete type, and each callback is set one by one, so a
foreign function interface builds no struct. Python ctypes, Ruby
Fiddle, LuaJIT and Java Panama can call it as it is.

```c
#include <mustache-c/mustache.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The data of the caller: one map, the root, with two text values. A
 * value is any pointer the caller chooses; the core only passes it back. */
struct pair {
  const char *key;
  const char *text;
};

static const struct pair data[] = {{"name", "Ada"}, {"city", "London"}};
static const char root[] = "the map";

static const void *
find(void *user, const void *value, const char *key, size_t size)
{
  size_t i;
  (void)user;
  if (value != root) return NULL;
  for (i = 0; i < 2; i++)
    if (strlen(data[i].key) == size && memcmp(data[i].key, key, size) == 0) return &data[i];
  return NULL;
}

static int
kind(void *user, const void *value)
{
  (void)user;
  return value == root ? MUSTACHE_KIND_MAP : MUSTACHE_KIND_TEXT;
}

static const char *
text(void *user, const void *value, size_t *size)
{
  const struct pair *p = value;
  (void)user;
  *size = strlen(p->text);
  return p->text;
}

static size_t
size_of(void *user, const void *value)
{
  (void)user;
  (void)value;
  return 0;
}

static const void *
element(void *user, const void *value, size_t index)
{
  (void)user;
  (void)index;
  return value;
}

static const mustache_template *
partial(void *user, const char *name, size_t size)
{
  (void)user;
  (void)name;
  (void)size;
  return NULL;
}

/* The string is the caller's. The render writes into it and asks for
 * more room when it needs it. */
static char *
new_string(void *user, size_t capacity, void **string, size_t *real_capacity)
{
  char *bytes = malloc(capacity);
  (void)user;
  *string = bytes;
  *real_capacity = capacity;
  return bytes;
}

static char *
grow(void *user, void **string, size_t size, size_t capacity, size_t *real_capacity)
{
  char *bytes = realloc(*string, capacity);
  (void)user;
  (void)size;
  if (bytes == NULL) return NULL;
  *string = bytes;
  *real_capacity = capacity;
  return bytes;
}

static int
done(void *user, void *string, size_t size)
{
  (void)user;
  ((char *)string)[size] = '\0';
  return 0;
}

int
main(void)
{
  const char *source = "{{name}} lives in {{city}}.";
  mustache_template *tpl = NULL;
  void *string = NULL;
  int next;

  if (mustache_compile(source, strlen(source), &tpl) != 0) {
    const char *message;
    size_t size;
    mustache_message(tpl, &message, &size);
    fprintf(stderr, "%.*s\n", (int)size, message);
    mustache_dispose_template(tpl);
    return 1;
  }

  /* mustache_next names the next setter to call, until every callback
   * is set. */
  while ((next = mustache_next(tpl)) != MUSTACHE_RENDER) {
    switch (next) {
      case MUSTACHE_SET_FIND:       mustache_set_find(tpl, find); break;
      case MUSTACHE_SET_KIND:       mustache_set_kind(tpl, kind); break;
      case MUSTACHE_SET_TEXT:       mustache_set_text(tpl, text); break;
      case MUSTACHE_SET_SIZE:       mustache_set_size(tpl, size_of); break;
      case MUSTACHE_SET_ELEMENT:    mustache_set_element(tpl, element); break;
      case MUSTACHE_SET_PARTIAL:    mustache_set_partial(tpl, partial); break;
      case MUSTACHE_SET_NEW_STRING: mustache_set_new_string(tpl, new_string); break;
      case MUSTACHE_SET_GROW:       mustache_set_grow(tpl, grow); break;
      case MUSTACHE_SET_DONE:       mustache_set_done(tpl, done); break;
      default:                      return 1;
    }
  }

  if (mustache_render(tpl, root, NULL, NULL, &string) != 0) {
    fprintf(stderr, "the render failed: errno %d\n", errno);
    mustache_dispose_template(tpl);
    return 1;
  }
  printf("%s\n", (char *)string);
  free(string);
  mustache_dispose_template(tpl);
  return 0;
}
```

Output:

```
Ada lives in London.
```

- `mustache_next` names the setter that is still missing, or gives
  `MUSTACHE_RENDER`. A render with a missing callback does not start:
  it gives the same code.
- A value is any pointer of the caller. The core only passes it back.
- The caller owns the string. `done` comes after every render that got a
  string, also after a failure.
- `mustache_render` owns the root it is given. It calls `release` when
  the render ends, whether it succeeded or not.
- `mustache_message` gives the message of the last failure.

| errno | why |
|---|---|
| `EILSEQ` | the template does not compile |
| `EINVAL` | `{{x}}` names a list or a map, or an argument is wrong |
| `ELOOP` | nesting too deep |
| `ETIME` | the render does more work than its limit |
| `E2BIG` | the answer is larger than its limit |
| `ENOMEM` | an allocation failed |
| `EBADF` | the handle is not a template |
| `EBUSY` | the template renders already, for example from a callback |

## What the library does not do

- Lambdas (`~lambdas` in the specification). Data never runs code in a
  render.
- Dynamic names (`~dynamic-names`). The data never picks a template.

The specification is in `refs/mustache-spec/`, byte for byte.

## Tests and benchmarks

```sh
cmake -S . -B build -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined"
cmake --build build
ctest --test-dir build
```

The tests need simdjson and GoogleTest. They run every case of the specification
through the C++ render and through the C API.

`bench/build.sh` builds one benchmark binary for each workload and each
compiler, and `bench/run.sh` runs them. They need Google Benchmark.

## License

Apache-2.0.
