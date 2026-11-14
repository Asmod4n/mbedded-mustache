// Finds the last byte that escape_into of the tree stores, for inputs of n bytes.
// A byte counts as stored when it differs from one of two fill values in two runs.
#include <mustache/out.hpp>
#include <cstdio>
#include <cstdint>
#include <random>
#include <string>
#include <vector>
using namespace mustache;
static const size_t kRoom = 6 * 96 + 256;
static std::vector<char> a(kRoom), b(kRoom);
struct Seen { long over_bound = -1000, over_exact = -1000, over_end = -1000; std::string wb, we; };
static size_t stored_end(const std::string &s, size_t &written)
{
  std::fill(a.begin(), a.end(), char(0xAA));
  std::fill(b.begin(), b.end(), char(0x55));
  char *e = escape_into(a.data(), s);
  escape_into(b.data(), s);
  written = size_t(e - a.data());
  size_t end = 0;
  for (size_t i = 0; i < kRoom; i++) if (a[i] != char(0xAA) || b[i] != char(0x55)) end = i + 1;
  return end;
}
static void see(Seen &m, const std::string &s)
{
  size_t written; const size_t end = stored_end(s, written);
  if (written != escaped_size_of(s)) { printf("WRONG SIZE n=%zu\n", s.size()); }
  const long ob = long(end) - long(kEntityMax * s.size());
  const long oe = long(end) - long(written);
  if (ob > m.over_bound) { m.over_bound = ob; m.wb = s; }
  if (oe > m.over_exact) { m.over_exact = oe; m.we = s; }
}
int main()
{
  const std::string alpha = "a\"<&";
  std::mt19937_64 rng(1);
  long gb = -1000, ge = -1000;
  for (size_t n = 0; n <= 96; n++) {
    Seen m;
    std::string s(n, 'a');
    if (n <= 20) {                       // every pattern of plain and '"'
      for (uint64_t p = 0; p < (uint64_t{1} << n); p++) { for (size_t i = 0; i < n; i++) s[i] = (p >> i & 1) ? '"' : 'a'; see(m, s); }
    }
    if (n <= 10) {                       // every string over a " < &
      uint64_t total = 1; for (size_t i = 0; i < n; i++) total *= 4;
      for (uint64_t p = 0; p < total; p++) { uint64_t q = p; for (size_t i = 0; i < n; i++) { s[i] = alpha[q & 3]; q >>= 2; } see(m, s); }
    }
    if (n > 20) {                        // every pattern of the last 16 bytes, for 6 prefixes
      for (int pre = 0; pre < 6; pre++) {
        for (size_t i = 0; i + 16 < n; i++) s[i] = pre == 0 ? 'a' : pre == 1 ? '"' : pre == 2 ? (i % 4 == 3 ? '"' : 'a') : pre == 3 ? (i % 32 == 31 ? '"' : 'a') : pre == 4 ? (i % 32 >= 24 ? '"' : 'a') : (i % 32 < 9 ? '"' : 'a');
        for (uint64_t p = 0; p < 65536; p++) { for (size_t i = 0; i < 16; i++) s[n - 16 + i] = (p >> i & 1) ? '"' : 'a'; see(m, s); }
      }
      for (int r = 0; r < 300000; r++) { // random patterns, the density of marks from 0 to 100 percent, 5 kinds of mark
        const unsigned d = rng() % 101;
        for (size_t i = 0; i < n; i++) s[i] = rng() % 100 < d ? "\"<&>'"[rng() % 5] : 'a';
        see(m, s);
      }
    }
    printf("n=%2zu end-6n max %ld   end-exact max %ld\n", n, m.over_bound, m.over_exact);
    if (m.over_bound > gb) gb = m.over_bound;
    if (m.over_exact > ge) ge = m.over_exact;
  }
  printf("ALL: store end - 6n max %ld; store end - exact size max %ld\n", gb, ge);
  // the other contexts: written size against escaped_size_max_in, every string of 0 to 6 bytes over an alphabet
  const std::string alpha2 = std::string("a .\"\\'`~^\n\x01<&\xe2\x80\xa8!)", 18);
  for (const Context c : {Context::xml_text, Context::json_string, Context::js_string, Context::css_string, Context::troff_text}) {
    long worst = -1000; std::string w;
    for (size_t n = 0; n <= 5; n++) {
      uint64_t total = 1; for (size_t i = 0; i < n; i++) total *= alpha2.size();
      std::string s(n, 'a');
      for (uint64_t p = 0; p < total; p++) {
        uint64_t q = p; for (size_t i = 0; i < n; i++) { s[i] = alpha2[q % alpha2.size()]; q /= alpha2.size(); }
        std::fill(a.begin(), a.end(), 0);
        const long got = escaped_into_in(c, a.data(), s) - a.data();
        const long over = got - long(escaped_size_max_in(c, n));
        if (over > worst) { worst = over; w = s; }
      }
    }
    printf("context %d: written - escaped_size_max_in max %ld (input length %zu)\n", int(c), worst, w.size());
  }
}
