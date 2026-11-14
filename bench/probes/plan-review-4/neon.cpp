#include <arm_neon.h>
#include <algorithm>
#include <array>
#include <bit>
#include <span>
uint8x16_t loaded(const std::span<const char, 16> b)
{
  std::array<char, 16> a;
  std::copy_n(b.begin(), b.size(), a.begin());
  return std::bit_cast<uint8x16_t>(a);
}
void stored(const std::span<char, 16> o, const uint8x16_t v)
{
  const std::array<char, 16> a = std::bit_cast<std::array<char, 16>>(v);
  std::copy_n(a.begin(), a.size(), o.begin());
}
