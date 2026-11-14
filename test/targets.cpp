#include <mustache/mustache.hpp>

#include "node.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <cstddef>
#include <expected>
#include <iomanip>
#include <ios>
#include <limits>
#include <map>
#include <memory_resource>
#include <span>
#include <sstream>
#include <streambuf>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

mustache::Template
compiled(const std::string_view source)
{
  const std::expected<mustache::Template, mustache::Error> t = mustache::compile(source);
  EXPECT_TRUE(t);
  return t ? *t : mustache::Template{};
}

Node
page()
{
  return Node{NodeMap{{"name", Node{std::string("Ada <&> co")}}, {"n", Node{std::int64_t{42}}}}};
}

constexpr std::string_view kSource = "<p>{{name}}</p>{{{name}}}{{n}}";
constexpr std::string_view kRendered = "<p>Ada &lt;&amp;&gt; co</p>Ada <&> co42";

// render_to is documented as the append form of render, so its output
// is compared with the output of render and with the text written by
// hand from the specification of the escape.
TEST(RenderToString, AppendsTheBytesThatRenderReturns)
{
  const mustache::Template t = compiled(kSource);
  const Node data = page();
  const std::expected<std::string, mustache::Error> whole = mustache::render(t, data);
  ASSERT_TRUE(whole);
  ASSERT_EQ(*whole, kRendered);
  for (const std::string_view before : {"", "x", "a string that is longer than the small string of the library"}) {
    SCOPED_TRACE(before);
    std::string out(before);
    const std::expected<void, mustache::Error> r = mustache::render_to(out, t, data);
    ASSERT_TRUE(r);
    EXPECT_EQ(out, std::string(before) + std::string(kRendered));
  }
}

// Two calls on one string leave both renders in it, one after the other.
TEST(RenderToString, AppendsAgainOnASecondCall)
{
  const mustache::Template t = compiled(kSource);
  const Node data = page();
  std::string out;
  ASSERT_TRUE(mustache::render_to(out, t, data));
  ASSERT_TRUE(mustache::render_to(out, t, data));
  EXPECT_EQ(out, std::string(kRendered) + std::string(kRendered));
}

// The reason for the call is that the string of the caller keeps its
// memory: a string with room for the render gets no new block, and an
// initial capacity of zero does not make it smaller.
TEST(RenderToString, KeepsTheBlockOfAStringWithRoom)
{
  const mustache::Template t = compiled(kSource);
  const Node data = page();
  std::string out = "keep:";
  out.reserve(100000);
  const char *const block = out.data();
  const size_t capacity = out.capacity();
  for (const size_t initial : {size_t{0}, size_t{1}, mustache::kInitialCapacity}) {
    SCOPED_TRACE(initial);
    out = "keep:";
    ASSERT_TRUE(mustache::render_to(out, t, data, {}, initial));
    EXPECT_EQ(out, "keep:" + std::string(kRendered));
    EXPECT_EQ(out.data(), block);
    EXPECT_EQ(out.capacity(), capacity);
  }
}

// A string that is too small grows to hold the render, and the bytes
// it held before the call stay in front.
TEST(RenderToString, GrowsAStringWithoutRoom)
{
  const Node data{NodeMap{{"a", Node{std::string(50000, 'x')}}}};
  const mustache::Template t = compiled("{{{a}}}{{a}}");
  for (const size_t initial : {size_t{0}, size_t{1}, mustache::kInitialCapacity}) {
    SCOPED_TRACE(initial);
    std::string out = "head";
    ASSERT_TRUE(mustache::render_to(out, t, data, {}, initial));
    EXPECT_EQ(out, "head" + std::string(100000, 'x'));
  }
}

// An initial capacity over the limit of the output is an error value, as
// in render, and the string is as it was.
TEST(RenderToString, LeavesTheStringAsItWasOnAnInitialCapacityOverTheLimit)
{
  const mustache::Template t = compiled(kSource);
  const Node data = page();
  std::string out = "keep";
  const std::expected<void, mustache::Error> r =
      mustache::render_to(out, t, data, {}, std::numeric_limits<size_t>::max());
  ASSERT_FALSE(r);
  EXPECT_EQ(r.error().fault, mustache::Fault::over_limit);
  EXPECT_EQ(r.error().allowed, mustache::kMaxCapacity);
  EXPECT_EQ(out, "keep");
}

