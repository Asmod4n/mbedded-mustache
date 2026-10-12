#include <gtest/gtest.h>

extern "C" {
const char *c_api_parse_gives_eilseq(void);
const char *c_api_render_hands_back_the_string(void);
const char *c_api_not_text_gives_einval(void);
const char *c_api_nesting_gives_eloop(void);
const char *c_api_next_names_the_missing_setter(void);
const char *c_api_foreign_handle_gives_ebadf(void);
const char *c_api_render_inside_render_gives_ebusy(void);
}

namespace {

// The checks of the C API are written in C, in test/c_api.c, so that
// file shows that the header compiles as C. Each one gives NULL or the
// name of the check that did not hold.
void
holds(const char *const failed)
{
  EXPECT_EQ(failed, nullptr) << (failed != nullptr ? failed : "");
}

TEST(CApi, ParseGivesEilseq) { holds(c_api_parse_gives_eilseq()); }
TEST(CApi, RenderHandsBackTheString) { holds(c_api_render_hands_back_the_string()); }
TEST(CApi, NotTextGivesEinval) { holds(c_api_not_text_gives_einval()); }
TEST(CApi, NestingGivesEloop) { holds(c_api_nesting_gives_eloop()); }
TEST(CApi, NextNamesTheMissingSetter) { holds(c_api_next_names_the_missing_setter()); }
TEST(CApi, ForeignHandleGivesEbadf) { holds(c_api_foreign_handle_gives_ebadf()); }
TEST(CApi, RenderInsideRenderGivesEbusy) { holds(c_api_render_inside_render_gives_ebusy()); }

}
