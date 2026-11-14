#include <mustache/out.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <random>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using mustache::Context;

// Every member of the Context enum. A writer that the test does not reach
// is a writer that no test holds to its room.
constexpr std::array kContexts = {Context::html_text,  Context::xml_text,   Context::json_string,
                                  Context::js_string,  Context::css_string, Context::troff_text};

// Mustache spec, interpolation: {{name}} escapes HTML. These five
// replacements are the rule, written here from the spec and not taken
// from the table under test. The exact size of an html value comes from
// this rule.
std::string
escaped_by_rule(const std::string_view s)
{
  std::string r;
  for (const char c : s) {
    if (c == '&') r += "&amp;";
    else if (c == '<') r += "&lt;";
    else if (c == '>') r += "&gt;";
    else if (c == '"') r += "&quot;";
    else if (c == '\'') r += "&#39;";
    else r += c;
  }
  return r;
}

// The longest piece that the rule of each context writes for one input
// byte. The room of a context other than html is this length for each
// input byte. Each piece is written from its rule and not from the table
// under test.
std::string_view
longest_piece_of(const Context c)
{
  switch (c) {
    // Mustache spec: the longest of the five replacements.
    case Context::html_text:   return "&quot;";
    // XML 1.0, 4.6: the predefined entities; &quot; and &apos; are longest.
    case Context::xml_text:    return "&apos;";
    // RFC 8259, 7: a control character without a short form is \u00XX.
    case Context::json_string: return "\\u001f";
    // A byte that is not plain is \xXX. U+2028 and U+2029 are \u2028
    // and \u2029: six bytes for three input bytes, so two for each.
    case Context::js_string:   return "\\x3c";
    // A byte that is not plain is \XX and a space that ends the escape.
    case Context::css_string:  return "\\3c ";
    // A line of one space: \& before it, "\ " for the space at the end of
    // the line, and \& after it because the line ends in a space.
    case Context::troff_text:  return "\\&\\ \\&";
  }
  return {};
}

constexpr size_t kLongestPiece = std::string_view("&quot;").size();

// The room that the rule asks for: nothing for an empty value; else the
// exact size for html, the longest piece for each byte in the other
// contexts, and the bytes that a writer may store past its count.
size_t
room_by_rule(const Context c, const std::string_view s)
{
  if (s.empty()) return 0;
  const size_t written = c == Context::html_text ? escaped_by_rule(s).size() : longest_piece_of(c).size() * s.size();
  return written + mustache::kBytesPastTheWrittenCount;
}

// The random strings reach this length. It is three AVX2 vectors.
constexpr size_t kRandomLengthMax = 96;

constexpr size_t kLongestInput = std::max(kRandomLengthMax, 3 * mustache::kVectorBytes);

constexpr size_t kGuardBytes = 1;

constexpr size_t kRoomMax = kLongestPiece * kLongestInput + mustache::kBytesPastTheWrittenCount;

constexpr size_t kArenaBytes = kGuardBytes + kRoomMax + kGuardBytes;

// Two fills: a stray write of one fill value shows with the other.
constexpr std::array<char, 2> kFills = {'\x5a', '\xa5'};

struct Written {
  bool        inside;
  bool        tail_untouched;
  std::string text;
};

// Runs write on a room of exactly room_bytes inside a larger arena. inside
// says that no byte before or behind the room changed and that the count
// lies in the room. tail_untouched says that no byte of the room past the
// count changed.
template <class Write>
Written
written_in_room(const size_t room_bytes, const Write &write)
{
  if (room_bytes > kRoomMax) {
    ADD_FAILURE() << "the arena holds no room of " << room_bytes << " bytes";
    return {false, false, {}};
  }
  Written result{true, true, {}};
  for (const char fill : kFills) {
    std::array<char, kArenaBytes> arena;
    arena.fill(fill);
    const std::span<char> whole(arena);
    const std::span<char> room = whole.subspan(kGuardBytes, room_bytes);
    const char *const end = write(room);
    const std::ptrdiff_t count = end - room.data();
    const auto untouched = [fill](const char b) { return b == fill; };
    const bool inside = std::ranges::all_of(whole.first(kGuardBytes), untouched) &&
                        std::ranges::all_of(whole.subspan(kGuardBytes + room_bytes), untouched) && count >= 0 &&
                        std::cmp_less_equal(count, room_bytes);
    result.inside = result.inside && inside;
    if (!inside) return result;
    const size_t written = static_cast<size_t>(count);
    result.tail_untouched = result.tail_untouched && std::ranges::all_of(room.subspan(written), untouched);
    result.text.assign(room.data(), written);
  }
  return result;
}

