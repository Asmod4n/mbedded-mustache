// Review probe: runs the writers of the tree in a room of exactly "required" bytes
// with guard bytes before and behind, and reports the real extent of the stores.
#include <mustache/out.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

using mustache::Context;

#if defined(__AVX2__) || defined(__ARM_NEON)
constexpr size_t kTail = mustache::kVectorBytes;
#else
constexpr size_t kTail = 8;
#endif
constexpr size_t kGuard = 64;
constexpr size_t kContexts = 6;

size_t required_of(const Context c, const std::string_view s)
{
  if (s.empty()) return 0;
  if (c == Context::html_text) return mustache::escaped_size_of(s) + kTail;
  return mustache::escaped_size_max_in(c, s.size()) + kTail;
}

struct Seen {
  size_t runs = 0;
  size_t guard_faults = 0;
  size_t over_bound = 0;
  size_t tail_max = 0;     // highest stored byte minus written count
  size_t piece_max = 0;
  std::string worst;
};

std::array<Seen, kContexts> seen;
std::array<Seen, 2> seen_string;   // escape_string_into, copied_string

std::vector<char> buffer;

// Runs the writer twice with two guard values; a byte that differs from its fill was stored.
template <class Writer>
void run(Seen &seen_here, const size_t required, const size_t bound, const std::string_view s, const Writer &writer)
{
  size_t extent = 0;
  size_t written = 0;
  bool guard_fault = false;
  for (const char fill : {'\x5a', '\xa5'}) {
    buffer.assign(kGuard + required + kGuard, fill);
    char *const room = buffer.data() + kGuard;
    written = writer(room);
    for (size_t i = 0; i < buffer.size(); i++) {
      if (buffer[i] == fill) continue;
      if (i < kGuard || i >= kGuard + required) guard_fault = true;
      else extent = std::max(extent, i - kGuard + 1);
    }
  }
  seen_here.runs++;
  if (guard_fault) {
    if (seen_here.guard_faults == 0) seen_here.worst = std::string(s);
    seen_here.guard_faults++;
  }
  if (written > bound) seen_here.over_bound++;
  if (extent > written && extent - written > seen_here.tail_max) seen_here.tail_max = extent - written;
}

void every_context(const std::string_view s)
{
  for (size_t k = 0; k < kContexts; k++) {
    const Context c = (Context)k;
    const size_t bound = c == Context::html_text ? mustache::escaped_size_of(s) : mustache::escaped_size_max_in(c, s.size());
    run(seen[k], required_of(c, s), bound, s,
        [&](char *const room) { return (size_t)(mustache::escaped_into_in(c, room, s) - room); });
  }
  const std::string held(s);
  const size_t required = required_of(Context::html_text, s);
  run(seen_string[0], required, mustache::escaped_size_of(s), s, [&](char *const room) {
    return (size_t)(mustache::escape_string_into(std::span<char>(room, required), held) - room);
  });
  if (s.size() <= mustache::kCopiedStringMax) {
    // the room of a batch value: kCopiedStringMax bytes
    run(seen_string[1], mustache::kCopiedStringMax, s.size(), s, [&](char *const room) {
      const mustache::Copied c = mustache::copied_string(std::span<char>(room, mustache::kCopiedStringMax), held);
      return c.w == nullptr ? size_t{0} : (size_t)(c.w - room);
    });
  }
}

uint32_t state = 12345;
uint32_t next()
{
  state = state * 1664525u + 1013904223u;
  return state >> 8;
}

void all_strings(const std::string_view alphabet, const size_t length_max)
{
  std::string s;
  std::vector<size_t> digit;
  for (size_t n = 0; n <= length_max; n++) {
    digit.assign(n, 0);
    for (;;) {
      s.resize(n);
      for (size_t i = 0; i < n; i++) s[i] = alphabet[digit[i]];
      every_context(s);
      size_t i = 0;
      while (i < n && ++digit[i] == alphabet.size()) digit[i++] = 0;
      if (i == n) break;
    }
  }
}

int main()
{
  constexpr size_t kLengthMax = 96;
  constexpr std::string_view kSpecial("&<>\"'\\ .\n`~^!?)]\x01\x1f\x7f\xe2\x80\xa8\xa9\b\f\r\t-_,x", 31);
  // 1: every length, one byte value 0..255 in a run of 'x', at every position; and the run of that byte alone.
  for (size_t n = 0; n <= kLengthMax; n++) {
    for (unsigned v = 0; v < 256; v++) {
      every_context(std::string(n, (char)v));
      for (size_t at = 0; at < n; at++) {
        std::string s(n, 'x');
        s[at] = (char)v;
        every_context(s);
      }
    }
  }
  // 2: every length, k marks (each of the special bytes) as the first k, the last k, and the last k of each block of 16.
  for (size_t n = 0; n <= kLengthMax; n++) {
    for (const char m : kSpecial) {
      for (size_t k = 0; k <= n; k++) {
        std::string first(n, 'x'), last(n, 'x'), each(n, 'x');
        for (size_t i = 0; i < k; i++) first[i] = m;
        for (size_t i = 0; i < k; i++) last[n - 1 - i] = m;
        for (size_t i = 0; i < n; i++) if (16 - 1 - i % 16 < k) each[i] = m;
        every_context(first);
        every_context(last);
        every_context(each);
      }
    }
  }
  // 3: every length, 3000 random strings: the share of special bytes is 3, 12, 25, 50 or 100 percent.
  for (size_t n = 0; n <= kLengthMax; n++) {
    for (size_t r = 0; r < 3000; r++) {
      const uint32_t percent = std::array<uint32_t, 5>{3, 12, 25, 50, 100}[r % 5];
      std::string s(n, 'x');
      for (char &c : s) if (next() % 100 < percent) c = kSpecial[next() % kSpecial.size()];
      every_context(s);
    }
  }
  // 4: every string up to 7 bytes over the bytes of the troff rules, and up to 6 over the bytes of the js triple.
  all_strings(" .\"x\n\\", 7);
  all_strings(std::string_view("\xe2\x80\xa8\xa9x<", 6), 6);
  // 5: every pattern of marked bytes for the lengths 0 to 12, with the longest entity.
  for (size_t n = 0; n <= 12; n++) {
    for (uint32_t bits = 0; bits < (1u << n); bits++) {
      std::string s(n, 'x');
      for (size_t i = 0; i < n; i++) if (bits >> i & 1) s[i] = '"';
      every_context(s);
    }
  }
  constexpr std::array<const char *, kContexts> kNames{"html", "xml", "json", "js", "css", "troff"};
  std::printf("tail T = %zu bytes\n", kTail);
  bool bad = false;
  for (size_t k = 0; k < kContexts; k++) {
    std::printf("%-6s runs %zu, guard changed in %zu, written over the bound in %zu, stores reach %zu bytes past the written count\n",
                kNames[k], seen[k].runs, seen[k].guard_faults, seen[k].over_bound, seen[k].tail_max);
    bad |= seen[k].guard_faults != 0 || seen[k].over_bound != 0;
  }
  std::printf("escape_string_into runs %zu, guard changed in %zu, stores reach %zu bytes past the written count\n",
              seen_string[0].runs, seen_string[0].guard_faults, seen_string[0].tail_max);
  std::printf("copied_string (room %zu) runs %zu, guard changed in %zu, stores reach %zu bytes past the written count\n",
              mustache::kCopiedStringMax, seen_string[1].runs, seen_string[1].guard_faults, seen_string[1].tail_max);
  bad |= seen_string[0].guard_faults != 0 || seen_string[1].guard_faults != 0;
  return bad ? 1 : 0;
}
