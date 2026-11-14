// Probe: the five scalar writers of include/mustache/escape.hpp (xml, json, js, css, troff) on the escape alone.
// One binary times one arm, chosen with -DARM=<number>; each arm is held to the arm "before" first.
// Arms: 0 and 1 the writers of 4dac08e (the same code twice, for the noise), 2 each piece of output, a plain
// byte too, written as one 8-byte word through std::span<char, 8>, std::bit_cast and std::copy_n, the count
// moves on by the length of the piece, and the word of each byte comes from a table of 256 words for each
// context, built at compile time, as the html writer reads kEntities; 3 arm 2 with each run of plain bytes
// copied with std::copy_n into the room and only the escaped bytes written as words from the same tables;
// 4 arm 2 with a class in place of a word: for each context a table of 256 classes of one byte (0 the byte
// stays, 1 a hex form made from the byte, from 2 on, or from 1 on in a context with no hex form, the number of
// a fixed replacement) and a table of the words of the classes; the word of a byte is the word of its class,
// with the byte put in by a mask where the class is 0 and the two hex digits, made by arithmetic, put in by a
// mask where the class is 1; 5 arm 4 with the classes of all five contexts in bit fields of one table of 256
// entries of 16 bits; 6 the same code as arm 2, a second binary: the noise of the cell.
// Each arm writes into a room of escaped_size_max_in(context, n) + 32 bytes, as the callers in out.hpp prove.
#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <print>
#include <random>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#ifndef ARM
#error "ARM names the arm"
#endif