std::string
described(const std::string_view s)
{
  std::string r = "input of " + std::to_string(s.size()) + " bytes:";
  for (const char c : s) r += " " + std::to_string(static_cast<unsigned char>(c));
  return r;
}

std::string
described(const Context c, const std::string_view s)
{
  return "context " + std::to_string(std::to_underlying(c)) + ", " + described(s);
}

// The writer of each context stays inside the smallest room that
// room_holds_the_escaped_value accepts. The room one byte smaller is
// refused, so that smallest room is the room of the rule.
void
expect_the_escape_stays_in_its_room(const Context c, const std::string &s)
{
  const size_t required = room_by_rule(c, s);
  EXPECT_TRUE(mustache::room_holds_the_escaped_value(c, s, required)) << described(c, s);
  if (required > 0) {
    EXPECT_FALSE(mustache::room_holds_the_escaped_value(c, s, required - 1)) << described(c, s);
  }
  EXPECT_EQ(mustache::room_holds_the_escaped_value(c, s, 0), s.empty()) << described(c, s);
  if (!s.empty()) {
    EXPECT_EQ(mustache::room_needed_for_the_escaped_value(c, s) + mustache::kBytesPastTheWrittenCount, required)
        << described(c, s);
  }

  const Written w = written_in_room(required, [c, &s](const std::span<char> room) {
    return mustache::detail::escaped_into_in(c, room.data(), s);
  });
  EXPECT_TRUE(w.inside) << described(c, s);
  if (!w.inside) return;
  if (c == Context::html_text) {
    EXPECT_EQ(w.text, escaped_by_rule(s)) << described(c, s);
    const Written e = written_in_room(required, [&s](const std::span<char> room) {
      return room.subspan(mustache::escape_string_into(room, s)).data();
    });
    EXPECT_TRUE(e.inside) << described(c, s);
    if (e.inside) {
      EXPECT_EQ(e.text, escaped_by_rule(s)) << described(c, s);
    }
    return;
  }
  // The five writers other than html write each piece byte by byte, so
  // they write nothing past their count.
  EXPECT_TRUE(w.tail_untouched) << described(c, s);
  EXPECT_LE(w.text.size(), longest_piece_of(c).size() * s.size()) << described(c, s);
}

// The copy rule refuses a room one byte smaller than the value. Each
// caller of copied_string proves kCopiedStringMax bytes of room: Batch
// counts them for each value in batch_bound, and Answer keeps
// kBytesPastTheWrittenCount bytes behind its size. copied_string may store
// a whole block in that room, so the test gives it that room and holds it
// to the guard bytes around it. An empty value writes nothing, so it gets
// no room. A value that is not copied leaves the room as it was.
void
expect_the_copy_stays_in_its_room(const std::string &s)
{
  const size_t required = s.size();
  EXPECT_TRUE(mustache::room_holds_the_copied_value(s, required)) << described(s);
  if (required > 0) {
    EXPECT_FALSE(mustache::room_holds_the_copied_value(s, required - 1)) << described(s);
  }
  size_t count = 0;
  const size_t proven = s.empty() ? 0 : mustache::kCopiedStringMax;
  const Written w = written_in_room(proven, [&s, &count](const std::span<char> room) {
    count = mustache::copied_string(room, s).count;
    return room.subspan(count).data();
  });
  EXPECT_TRUE(w.inside) << described(s);
  const bool copied = s.size() <= mustache::kCopiedStringMax;
  EXPECT_EQ(count, copied ? s.size() : 0) << described(s);
  if (copied) {
    EXPECT_EQ(w.text, s) << described(s);
  }
  else {
    EXPECT_TRUE(w.tail_untouched) << described(s);
  }
}