// The limit counts the bytes that the string held before the call. The
// walk is run with a limit of 64 bytes: 50 held bytes and 14 new bytes
// fit, and 51 held bytes and 14 new bytes do not.
TEST(RenderToString, CountsTheHeldBytesInTheLimit)
{
  const mustache::Template t = compiled("{{a}}");
  const Node data{NodeMap{{"a", Node{std::string(14, 'y')}}}};
  const auto walk = [] {
    return mustache::Walk<>(mustache::detail::containers::Access{}, nullptr, mustache::kMaxRenderScore, 4, 64);
  };
  std::string fits(50, 'x');
  ASSERT_TRUE(walk().appended_to(fits, t, mustache::detail::containers::value_of(data)));
  EXPECT_EQ(fits, std::string(50, 'x') + std::string(14, 'y'));

  std::string over(51, 'x');
  const std::expected<void, mustache::Error> r = walk().appended_to(over, t, mustache::detail::containers::value_of(data));
  ASSERT_FALSE(r);
  EXPECT_EQ(r.error().fault, mustache::Fault::over_limit);
  EXPECT_EQ(r.error().allowed, 64U);
  EXPECT_EQ(r.error().asked, 65U);
  EXPECT_EQ(over, std::string(51, 'x'));
}

// An error in the middle of the render, after a part of it was written,
// leaves no part of it in the string.
TEST(RenderToString, LeavesTheStringAsItWasOnAnErrorAfterAPartialWrite)
{
  const mustache::Template t = compiled("{{a}}{{b}}{{a}}{{a}}{{a}}{{a}}");
  const Node data{NodeMap{{"a", Node{std::string(20, 'y')}}, {"b", Node{NodeList{}}}}};
  std::string out(10, 'x');
  const std::expected<void, mustache::Error> r =
      mustache::Walk<>(mustache::detail::containers::Access{}, nullptr, mustache::kMaxRenderScore, 4, 64)
          .appended_to(out, t, mustache::detail::containers::value_of(data));
  ASSERT_FALSE(r);
  EXPECT_EQ(out, std::string(10, 'x'));
}

// A value that is not text is an error of the render, as in render.
TEST(RenderToString, LeavesTheStringAsItWasOnAValueThatIsNotText)
{
  const mustache::Template t = compiled("ab{{a}}");
  const Node data{NodeMap{{"a", Node{NodeMap{{"x", Node{true}}}}}}};
  std::string out = "keep";
  const std::expected<void, mustache::Error> r = mustache::render_to(out, t, data);
  ASSERT_FALSE(r);
  EXPECT_EQ(r.error().fault, mustache::Fault::not_text);
  EXPECT_EQ(out, "keep");
}

// The work limit stops a render that has already written bytes.
TEST(RenderToString, LeavesTheStringAsItWasOnTheWorkLimit)
{
  const Node data{NodeMap{{"a", Node{NodeList(5000, Node{true})}}}};
  const mustache::Template t = compiled("{{#a}}{{#a}}x{{/a}}{{/a}}");
  std::string out = "keep";
  const std::expected<void, mustache::Error> r = mustache::render_to(out, t, data);
  ASSERT_FALSE(r);
  EXPECT_EQ(r.error().fault, mustache::Fault::over_work);
  EXPECT_EQ(out, "keep");
}

// A request for memory that no machine has is no_memory, not a throw.
TEST(RenderToString, LeavesTheStringAsItWasWhenTheMemoryIsRefused)
{
  const mustache::Template t = compiled(kSource);
  const Node data = page();
  std::string out = "keep";
  const std::expected<void, mustache::Error> r =
      mustache::render_to(out, t, data, {}, mustache::kMaxCapacity - out.size());
  ASSERT_FALSE(r);
  EXPECT_EQ(r.error().fault, mustache::Fault::no_memory);
  EXPECT_EQ(out, "keep");
}

struct ThrowingLess {
  using is_transparent = void;
  bool operator()(const std::string_view, const std::string_view) const { throw std::runtime_error("a lookup throws"); }
};

