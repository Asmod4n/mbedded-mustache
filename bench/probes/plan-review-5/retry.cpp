// The retry form of plan form 5: the whole render runs inside resize_and_overwrite.
#include <cstdio>
#include <expected>
#include <span>
#include <string>
#include <string_view>
#include <algorithm>
#include <stdexcept>
enum class Fault { none, over_limit, not_text };
struct Error { Fault fault; size_t asked; };
static int walks = 0;
// a render into a span: gives the count, or the error; "full" is over_limit with asked = the needed size
static std::expected<size_t, Error> render_to(const std::span<char> room, const std::string_view data, const bool bad, const bool throws)
{
  walks++;
  if (bad) return std::unexpected(Error{Fault::not_text, 0});
  if (throws) throw std::logic_error("thrown inside the operation");
  if (room.size() < data.size()) return std::unexpected(Error{Fault::over_limit, data.size()});
  std::copy_n(data.begin(), data.size(), room.begin());
  return data.size();
}
static std::expected<std::string, Error> render(const std::string_view data, const size_t hint, const bool bad = false, const bool throws = false)
{
  std::string bytes;
  size_t capacity = hint;
  for (;;) {
    std::expected<size_t, Error> got = size_t{0};
    bytes.resize_and_overwrite(capacity, [&](char *const p, const size_t n) {
      got = render_to(std::span<char>(p, n), data, bad, throws);
      return got ? *got : size_t{0};
    });
    if (got) return bytes;
    if (got.error().fault != Fault::over_limit) return std::unexpected(got.error());
    capacity = got.error().asked;
  }
}
int main()
{
  const std::string data(3000, 'x');
  walks = 0; auto a = render(data, 4096);
  printf("good hint: ok %d size %zu capacity %zu walks %d\n", a.has_value(), a->size(), a->capacity(), walks);
  walks = 0; auto b = render(data, 1024);
  printf("small hint: ok %d size %zu capacity %zu walks %d\n", b.has_value(), b->size(), b->capacity(), walks);
  auto c = render(data, 1024, true);
  printf("failing render: ok %d fault %d\n", c.has_value(), int(c.error().fault));
  try { auto d = render(data, 4096, false, true); printf("no throw\n"); }
  catch (const std::logic_error &e) { printf("a throw left the operation: %s\n", e.what()); }
}