// copied_string may read a value in the capacity of its string. The bytes
// between size() and capacity() are written with a mark through
// resize_and_overwrite, which lets the operation write every byte of the
// count that it is given; that count is the capacity of the string. The
// string holds its value in its own buffer, or on the heap when reserved
// says so. The bytes behind the size must not change the mark of the
// value, and the copy stays in the proven room.
void
expect_the_bytes_behind_the_size_change_nothing(const size_t n, const size_t mark_at, const size_t reserved)
{
  std::string s;
  s.reserve(reserved);
  s.resize_and_overwrite(std::max(n, s.capacity()), [n, mark_at](char *const p, const size_t capacity) {
    const std::span<char> all(p, capacity);
    std::ranges::fill(all, '<');
    std::ranges::fill(all.first(n), 'x');
    if (mark_at < n) all.subspan(mark_at).front() = '&';
    return n;
  });
  const bool marked = mark_at < n;
  size_t count = 0;
  bool found = false;
  const Written w = written_in_room(n == 0 ? 0 : mustache::kCopiedStringMax, [&s, &count, &found](const std::span<char> room) {
    const mustache::Copied c = mustache::copied_string(room, s);
    count = c.count;
    found = c.marks != 0;
    return room.subspan(count).data();
  });
  EXPECT_TRUE(w.inside) << described(s);
  EXPECT_EQ(count, n) << described(s);
  EXPECT_EQ(found, marked) << described(s);
  EXPECT_EQ(w.text, s) << described(s);
}

void
expect_every_writer_stays_in_its_room(const std::string &s)
{
  for (const Context c : kContexts) expect_the_escape_stays_in_its_room(c, s);
  expect_the_copy_stays_in_its_room(s);
}

constexpr size_t kByteValues = 256;

std::string
byte_of(const size_t value)
{
  return std::string(1, static_cast<char>(static_cast<unsigned char>(value)));
}

// The empty value needs no room. A room of 0 bytes holds it, and no
// writer writes into the arena around that room.
TEST(Room, EmptyValue)
{
  expect_every_writer_stays_in_its_room("");
}

// Each byte value is plain or marked in each context. A byte alone is the
// shortest input; at each position of one vector it reaches every lane of
// the SIMD escape.
TEST(Room, EveryByteAloneAndAtEveryPositionOfAVector)
{
  for (size_t b = 0; b < kByteValues; b++) {
    expect_every_writer_stays_in_its_room(byte_of(b));
    for (size_t at = 0; at < mustache::kVectorBytes; at++) {
      std::string s(mustache::kVectorBytes, 'x');
      s.replace(at, 1, byte_of(b));
      expect_every_writer_stays_in_its_room(s);
    }
  }
}

// Pieces that change what a writer does next to them: the five marks of
// html (entities of 4, 5 and 6 bytes), U+2028 and U+2029 for js, also cut
// after e2 and after e2 80, and the lines of troff: one byte that is first
// and last of its line, an empty line, a sentence mark before closing
// delimiters. Each piece stands at every position of every length up to
// three vectors, and is cut where the input ends. This reaches a mark as
// the last byte of a block when the length is a multiple of the vector,
// and both sides of the copy of the last block to the stack.
TEST(Room, EveryPieceAtEveryPositionOfEveryLength)
{
  const std::array<std::string_view, 21> pieces = {
      "&",        "<",        ">",       "\"",     "'",         "\xe2\x80\xa8", "\xe2\x80\xa9",
      "\xe2",     "\xe2\x80", " ",       ".",      "\n",        "\\",           "\n \n",
      "\n.\n",    "\n\"\n",   "\n\n",    ".)\"",   ".)\"\n",    "!]'",          "\x01",
  };
  for (size_t n = 0; n <= 3 * mustache::kVectorBytes; n++) {
    expect_every_writer_stays_in_its_room(std::string(n, 'x'));
    for (size_t at = 0; at < n; at++) {
      for (const std::string_view piece : pieces) {
        std::string s(n, 'x');
        s.replace(at, std::min(piece.size(), n - at), piece.substr(0, n - at));
        expect_every_writer_stays_in_its_room(s);
      }
    }
  }
}