// An exception of the data leaves the walk. The string of the caller
// comes back as it was, with its bytes.
TEST(RenderToString, GivesTheStringBackWhenTheDataThrows)
{
  const mustache::Template t = compiled("x{{a}}");
  const std::map<std::string, std::string, ThrowingLess> data{{"a", "b"}};
  std::string out = "keep";
  EXPECT_THROW(static_cast<void>(mustache::render_to(out, t, data)), std::runtime_error);
  EXPECT_EQ(out, "keep");
}

// A string with another allocator takes the render with one copy. The
// memory of the string is a buffer of the test; when it is too small
// the null resource refuses the next block, and the string is as it was.
TEST(RenderToString, AppendsToAStringOfAnotherAllocator)
{
  const mustache::Template t = compiled(kSource);
  const Node data = page();
  std::array<std::byte, 512> buffer;
  std::pmr::monotonic_buffer_resource memory(buffer.data(), buffer.size(), std::pmr::null_memory_resource());
  std::pmr::string out("keep:", &memory);
  ASSERT_TRUE(mustache::render_to(out, t, data));
  EXPECT_EQ(std::string_view(out), "keep:" + std::string(kRendered));
}

TEST(RenderToString, ReturnsNoMemoryForAStringOfAnotherAllocatorThatIsFull)
{
  const Node data{NodeMap{{"a", Node{std::string(4000, 'x')}}}};
  const mustache::Template t = compiled("{{{a}}}");
  std::array<std::byte, 256> buffer;
  std::pmr::monotonic_buffer_resource memory(buffer.data(), buffer.size(), std::pmr::null_memory_resource());
  std::pmr::string out("keep", &memory);
  const std::expected<void, mustache::Error> r = mustache::render_to(out, t, data);
  ASSERT_FALSE(r);
  EXPECT_EQ(r.error().fault, mustache::Fault::no_memory);
  EXPECT_EQ(std::string_view(out), "keep");
}

// An error of the render comes back for a string of another allocator
// as it does for std::string.
TEST(RenderToString, ReturnsTheErrorOfTheRenderForAStringOfAnotherAllocator)
{
  const mustache::Template t = compiled("ab{{a}}");
  const Node data{NodeMap{{"a", Node{NodeMap{{"x", Node{true}}}}}}};
  std::pmr::string out("keep");
  const std::expected<void, mustache::Error> r = mustache::render_to(out, t, data);
  ASSERT_FALSE(r);
  EXPECT_EQ(r.error().fault, mustache::Fault::not_text);
  EXPECT_EQ(std::string_view(out), "keep");
}

std::vector<std::byte>
bytes_of(const std::string_view s)
{
  const auto b = std::as_bytes(std::span<const char>(s));
  return std::vector<std::byte>(b.begin(), b.end());
}

// A vector of char and a vector of std::byte take the bytes of the
// render behind the bytes they hold, and the high bytes of a UTF-8
// value are not changed.
TEST(RenderToVector, AppendsTheBytesThatRenderReturns)
{
  const mustache::Template t = compiled("\xC3\xA9{{name}}\xFF");
  const Node data = page();
  const std::string whole = "\xC3\xA9" "Ada &lt;&amp;&gt; co\xFF";
  ASSERT_EQ(*mustache::render(t, data), whole);

  std::vector<char> chars = {'h', 'e', 'a', 'd'};
  ASSERT_TRUE(mustache::render_to(chars, t, data));
  EXPECT_EQ(std::string(chars.begin(), chars.end()), "head" + whole);

  std::vector<std::byte> octets = bytes_of("head");
  ASSERT_TRUE(mustache::render_to(octets, t, data));
  EXPECT_EQ(octets, bytes_of("head" + whole));
}

// The vector keeps its block when it has room for the bytes.
TEST(RenderToVector, KeepsTheBlockOfAVectorWithRoom)
{
  const mustache::Template t = compiled(kSource);
  const Node data = page();
  std::vector<char> out;
  out.reserve(100000);
  const char *const block = out.data();
  for (int i = 0; i < 3; i++) ASSERT_TRUE(mustache::render_to(out, t, data));
  EXPECT_EQ(out.data(), block);
  EXPECT_EQ(out.capacity(), 100000U);
  EXPECT_EQ(out.size(), 3 * kRendered.size());
}

