#include <functional>
#include <optional>
#include <variant>
struct Partials { int n; };
std::optional<const Partials &> named(const Partials &p, const bool found)
{
  if (!found) return std::nullopt;
  return p;
}
int called(const std::function_ref<int(int)> f) { return f(1); }
#ifdef VARIANT_OF_REFERENCES
std::variant<const int &, const long &> entry(const int &i) { return i; }
#endif
int main() { const Partials p{3}; return named(p, true)->n + called([](const int x) { return x; }) - 4; }
