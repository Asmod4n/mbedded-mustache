#include <mustache/mustache.hpp>

#include <cstdio>
#include <map>
#include <string>

#if defined(MEDIA_RUN_TIME)
extern const char *volatile chosen_media_type;
const char *volatile chosen_media_type = "application/json";
#endif

int main()
{
#if defined(MEDIA_RUN_TIME)
  const auto tpl = mustache::compile("Hello, {{name}}! You have {{count}} new {{what}}.", std::string_view(chosen_media_type));
#elif defined(MEDIA)
  const auto tpl = mustache::compile("Hello, {{name}}! You have {{count}} new {{what}}.", MEDIA);
#else
  const auto tpl = mustache::compile("Hello, {{name}}! You have {{count}} new {{what}}.");
#endif
  if (!tpl) return 1;
  const std::map<std::string, std::string> data{{"name", "Ada <admin>"}, {"count", "3"}, {"what", "messages"}};
  const auto answer = mustache::render(*tpl, data);
  if (!answer) return 1;
  std::puts(answer->c_str());
}