// Any other element type is refused when the call is compiled: a
// vector of unsigned char or int, and the packed vector of bool.
TEST(RenderToVector, RefusesAnElementThatIsNotCharOrByte)
{
  const mustache::Template t = compiled(kSource);
  const Node data = page();
  const auto accepts = []<class V>(V &v) { return requires { mustache::render_to(v, t, data); }; };
  std::vector<char> c;
  std::vector<std::byte> b;
  std::vector<unsigned char> u;
  std::vector<std::int8_t> i;
  std::vector<int> n;
  std::vector<bool> flags;
  EXPECT_TRUE(accepts(c));
  EXPECT_TRUE(accepts(b));
  EXPECT_FALSE(accepts(u));
  EXPECT_FALSE(accepts(i));
  EXPECT_FALSE(accepts(n));
  EXPECT_FALSE(accepts(flags));
}

TEST(RenderToVector, LeavesTheVectorAsItWasOnAnError)
{
  const mustache::Template t = compiled("ab{{a}}");
  const Node not_text{NodeMap{{"a", Node{NodeMap{{"x", Node{true}}}}}}};
  std::vector<std::byte> out = bytes_of("keep");
  const std::expected<void, mustache::Error> r = mustache::render_to(out, t, not_text);
  ASSERT_FALSE(r);
  EXPECT_EQ(r.error().fault, mustache::Fault::not_text);
  EXPECT_EQ(out, bytes_of("keep"));

  const mustache::Template u = compiled(kSource);
  const Node data = page();
  const std::expected<void, mustache::Error> over =
      mustache::render_to(out, u, data, {}, std::numeric_limits<size_t>::max());
  ASSERT_FALSE(over);
  EXPECT_EQ(over.error().fault, mustache::Fault::over_limit);
  EXPECT_EQ(out, bytes_of("keep"));
}

// A vector of another allocator that is full gives no_memory and keeps
// what it held.
TEST(RenderToVector, ReturnsNoMemoryForAVectorThatIsFull)
{
  const Node data{NodeMap{{"a", Node{std::string(4000, 'x')}}}};
  const mustache::Template t = compiled("{{{a}}}");
  std::array<std::byte, 256> buffer;
  std::pmr::monotonic_buffer_resource memory(buffer.data(), buffer.size(), std::pmr::null_memory_resource());
  std::pmr::vector<char> out({'k', 'e', 'e', 'p'}, &memory);
  const std::expected<void, mustache::Error> r = mustache::render_to(out, t, data);
  ASSERT_FALSE(r);
  EXPECT_EQ(r.error().fault, mustache::Fault::no_memory);
  EXPECT_EQ(std::string(out.begin(), out.end()), "keep");
}

// A stream buffer that counts the calls of the stream and keeps the
// bytes. With a limit it takes only that many bytes, as a full disk does.
class CountingBuffer : public std::streambuf {
public:
  std::string  bytes;
  int          writes = 0;
  std::streamsize limit = std::numeric_limits<std::streamsize>::max();

protected:
  std::streamsize xsputn(const char *const s, const std::streamsize n) override
  {
    writes++;
    const std::streamsize taken = n < limit ? n : limit;
    bytes.append(s, static_cast<size_t>(taken));
    return taken;
  }

  int_type overflow(const int_type c) override
  {
    if (!traits_type::eq_int_type(c, traits_type::eof())) bytes.push_back(traits_type::to_char_type(c));
    return traits_type::not_eof(c);
  }
};

// The render goes to the stream as one write, so a stream buffer sees
// one call however long the render is.
TEST(RenderToStream, WritesTheBytesThatRenderReturnsInOneWrite)
{
  const Node data{NodeMap{{"a", Node{std::string(50000, 'x')}}}};
  const mustache::Template t = compiled("<{{{a}}}>{{a}}");
  CountingBuffer buffer;
  std::ostream out(&buffer);
  ASSERT_TRUE(mustache::render_to(out, t, data));
  EXPECT_EQ(buffer.bytes, "<" + std::string(50000, 'x') + ">" + std::string(50000, 'x'));
  EXPECT_EQ(buffer.writes, 1);
  EXPECT_TRUE(out.good());
}