namespace probe {

enum class Context : uint32_t { html_text = 0, xml_text, json_string, js_string, css_string, troff_text };

inline constexpr std::array kContexts = {Context::xml_text, Context::json_string, Context::js_string, Context::css_string,
                                         Context::troff_text};

inline constexpr size_t kEntityMax = 6;
inline constexpr size_t kEntityWordBytes = sizeof(uint64_t);
inline constexpr size_t kBytesPastTheWrittenCount = 32;

constexpr size_t
escaped_size_max_in(const Context c, const size_t n)
{
  switch (c) {
    case Context::css_string:
    case Context::js_string: return 4 * n;
    default: return kEntityMax * n;
  }
}

constexpr bool
is_js_plain(const char c)
{
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == ' ' || c == '_' ||
         c == ',' || c == '.' || (unsigned char)c >= 0x80;
}

constexpr bool
is_css_plain(const char c)
{
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == ' ' || c == '_' ||
         c == '-' || c == ',' || c == '.' || (unsigned char)c >= 0x80;
}

constexpr bool
is_sentence_mark(const char c)
{
  return c == '.' || c == '!' || c == '?';
}

constexpr bool
is_closing_delimiter(const char c)
{
  return c == ')' || c == ']' || c == '\'' || c == '"';
}

inline constexpr std::string_view kHexDigits = "0123456789abcdef";

// The writers of include/mustache/escape.hpp at 4dac08e, as they stand there.
namespace before {

constexpr char *
bytes_into(char *w, const std::string_view s)
{
  return std::ranges::copy(s, w).out;
}

constexpr char *
xml_escaped_into(char *w, const std::string_view s)
{
  for (const char c : s) {
    if (c == '&') w = bytes_into(w, "&amp;");
    else if (c == '<') w = bytes_into(w, "&lt;");
    else if (c == '>') w = bytes_into(w, "&gt;");
    else if (c == '"') w = bytes_into(w, "&quot;");
    else if (c == '\'') w = bytes_into(w, "&apos;");
    else *w++ = c;
  }
  return w;
}

constexpr char *
json_string_escaped_into(char *w, const std::string_view s)
{
  for (const char c : s) {
    const unsigned char u = (unsigned char)c;
    if (c == '"') w = bytes_into(w, "\\\"");
    else if (c == '\\') w = bytes_into(w, "\\\\");
    else if (c == '\b') w = bytes_into(w, "\\b");
    else if (c == '\f') w = bytes_into(w, "\\f");
    else if (c == '\n') w = bytes_into(w, "\\n");
    else if (c == '\r') w = bytes_into(w, "\\r");
    else if (c == '\t') w = bytes_into(w, "\\t");
    else if (u < 0x20) {
      w = bytes_into(w, "\\u00");
      *w++ = kHexDigits.at(u >> 4);
      *w++ = kHexDigits.at(u & 15);
    }
    else *w++ = c;
  }
  return w;
}

constexpr char *
js_string_escaped_into(char *w, const std::string_view s)
{
  for (size_t i = 0; i < s.size(); i++) {
    const unsigned char u = (unsigned char)s.at(i);
    if (u == 0xe2 && i + 2 < s.size() && (unsigned char)s.at(i + 1) == 0x80 &&
        ((unsigned char)s.at(i + 2) == 0xa8 || (unsigned char)s.at(i + 2) == 0xa9)) {
      w = bytes_into(w, (unsigned char)s.at(i + 2) == 0xa8 ? "\\u2028" : "\\u2029");
      i += 2;
    }
    else if (is_js_plain(s.at(i))) *w++ = s.at(i);
    else {
      w = bytes_into(w, "\\x");
      *w++ = kHexDigits.at(u >> 4);
      *w++ = kHexDigits.at(u & 15);
    }
  }
  return w;
}

constexpr char *
css_string_escaped_into(char *w, const std::string_view s)
{
  for (const char c : s) {
    const unsigned char u = (unsigned char)c;
    if (is_css_plain(c)) *w++ = c;
    else {
      *w++ = '\\';
      *w++ = kHexDigits.at(u >> 4);
      *w++ = kHexDigits.at(u & 15);
      *w++ = ' ';
    }
  }
  return w;
}

constexpr char *
troff_line_escaped_into(char *w, const std::string_view line)
{
  for (size_t j = 0; j < line.size(); j++) {
    const char c = line.at(j);
    const bool first = j == 0;
    const bool last = j + 1 == line.size();
    if (c == ' ') {
      if (first) w = bytes_into(w, "\\&");
      w = bytes_into(w, last ? "\\ " : " ");
    }
    else if (c == '.' && first) w = bytes_into(w, "\\&.");
    else if (c == '\\') w = bytes_into(w, "\\e");
    else if (c == '"') w = bytes_into(w, "\\(dq");
    else if (c == '\'') w = bytes_into(w, "\\(aq");
    else if (c == '`') w = bytes_into(w, "\\(ga");
    else if (c == '~') w = bytes_into(w, "\\(ti");
    else if (c == '^') w = bytes_into(w, "\\(ha");
    else *w++ = c;
  }
  size_t core = line.size();
  while (core > 0 && is_closing_delimiter(line.at(core - 1))) core--;
  const bool ends_in_space = !line.empty() && line.back() == ' ';
  const bool ends_a_sentence = core > 0 && is_sentence_mark(line.at(core - 1));
  if (ends_in_space || ends_a_sentence) w = bytes_into(w, "\\&");
  return w;
}

constexpr char *
troff_text_escaped_into(char *w, const std::string_view s)
{
  size_t from = 0;
  for (;;) {
    const size_t nl = s.find('\n', from);
    const std::string_view line = s.substr(from, nl == std::string_view::npos ? std::string_view::npos : nl - from);
    w = troff_line_escaped_into(w, line);
    if (nl == std::string_view::npos) return w;
    *w++ = '\n';
    from = nl + 1;
  }
}

constexpr size_t
escaped_into_in(const Context c, const std::span<char> room, const std::string_view s)
{
  char *const w = room.data();
  switch (c) {
    case Context::xml_text: return static_cast<size_t>(std::distance(w, xml_escaped_into(w, s)));
    case Context::json_string: return static_cast<size_t>(std::distance(w, json_string_escaped_into(w, s)));
    case Context::js_string: return static_cast<size_t>(std::distance(w, js_string_escaped_into(w, s)));
    case Context::css_string: return static_cast<size_t>(std::distance(w, css_string_escaped_into(w, s)));
    default: return static_cast<size_t>(std::distance(w, troff_text_escaped_into(w, s)));
  }
}

}

// The word form: every piece is an 8-byte word whose top byte holds the length of the piece, as the html table
// of escape.hpp holds its entities.
namespace word {

using namespace std::string_view_literals;

inline constexpr int kEntityLengthShift = (kEntityWordBytes - 1) * std::numeric_limits<unsigned char>::digits;

constexpr size_t
entity_length(const uint64_t word)
{
  return static_cast<size_t>(word >> kEntityLengthShift);
}

consteval uint64_t
word_of(const std::string_view piece)
{
  uint64_t word = uint64_t{piece.size()} << kEntityLengthShift;
  int shift = 0;
  for (const char c : piece) {
    word |= uint64_t{static_cast<unsigned char>(c)} << shift;
    shift += std::numeric_limits<unsigned char>::digits;
  }
  return word;
}

constexpr uint64_t
byte_word_of(const char c)
{
  return uint64_t{static_cast<unsigned char>(c)} | (uint64_t{1} << kEntityLengthShift);
}

constexpr uint64_t
hex_digits_put(const uint64_t word, const int at, const unsigned char u)
{
  const int shift = at * std::numeric_limits<unsigned char>::digits;
  return word | uint64_t{static_cast<unsigned char>(kHexDigits.at(u >> 4))} << shift |
         uint64_t{static_cast<unsigned char>(kHexDigits.at(u & 15))} << (shift + std::numeric_limits<unsigned char>::digits);
}

[[gnu::always_inline, gnu::flatten]] constexpr void
stored_word(const std::span<char, kEntityWordBytes> o, const uint64_t word)
{
  const auto bytes = std::bit_cast<std::array<char, kEntityWordBytes>>(word);
  std::copy_n(bytes.begin(), bytes.size(), o.begin());
}

[[gnu::always_inline, gnu::flatten]] constexpr size_t
word_appended(const std::span<char> room, const size_t count, const uint64_t word)
{
  static_assert(kBytesPastTheWrittenCount >= kEntityWordBytes,
                "a word stored at the written count stays inside the bytes that the caller proved past the count");
  stored_word(room.subspan(count).first<kEntityWordBytes>(), word);
  return count + entity_length(word);
}

[[gnu::always_inline, gnu::flatten]] constexpr size_t
bytes_appended(const std::span<char> room, const size_t count, const std::string_view bytes)
{
  std::copy_n(bytes.begin(), bytes.size(), room.subspan(count).begin());
  return count + bytes.size();
}

constexpr uint64_t
xml_word_of(const char c)
{
  switch (c) {
    case '&': return word_of("&amp;");
    case '<': return word_of("&lt;");
    case '>': return word_of("&gt;");
    case '"': return word_of("&quot;");
    case '\'': return word_of("&apos;");
    default: return byte_word_of(c);
  }
}

constexpr uint64_t
json_word_of(const char c)
{
  switch (c) {
    case '"': return word_of("\\\"");
    case '\\': return word_of("\\\\");
    case '\b': return word_of("\\b");
    case '\f': return word_of("\\f");
    case '\n': return word_of("\\n");
    case '\r': return word_of("\\r");
    case '\t': return word_of("\\t");
    default: break;
  }
  const unsigned char u = static_cast<unsigned char>(c);
  if (u < 0x20) return hex_digits_put(word_of("\\u00\0\0"sv), 4, u);
  return byte_word_of(c);
}

constexpr uint64_t
js_word_of(const char c)
{
  if (is_js_plain(c)) return byte_word_of(c);
  return hex_digits_put(word_of("\\x\0\0"sv), 2, static_cast<unsigned char>(c));
}

constexpr uint64_t
css_word_of(const char c)
{
  if (is_css_plain(c)) return byte_word_of(c);
  return hex_digits_put(word_of("\\\0\0 "sv), 1, static_cast<unsigned char>(c));
}

constexpr uint64_t
troff_word_of(const char c)
{
  switch (c) {
    case '\\': return word_of("\\e");
    case '"': return word_of("\\(dq");
    case '\'': return word_of("\\(aq");
    case '`': return word_of("\\(ga");
    case '~': return word_of("\\(ti");
    case '^': return word_of("\\(ha");
    default: return byte_word_of(c);
  }
}

inline constexpr std::string_view kLineSeparator = "\xe2\x80\xa8";
inline constexpr std::string_view kParagraphSeparator = "\xe2\x80\xa9";

struct Entities {
  std::array<uint64_t, 256> word;
};

template <class WordOf>
consteval Entities
entities_by(const WordOf &word_of_byte)
{
  Entities e{};
  for (size_t c = 0; c < e.word.size(); c++) e.word.at(c) = word_of_byte(static_cast<char>(static_cast<unsigned char>(c)));
  return e;
}

inline constexpr Entities kXmlEntities = entities_by(xml_word_of);
inline constexpr Entities kJsonEntities = entities_by(json_word_of);
inline constexpr Entities kJsEntities = entities_by(js_word_of);
inline constexpr Entities kCssEntities = entities_by(css_word_of);
inline constexpr Entities kTroffEntities = entities_by(troff_word_of);

[[gnu::always_inline]] constexpr uint64_t
entity_word_of(const Entities &table, const unsigned char c)
{
  static_assert(std::cmp_less(std::numeric_limits<unsigned char>::max(), std::tuple_size_v<decltype(Entities::word)>),
                "every value of unsigned char indexes an entry of the table");
  return table.word[c];
}

#if ARM != 3

[[gnu::always_inline]] constexpr size_t
table_escaped_into(const std::span<char> room, const size_t count, const std::string_view s, const Entities &table)
{
  size_t at = count;
  for (const char c : s) at = word_appended(room, at, entity_word_of(table, static_cast<unsigned char>(c)));
  return at;
}

constexpr size_t
js_string_escaped_into(const std::span<char> room, const std::string_view s)
{
  size_t count = 0;
  for (auto at = s.begin(); at != s.end(); ++at) {
    const std::string_view rest(at, s.end());
    if (*at != kLineSeparator.front()) count = word_appended(room, count, entity_word_of(kJsEntities, static_cast<unsigned char>(*at)));
    else if (rest.starts_with(kLineSeparator)) {
      count = word_appended(room, count, word_of("\\u2028"));
      at = std::next(at, kLineSeparator.size() - 1);
    }
    else if (rest.starts_with(kParagraphSeparator)) {
      count = word_appended(room, count, word_of("\\u2029"));
      at = std::next(at, kParagraphSeparator.size() - 1);
    }
    else count = word_appended(room, count, entity_word_of(kJsEntities, static_cast<unsigned char>(*at)));
  }
  return count;
}

#else

[[gnu::always_inline]] constexpr size_t
table_escaped_into(const std::span<char> room, const size_t count, const std::string_view s, const Entities &table)
{
  size_t written = count;
  auto at = s.begin();
  for (;;) {
    const auto stop = std::find_if(at, s.end(), [&table](const char c) {
      return entity_length(entity_word_of(table, static_cast<unsigned char>(c))) != 1;
    });
    written = bytes_appended(room, written, std::string_view(at, stop));
    if (stop == s.end()) return written;
    written = word_appended(room, written, entity_word_of(table, static_cast<unsigned char>(*stop)));
    at = std::next(stop);
  }
}

constexpr size_t
js_string_escaped_into(const std::span<char> room, const std::string_view s)
{
  size_t count = 0;
  auto at = s.begin();
  for (;;) {
    const auto stop = std::find_if(at, s.end(), [](const char c) {
      return c == '\xe2' || entity_length(entity_word_of(kJsEntities, static_cast<unsigned char>(c))) != 1;
    });
    count = bytes_appended(room, count, std::string_view(at, stop));
    if (stop == s.end()) return count;
    const std::string_view rest(stop, s.end());
    if (rest.starts_with(kLineSeparator)) {
      count = word_appended(room, count, word_of("\\u2028"));
      at = std::next(stop, kLineSeparator.size());
    }
    else if (rest.starts_with(kParagraphSeparator)) {
      count = word_appended(room, count, word_of("\\u2029"));
      at = std::next(stop, kParagraphSeparator.size());
    }
    else {
      count = word_appended(room, count, entity_word_of(kJsEntities, static_cast<unsigned char>(*stop)));
      at = std::next(stop);
    }
  }
}

#endif

constexpr size_t
xml_escaped_into(const std::span<char> room, const std::string_view s)
{
  return table_escaped_into(room, 0, s, kXmlEntities);
}

constexpr size_t
json_string_escaped_into(const std::span<char> room, const std::string_view s)
{
  return table_escaped_into(room, 0, s, kJsonEntities);
}

constexpr size_t
css_string_escaped_into(const std::span<char> room, const std::string_view s)
{
  return table_escaped_into(room, 0, s, kCssEntities);
}

constexpr size_t
troff_body_escaped_into(const std::span<char> room, const size_t count, const std::string_view body)
{
  return table_escaped_into(room, count, body, kTroffEntities);
}

constexpr size_t
troff_line_escaped_into(const std::span<char> room, const std::string_view line)
{
  size_t count = 0;
  if (line.starts_with(' ') || line.starts_with('.')) count = word_appended(room, count, word_of("\\&"));
  const bool ends_in_space = line.ends_with(' ');
  count = troff_body_escaped_into(room, count, ends_in_space ? line.substr(0, line.size() - 1) : line);
  if (ends_in_space) count = word_appended(room, count, word_of("\\ "));
  const auto core = std::find_if_not(line.rbegin(), line.rend(), is_closing_delimiter);
  const bool ends_a_sentence = core != line.rend() && is_sentence_mark(*core);
  if (ends_in_space || ends_a_sentence) count = word_appended(room, count, word_of("\\&"));
  return count;
}

constexpr size_t
troff_text_escaped_into(const std::span<char> room, const std::string_view s)
{
  size_t count = 0;
  std::string_view rest = s;
  for (;;) {
    const size_t nl = rest.find('\n');
    count += troff_line_escaped_into(room.subspan(count), rest.substr(0, nl));
    if (nl == std::string_view::npos) return count;
    count = word_appended(room, count, byte_word_of('\n'));
    rest.remove_prefix(nl + 1);
  }
}

constexpr size_t
escaped_into_in(const Context c, const std::span<char> room, const std::string_view s)
{
  switch (c) {
    case Context::xml_text: return xml_escaped_into(room, s);
    case Context::json_string: return json_string_escaped_into(room, s);
    case Context::js_string: return js_string_escaped_into(room, s);
    case Context::css_string: return css_string_escaped_into(room, s);
    default: return troff_text_escaped_into(room, s);
  }
}

}

// The class form of arms 4 and 5: the word of a byte is made from the class of the byte and the byte.
namespace classes {

using namespace std::string_view_literals;
using word::byte_word_of;
using word::word_appended;
using word::word_of;

inline constexpr uint8_t kUnchanged = 0;
inline constexpr uint8_t kHexForm = 1;
inline constexpr size_t kByteValues = 256;

using Classes = std::array<uint8_t, kByteValues>;

inline constexpr int kNibbleBits = 4;
inline constexpr unsigned kNibbleMask = 15;
inline constexpr int kBitsPerByte = std::numeric_limits<unsigned char>::digits;
// A nibble of 10 or more carries into bit 4 when 16 - 10 = 6 is added; the low byte stays under 32, so no carry
// reaches the high byte.
inline constexpr uint64_t kSixInBothBytes = 0x0606;
inline constexpr uint64_t kOneInBothBytes = 0x0101;
inline constexpr uint64_t kZeroDigitInBothBytes = 0x3030;
inline constexpr uint64_t kLetterAfterDigits = 'a' - '0' - 10;

// The two hex digits of u, the high digit in the low byte, as text order wants them in a little-endian word.
constexpr uint64_t
hex_pair_of(const unsigned char u)
{
  const uint64_t nibbles = uint64_t{static_cast<unsigned>(u >> kNibbleBits)} | uint64_t{u & kNibbleMask} << kBitsPerByte;
  const uint64_t letters = (nibbles + kSixInBothBytes) >> kNibbleBits & kOneInBothBytes;
  return nibbles + kZeroDigitInBothBytes + letters * kLetterAfterDigits;
}

constexpr uint64_t
all_bits_if(const bool b)
{
  return uint64_t{0} - uint64_t{b};
}

template <Context C> inline constexpr int kPieces = 0;
template <> inline constexpr std::array<uint64_t, 6> kPieces<Context::xml_text> = {
    byte_word_of('\0'), word_of("&amp;"), word_of("&lt;"), word_of("&gt;"), word_of("&quot;"), word_of("&apos;")};
template <> inline constexpr std::array<uint64_t, 9> kPieces<Context::json_string> = {
    byte_word_of('\0'), word_of("\\u00\0\0"sv), word_of("\\\""), word_of("\\\\"), word_of("\\b"), word_of("\\f"),
    word_of("\\n"),     word_of("\\r"),         word_of("\\t")};
template <> inline constexpr std::array<uint64_t, 2> kPieces<Context::js_string> = {byte_word_of('\0'), word_of("\\x\0\0"sv)};
template <> inline constexpr std::array<uint64_t, 2> kPieces<Context::css_string> = {byte_word_of('\0'), word_of("\\\0\0 "sv)};
template <> inline constexpr std::array<uint64_t, 7> kPieces<Context::troff_text> = {
    byte_word_of('\0'), word_of("\\e"), word_of("\\(dq"), word_of("\\(aq"), word_of("\\(ga"), word_of("\\(ti"), word_of("\\(ha")};

// The byte place of the two hex digits in the word of the hex form; 0: the context has no hex form.
template <Context C> inline constexpr size_t kHexAt = 0;
template <> inline constexpr size_t kHexAt<Context::json_string> = 4;
template <> inline constexpr size_t kHexAt<Context::js_string> = 2;
template <> inline constexpr size_t kHexAt<Context::css_string> = 1;

template <Context C, class WordOf>
consteval Classes
classes_by(const WordOf &word_of_byte)
{
  Classes k{};
  for (size_t b = 0; b < k.size(); b++) {
    const unsigned char u = static_cast<unsigned char>(b);
    const char c = static_cast<char>(u);
    const uint64_t w = word_of_byte(c);
    const auto found = std::ranges::find(kPieces<C>, w);
    if (w == byte_word_of(c)) k.at(b) = kUnchanged;
    else if (kHexAt<C> != 0 && w == (kPieces<C>.at(kHexForm) | hex_pair_of(u) << (kHexAt<C> * kBitsPerByte))) k.at(b) = kHexForm;
    else if (found != kPieces<C>.end()) k.at(b) = static_cast<uint8_t>(std::distance(kPieces<C>.begin(), found));
    else throw std::logic_error("a byte has no class");
  }
  return k;
}

template <Context C> inline constexpr int kClasses = 0;
template <> inline constexpr Classes kClasses<Context::xml_text> = classes_by<Context::xml_text>(word::xml_word_of);
template <> inline constexpr Classes kClasses<Context::json_string> = classes_by<Context::json_string>(word::json_word_of);
template <> inline constexpr Classes kClasses<Context::js_string> = classes_by<Context::js_string>(word::js_word_of);
template <> inline constexpr Classes kClasses<Context::css_string> = classes_by<Context::css_string>(word::css_word_of);
template <> inline constexpr Classes kClasses<Context::troff_text> = classes_by<Context::troff_text>(word::troff_word_of);

struct Field {
  int      shift;
  uint16_t mask;
};

template <Context C> inline constexpr Field kField{};
template <> inline constexpr Field kField<Context::xml_text>{0, 7};
template <> inline constexpr Field kField<Context::json_string>{3, 15};
template <> inline constexpr Field kField<Context::js_string>{7, 1};
template <> inline constexpr Field kField<Context::css_string>{8, 1};
template <> inline constexpr Field kField<Context::troff_text>{9, 7};

template <Context C>
consteval uint16_t
field_bits_of(const size_t b)
{
  if (kClasses<C>.at(b) > kField<C>.mask) throw std::logic_error("a class does not fit its field");
  return static_cast<uint16_t>(kClasses<C>.at(b) << kField<C>.shift);
}

consteval std::array<uint16_t, kByteValues>
fields_of()
{
  std::array<uint16_t, kByteValues> f{};
  for (size_t b = 0; b < f.size(); b++)
    f.at(b) = field_bits_of<Context::xml_text>(b) | field_bits_of<Context::json_string>(b) | field_bits_of<Context::js_string>(b) |
              field_bits_of<Context::css_string>(b) | field_bits_of<Context::troff_text>(b);
  return f;
}

inline constexpr std::array<uint16_t, kByteValues> kFields = fields_of();

template <Context C>
[[gnu::always_inline]] constexpr uint8_t
class_of(const unsigned char u)
{
  static_assert(std::cmp_less(std::numeric_limits<unsigned char>::max(), kByteValues), "every value of unsigned char indexes an entry");
#if ARM == 5
  return static_cast<uint8_t>(kFields[u] >> kField<C>.shift & kField<C>.mask);
#else
  return kClasses<C>[u];
#endif
}

template <Context C>
consteval bool
every_class_indexes_a_piece()
{
  for (size_t b = 0; b < kByteValues; b++) {
    if (class_of<C>(static_cast<unsigned char>(b)) >= kPieces<C>.size()) return false;
  }
  return true;
}

static_assert(every_class_indexes_a_piece<Context::xml_text>() && every_class_indexes_a_piece<Context::json_string>() &&
                  every_class_indexes_a_piece<Context::js_string>() && every_class_indexes_a_piece<Context::css_string>() &&
                  every_class_indexes_a_piece<Context::troff_text>(),
              "the class of every byte indexes a word of the pieces of its context");

template <Context C>
[[gnu::always_inline]] constexpr uint64_t
word_of_byte(const unsigned char u)
{
  const uint8_t k = class_of<C>(u);
  uint64_t w = kPieces<C>[k] | (uint64_t{u} & all_bits_if(k == kUnchanged));
  if constexpr (kHexAt<C> != 0) w |= hex_pair_of(u) << (kHexAt<C> * kBitsPerByte) & all_bits_if(k == kHexForm);
  return w;
}

template <Context C>
[[gnu::always_inline]] constexpr size_t
classes_escaped_into(const std::span<char> room, const size_t count, const std::string_view s)
{
  size_t at = count;
  for (const char c : s) at = word_appended(room, at, word_of_byte<C>(static_cast<unsigned char>(c)));
  return at;
}

constexpr size_t
js_string_escaped_into(const std::span<char> room, const std::string_view s)
{
  size_t count = 0;
  for (auto at = s.begin(); at != s.end(); ++at) {
    const std::string_view rest(at, s.end());
    if (*at != word::kLineSeparator.front()) count = word_appended(room, count, word_of_byte<Context::js_string>(static_cast<unsigned char>(*at)));
    else if (rest.starts_with(word::kLineSeparator)) {
      count = word_appended(room, count, word_of("\\u2028"));
      at = std::next(at, word::kLineSeparator.size() - 1);
    }
    else if (rest.starts_with(word::kParagraphSeparator)) {
      count = word_appended(room, count, word_of("\\u2029"));
      at = std::next(at, word::kParagraphSeparator.size() - 1);
    }
    else count = word_appended(room, count, word_of_byte<Context::js_string>(static_cast<unsigned char>(*at)));
  }
  return count;
}

constexpr size_t
troff_line_escaped_into(const std::span<char> room, const std::string_view line)
{
  size_t count = 0;
  if (line.starts_with(' ') || line.starts_with('.')) count = word_appended(room, count, word_of("\\&"));
  const bool ends_in_space = line.ends_with(' ');
  count = classes_escaped_into<Context::troff_text>(room, count, ends_in_space ? line.substr(0, line.size() - 1) : line);
  if (ends_in_space) count = word_appended(room, count, word_of("\\ "));
  const auto core = std::find_if_not(line.rbegin(), line.rend(), is_closing_delimiter);
  const bool ends_a_sentence = core != line.rend() && is_sentence_mark(*core);
  if (ends_in_space || ends_a_sentence) count = word_appended(room, count, word_of("\\&"));
  return count;
}

constexpr size_t
troff_text_escaped_into(const std::span<char> room, const std::string_view s)
{
  size_t count = 0;
  std::string_view rest = s;
  for (;;) {
    const size_t nl = rest.find('\n');
    count += troff_line_escaped_into(room.subspan(count), rest.substr(0, nl));
    if (nl == std::string_view::npos) return count;
    count = word_appended(room, count, byte_word_of('\n'));
    rest.remove_prefix(nl + 1);
  }
}

constexpr size_t
escaped_into_in(const Context c, const std::span<char> room, const std::string_view s)
{
  switch (c) {
    case Context::xml_text: return classes_escaped_into<Context::xml_text>(room, 0, s);
    case Context::json_string: return classes_escaped_into<Context::json_string>(room, 0, s);
    case Context::js_string: return js_string_escaped_into(room, s);
    case Context::css_string: return classes_escaped_into<Context::css_string>(room, 0, s);
    default: return troff_text_escaped_into(room, s);
  }
}

}

[[gnu::always_inline]] inline size_t
escaped(const Context c, const std::span<char> room, const std::string_view s)
{
#if ARM == 0 || ARM == 1
  return before::escaped_into_in(c, room, s);
#elif ARM == 4 || ARM == 5
  return classes::escaped_into_in(c, room, s);
#else
  return word::escaped_into_in(c, room, s);
#endif
}

inline constexpr size_t kLengthMax = 1024;
inline constexpr size_t kRoomMax = kEntityMax * kLengthMax + kBytesPastTheWrittenCount;
inline constexpr size_t kBufferBytes = 1 << 16;
inline constexpr size_t kInputBytesPerRepetition = 1 << 22;
inline constexpr int kRepetitions = 5;
inline constexpr size_t kValuesPerInput = 64;
inline constexpr uint32_t kSeed = 20261114;

// The room of the rule for each value: escaped_size_max_in plus the bytes past the count. The bytes of the
// arm must equal the bytes of the arm "before", and no byte behind the room may change.
bool
held_to_before(const Context c, const std::string_view s)
{
  const size_t room = escaped_size_max_in(c, s.size()) + kBytesPastTheWrittenCount;
  std::vector<char> a(room + kBytesPastTheWrittenCount, '\x5a');
  std::vector<char> b(room + kBytesPastTheWrittenCount, '\x5a');
  const size_t n = escaped(c, std::span<char>(a).first(room), s);
  const size_t m = before::escaped_into_in(c, std::span<char>(b).first(room), s);
  const bool same = n == m && std::ranges::equal(std::span(a).first(n), std::span(b).first(m)) &&
                    std::ranges::all_of(std::span(a).subspan(room), [](const char x) { return x == '\x5a'; });
  if (!same) std::println(stderr, "arm {} differs from the arm before in context {} at a value of {} bytes", ARM, std::to_underlying(c), s.size());
  return same;
}

bool
every_case_held()
{
  constexpr std::array<std::string_view, 16> kPieces = {"\xe2\x80\xa8", "\xe2\x80\xa9", "\xe2", "\xe2\x80", " ", ".", "\n", "\n \n",
                                                        "\n.\n", ".)\"", "!]'", "\n\n", "  ", ". ", " .", "\x01"};
  bool all = true;
  for (const Context c : kContexts) {
    all = held_to_before(c, "") && all;
    for (int b = 0; b < 256; b++) {
      const char x = static_cast<char>(static_cast<unsigned char>(b));
      for (const std::string &s : {std::string(1, x), "a" + std::string(1, x), std::string(1, x) + "a", "a" + std::string(1, x) + "a"})
        all = held_to_before(c, s) && all;
    }
    for (size_t n = 0; n <= 20; n++) {
      for (size_t at = 0; at < n; at++) {
        for (const std::string_view p : kPieces) {
          std::string s(n, 'x');
          s.replace(at, std::min(p.size(), n - at), p.substr(0, n - at));
          all = held_to_before(c, s) && all;
        }
      }
    }
  }
  return all;
}

std::string_view
marks_of(const Context c)
{
  switch (c) {
    case Context::xml_text: return "&<>\"'";
    case Context::json_string: return "\"\\\n\x01";
    case Context::js_string: return "'<\\/";
    case Context::css_string: return "'}<\\";
    default: return "\\\"'`~^";
  }
}

// kValuesPerInput values of n bytes: plain letters, digits and a space; each byte is a mark of the context
// with the probability percent / 100; with lines, every 16th byte is a line feed.
std::vector<std::string>
values_of(const Context c, const size_t n, const uint32_t percent, const bool lines)
{
  constexpr std::string_view kPlain = "abcdefghijklmnopqrstuvwxyz0123456789 ";
  constexpr size_t kLineBytes = 16;
  std::mt19937 random(kSeed + static_cast<uint32_t>(n) + percent);
  std::uniform_int_distribution<uint32_t> hundred(0, 99);
  std::uniform_int_distribution<size_t> plain(0, kPlain.size() - 1);
  const std::string_view marks = marks_of(c);
  std::uniform_int_distribution<size_t> mark(0, marks.size() - 1);
  std::vector<std::string> v;
  for (size_t k = 0; k < kValuesPerInput; k++) {
    std::string s;
    for (size_t i = 0; i < n; i++) {
      if (lines && i % kLineBytes == kLineBytes - 1) s += '\n';
      else if (hundred(random) < percent) s += marks.at(mark(random));
      else s += kPlain.at(plain(random));
    }
    v.push_back(s);
  }
  return v;
}

struct Input {
  Context                  context;
  std::string              name;
  std::vector<std::string> values;
};

std::vector<Input>
inputs()
{
  std::vector<Input> in;
  for (const Context c : kContexts) {
    for (const size_t n : {size_t{16}, size_t{64}, size_t{1024}}) {
      for (const uint32_t p : {0u, 5u}) in.push_back({c, std::to_string(n) + "-" + std::to_string(p), values_of(c, n, p, false)});
    }
    if (c == Context::troff_text) {
      for (const size_t n : {size_t{64}, size_t{1024}}) {
        for (const uint32_t p : {0u, 5u}) in.push_back({c, "lines-" + std::to_string(n) + "-" + std::to_string(p), values_of(c, n, p, true)});
      }
    }
  }
  return in;
}

std::string_view
name_of(const Context c)
{
  switch (c) {
    case Context::xml_text: return "xml";
    case Context::json_string: return "json";
    case Context::js_string: return "js";
    case Context::css_string: return "css";
    default: return "troff";
  }
}

[[gnu::noinline]] size_t
escaped_values(const std::span<char> buffer, const Context c, const std::vector<std::string> &values, const size_t passes)
{
  size_t sum = 0;
  size_t at = 0;
  for (size_t p = 0; p < passes; p++) {
    for (const std::string &s : values) {
      const size_t n = escaped(c, buffer.subspan(at), s);
      at += n;
      sum += n;
      if (at > buffer.size() - kRoomMax) at = 0;
    }
  }
  return sum;
}

}

int
main(const int argc, const char *const *const argv)
{
  using namespace probe;
  if (!every_case_held()) return 1;
  const std::vector<Input> in = inputs();
  for (const Input &i : in) {
    for (const std::string &s : i.values) {
      if (!held_to_before(i.context, s)) return 1;
    }
  }
  const std::string_view label = argc > 2 ? argv[2] : "";
  const std::string_view round = argc > 1 ? argv[1] : "0";
  std::vector<char> buffer(kBufferBytes);
  for (const Input &i : in) {
    const size_t passes = std::max(size_t{1}, kInputBytesPerRepetition / (i.values.size() * i.values.front().size()));
    for (int r = 0; r < kRepetitions; r++) {
      const auto start = std::chrono::steady_clock::now();
      const size_t sum = escaped_values(buffer, i.context, i.values, passes);
      const std::chrono::duration<double, std::nano> took = std::chrono::steady_clock::now() - start;
      std::println("round={} {} context={} input={} ns_per_value={:.4f}", round, label, name_of(i.context), i.name,
                   took.count() / static_cast<double>(passes * i.values.size()));
      std::println(stderr, "sum {}", sum);
    }
  }
  return 0;
}
