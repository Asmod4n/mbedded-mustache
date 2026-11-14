#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <cstdio>
#include <span>
#include <string_view>

inline constexpr size_t kEntityWordBytes = 8;
inline constexpr size_t kTail = 32;

consteval uint64_t word_of(const std::string_view s)
{
  uint64_t v = 0;
  size_t i = 0;
  for (const char c : s) v |= uint64_t{static_cast<unsigned char>(c)} << (8 * i++);
  return v;
}

constexpr void stored_word(const std::span<char, kEntityWordBytes> o, const uint64_t word)
{
  const std::array<char, kEntityWordBytes> a = std::bit_cast<std::array<char, kEntityWordBytes>>(word);
  std::copy_n(a.begin(), a.size(), o.begin());
}

constexpr size_t xml_escaped_into(const std::span<char> room, const std::string_view s)
{
  size_t w = 0;
  for (const char c : s) {
    uint64_t word = static_cast<unsigned char>(c);
    size_t n = 1;
    if (c == '&') { word = word_of("&amp;"); n = 5; }
    else if (c == '"') { word = word_of("&quot;"); n = 6; }
    stored_word(room.subspan(w).first<kEntityWordBytes>(), word);
    w += n;
  }
  return w;
}

struct Out {
  char *w;
  char *end;
};

template <size_t Extra>
consteval size_t page_size()
{
  std::array<char, 12 + Extra> page{};
  Out out{page.data(), page.data() + page.size()};
  return xml_escaped_into(std::span<char>(out.w, out.end), "a\"&");
}

static_assert(page_size<kTail>() == 12);
#ifdef SHORT
static_assert(page_size<0>() == 12);
#endif

int main() { std::printf("ok\n"); }