// std::print does not pad to the width of the stream, and render_to
// does not either.
TEST(RenderToStream, IgnoresTheWidthOfTheStream)
{
  const mustache::Template t = compiled(kSource);
  const Node data = page();
  std::ostringstream out;
  out << std::setw(80) << std::setfill('.');
  ASSERT_TRUE(mustache::render_to(out, t, data));
  EXPECT_EQ(out.str(), kRendered);
}

TEST(RenderToStream, AppendsToTheBytesThatTheStreamHolds)
{
  const mustache::Template t = compiled(kSource);
  const Node data = page();
  std::ostringstream out("head:");
  out.seekp(0, std::ios_base::end);
  ASSERT_TRUE(mustache::render_to(out, t, data));
  ASSERT_TRUE(mustache::render_to(out, t, data));
  EXPECT_EQ(out.str(), "head:" + std::string(kRendered) + std::string(kRendered));
}

// An error of the render writes nothing and does not touch the state
// of the stream.
TEST(RenderToStream, WritesNothingOnAnErrorOfTheRender)
{
  const mustache::Template t = compiled("ab{{a}}");
  const Node data{NodeMap{{"a", Node{NodeMap{{"x", Node{true}}}}}}};
  CountingBuffer buffer;
  std::ostream out(&buffer);
  const std::expected<void, mustache::Error> r = mustache::render_to(out, t, data);
  ASSERT_FALSE(r);
  EXPECT_EQ(r.error().fault, mustache::Fault::not_text);
  EXPECT_EQ(buffer.writes, 0);
  EXPECT_TRUE(out.good());
}

// A stream with failbit set takes no bytes, as with std::print. The
// call returns a value, and the state stays set. Whether a failed
// sentry adds badbit is up to the standard library: the one of MSVC
// sets it in ostream::write, libstdc++ and libc++ do not. So the test
// does not read badbit.
TEST(RenderToStream, WritesNothingToAStreamWithFailbitSet)
{
  const mustache::Template t = compiled(kSource);
  const Node data = page();
  CountingBuffer buffer;
  std::ostream out(&buffer);
  out.setstate(std::ios_base::failbit);
  EXPECT_TRUE(mustache::render_to(out, t, data));
  EXPECT_EQ(buffer.writes, 0);
  EXPECT_TRUE(buffer.bytes.empty());
  EXPECT_TRUE(out.fail());
}

// A buffer that takes too few bytes sets badbit. Without an exceptions
// mask the call returns a value and the caller reads the state.
TEST(RenderToStream, LeavesAFailedWriteInTheStateOfTheStream)
{
  const mustache::Template t = compiled(kSource);
  const Node data = page();
  CountingBuffer buffer;
  buffer.limit = 3;
  std::ostream out(&buffer);
  EXPECT_TRUE(mustache::render_to(out, t, data));
  EXPECT_TRUE(out.bad());
  EXPECT_EQ(buffer.bytes, kRendered.substr(0, 3));
}

// With badbit in the exceptions mask the failed write throws the
// exception of the standard library, which is a std::system_error.
TEST(RenderToStream, ThrowsWhenTheExceptionsMaskHoldsTheFailedBit)
{
  const mustache::Template t = compiled(kSource);
  const Node data = page();
  CountingBuffer buffer;
  buffer.limit = 3;
  std::ostream out(&buffer);
  out.exceptions(std::ios_base::badbit);
  EXPECT_THROW(static_cast<void>(mustache::render_to(out, t, data)), std::ios_base::failure);
  EXPECT_TRUE(out.bad());
}

// An error of the render comes back as a value, and no exception of the
// mask is thrown, because no write was made.
TEST(RenderToStream, ReturnsTheErrorOfTheRenderWithAnExceptionsMask)
{
  const mustache::Template t = compiled("ab{{a}}");
  const Node data{NodeMap{{"a", Node{NodeMap{{"x", Node{true}}}}}}};
  CountingBuffer buffer;
  std::ostream out(&buffer);
  out.exceptions(std::ios_base::badbit | std::ios_base::failbit);
  const std::expected<void, mustache::Error> r = mustache::render_to(out, t, data);
  ASSERT_FALSE(r);
  EXPECT_EQ(r.error().fault, mustache::Fault::not_text);
}

}
