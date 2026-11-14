// What each arm does with a room that holds the exact output plus the slack, and what a
// run-time offset inside a block of fixed extent does.
#include "arms.hpp"
#include <cstdio>
#include <vector>

int main(int argc, char **)
{
  const std::string plain(64, 'a');
  std::vector<char> exact(plain.size() + mustache::kEscapeSlack);
  const auto a = probe::escaped_per_block(exact, plain);
  const auto b = probe::escaped_per_store(exact, plain);
  const auto c = probe::escaped_checked_once(exact, plain);
  std::printf("room %zu for 64 plain bytes: per block %s, per store %s, once %s; escaped_fits says %s\n", exact.size(),
              a ? "written" : "error value", b ? "written" : "error value", c ? "written" : "error value",
              "fits (64 <= 64)");
  std::array<char, probe::kBlockRoom + 64> guard{};
  const std::span<char, probe::kBlockRoom> block = std::span(guard).first<probe::kBlockRoom>();
  const size_t offset = static_cast<size_t>(argc) + probe::kBlockRoom;
  probe::stored(block.subspan(offset).first<probe::kVector>(), _mm256_set1_epi8('x'));
  std::printf("a store at offset %zu of a block of extent %zu compiled and ran; byte after the block: %c\n", offset,
              probe::kBlockRoom, guard.at(offset));
  return 0;
}