// The SIMD escape takes the scalar form for a block with more than
// kMarksPerVectorMax marks. The count of marks in one block runs from 0 to
// a full block, at the start and at the end of the block, in each block of
// inputs around the length at which the last block is copied to the stack.
TEST(Room, EveryCountOfMarksInABlock)
{
  constexpr std::string_view kMarks = "&<>\"'";
  const size_t v = mustache::kVectorBytes;
  for (const size_t n : {v, 2 * v - 1, 2 * v, 2 * v + 1, 3 * v}) {
    for (size_t block = 0; block + v <= n; block += v) {
      for (size_t marks = 0; marks <= v; marks++) {
        for (const bool at_the_start : {true, false}) {
          std::string mixed(n, 'x');
          std::string quotes(n, 'x');
          const size_t first = at_the_start ? block : block + v - marks;
          for (size_t i = 0; i < marks; i++) {
            mixed.at(first + i) = kMarks.at(i % kMarks.size());
            quotes.at(first + i) = '"';
          }
          expect_every_writer_stays_in_its_room(mixed);
          expect_every_writer_stays_in_its_room(quotes);
        }
      }
    }
  }
}

// copied_string takes a different path below 16 bytes, from 16 to 32
// bytes, and above kCopiedStringMax. Each length to one past that limit
// runs with a mark at the end and without one.
TEST(Room, CopiedStringAtEveryLength)
{
  for (size_t n = 0; n <= mustache::kCopiedStringMax + 1; n++) {
    std::string plain(n, 'x');
    expect_the_copy_stays_in_its_room(plain);
    if (n > 0) plain.back() = '<';
    expect_the_copy_stays_in_its_room(plain);
  }
}

// Each length to kCopiedStringMax, with no mark and with a mark at each
// position, in a string with its own buffer and in a string on the heap.
TEST(Room, CopiedStringIgnoresTheBytesBehindTheSize)
{
  for (const size_t reserved : {size_t{0}, 2 * mustache::kCopiedStringMax}) {
    for (size_t n = 0; n <= mustache::kCopiedStringMax; n++) {
      for (size_t mark_at = 0; mark_at <= n; mark_at++) expect_the_bytes_behind_the_size_change_nothing(n, mark_at, reserved);
    }
  }
}

// Random strings find the mixes that the cases above do not list. The seed
// is fixed so that a failure shows again. Each string takes a share of
// bytes from a set of marks and line pieces, or any byte value, and the
// rest from plain letters.
TEST(Room, RandomStrings)
{
  constexpr uint32_t kSeed = 20261113;
  constexpr size_t kStringsPerLength = 3000;
  constexpr std::array<uint32_t, 4> kMarkedPercent = {0, 12, 50, 100};
  constexpr uint32_t kPercent = 100;
  constexpr std::string_view kSpecial = "&<>\"'.!?)] \n\\`~^\xe2\x80\xa8\xa9\x01\x7f";
  constexpr std::string_view kPlain = "abcdefghijklmnopqrstuvwxyz";
  std::mt19937 random(kSeed);
  std::uniform_int_distribution<uint32_t> percent(0, kPercent - 1);
  std::uniform_int_distribution<size_t> share(0, kMarkedPercent.size() - 1);
  std::uniform_int_distribution<size_t> special(0, kSpecial.size());
  std::uniform_int_distribution<size_t> plain(0, kPlain.size() - 1);
  std::uniform_int_distribution<size_t> any_byte(0, kByteValues - 1);
  for (size_t n = 0; n <= kRandomLengthMax; n++) {
    for (size_t k = 0; k < kStringsPerLength; k++) {
      const uint32_t marked = kMarkedPercent.at(share(random));
      std::string s;
      for (size_t i = 0; i < n; i++) {
        if (percent(random) >= marked) s += kPlain.at(plain(random));
        else {
          const size_t pick = special(random);
          s += pick < kSpecial.size() ? std::string(1, kSpecial.at(pick)) : byte_of(any_byte(random));
        }
      }
      expect_every_writer_stays_in_its_room(s);
    }
  }
}

}
