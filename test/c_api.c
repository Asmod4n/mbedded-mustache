#include <mustache-c/mustache.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>

/* This file is C, so it also shows that the header is C: a handle of an
 * incomplete type, no struct a caller must build, and every function
 * gives 0 or a code, or -1 with errno. */

struct value {
  int                 kind;
  const char         *text;
  const struct value *child;
};

struct user {
  char answer[4096];
};

static const void *
find(void *user, const void *value, const char *key, size_t size)
{
  const struct value *v = value;
  (void)user;
  if (v->kind != MUSTACHE_KIND_MAP || size != 1 || key[0] != 'a') return NULL;
  return v->child;
}

static int
kind(void *user, const void *value)
{
  (void)user;
  return ((const struct value *)value)->kind;
}

static const char *
text(void *user, const void *value, size_t *size)
{
  const struct value *v = value;
  (void)user;
  *size = strlen(v->text);
  return v->text;
}

static size_t
size_of(void *user, const void *value)
{
  (void)user;
  (void)value;
  return 1;
}

static const void *
element(void *user, const void *value, size_t index)
{
  (void)user;
  (void)index;
  return ((const struct value *)value)->child;
}

static const mustache_template *
partial(void *user, const char *name, size_t size)
{
  (void)user;
  (void)name;
  (void)size;
  return NULL;
}

static char *
new_string(void *user, size_t capacity, void **string, size_t *real_capacity)
{
  char *bytes = malloc(capacity);
  (void)user;
  *string = bytes;
  *real_capacity = capacity;
  return bytes;
}

static char *
grow(void *user, void **string, size_t size, size_t capacity, size_t *real_capacity)
{
  char *bytes = realloc(*string, capacity);
  (void)user;
  (void)size;
  if (bytes == NULL) return NULL;
  *string = bytes;
  *real_capacity = capacity;
  return bytes;
}

static int
done(void *user, void *string, size_t size)
{
  struct user *u = user;
  memcpy(u->answer, string, size);
  u->answer[size] = '\0';
  free(string);
  return 0;
}

/* A render owns the root it is given and releases it once, whether the
 * render succeeds or not. The count shows that. */
static int released = 0;

static void
release(const void *root)
{
  (void)root;
  released++;
}


static mustache_template *reentered = NULL;
static int                reentered_result = 0;
static int                reentered_errno = 0;

static int
kind_that_renders_again(void *user, const void *value)
{
  void *string = NULL;
  reentered_result = mustache_render(reentered, value, NULL, user, &string);
  reentered_errno = errno;
  return ((const struct value *)value)->kind;
}

#define CHECK(ok, what) \
  do { \
    if (!(ok)) return what; \
  } while (0)

static mustache_template *
compiled(const char *source)
{
  mustache_template *tpl = NULL;
  if (mustache_compile(source, strlen(source), &tpl) != 0) return tpl;
  mustache_set_find(tpl, find);
  mustache_set_kind(tpl, kind);
  mustache_set_text(tpl, text);
  mustache_set_size(tpl, size_of);
  mustache_set_element(tpl, element);
  mustache_set_partial(tpl, partial);
  mustache_set_new_string(tpl, new_string);
  mustache_set_grow(tpl, grow);
  mustache_set_done(tpl, done);
  return tpl;
}

static int
message_is(const mustache_template *tpl, const char *expected)
{
  const char *message = NULL;
  size_t size = 0;
  if (mustache_message(tpl, &message, &size) != 0) return 0;
  return size == strlen(expected) && memcmp(message, expected, size) == 0;
}

/* Each function returns NULL when every check in it holds, or the name
 * of the first check that does not. test/c_api_test.cpp calls them, so
 * GoogleTest counts each one, and this file stays C. */

/* A source that does not compile gives EILSEQ. The template still
 * exists, so the caller reads the message and disposes it. */
const char *
c_api_parse_gives_eilseq(void)
{
  mustache_template *tpl = NULL;
  const int r = mustache_compile("{{#a}}", 6, &tpl);
  const int e = errno;
  const int has_message = tpl != NULL && message_is(tpl, "an unclosed section");
  mustache_dispose_template(tpl);
  CHECK(r == -1 && e == EILSEQ, "parse gives EILSEQ");
  CHECK(has_message, "parse message");
  return NULL;
}

/* A render that succeeds hands back the string of the host, and
 * releases its root once. */
const char *
c_api_render_hands_back_the_string(void)
{
  struct user u = {{0}};
  void *string = NULL;
  const struct value world = {MUSTACHE_KIND_TEXT, "World", NULL};
  const struct value root = {MUSTACHE_KIND_MAP, NULL, &world};
  const int before = released;
  mustache_template *tpl = compiled("Hello {{a}}!");
  const int r = mustache_render(tpl, &root, release, &u, &string);
  mustache_dispose_template(tpl);
  CHECK(r == 0 && strcmp(u.answer, "Hello World!") == 0, "render");
  CHECK(released == before + 1, "the render releases its root once");
  return NULL;
}

/* A value that is not text gives EINVAL, and done still runs, so the
 * host frees its string. */
const char *
c_api_not_text_gives_einval(void)
{
  struct user u = {{0}};
  void *string = NULL;
  const struct value list = {MUSTACHE_KIND_LIST, NULL, NULL};
  const struct value root = {MUSTACHE_KIND_MAP, NULL, &list};
  const int before = released;
  mustache_template *tpl = compiled("{{a}}");
  const int r = mustache_render(tpl, &root, release, &u, &string);
  const int e = errno;
  const int has_message = message_is(tpl, "a value is not text");
  mustache_dispose_template(tpl);
  CHECK(r == -1 && e == EINVAL, "not_text gives EINVAL");
  CHECK(has_message, "not_text message");
  CHECK(released == before + 1, "a failed render releases its root once");
  return NULL;
}

/* A map that holds itself nests without end. The depth limit stops
 * it with ELOOP. */
const char *
c_api_nesting_gives_eloop(void)
{
  struct user u = {{0}};
  void *string = NULL;
  struct value root = {MUSTACHE_KIND_MAP, NULL, NULL};
  char source[512] = "";
  int i;
  int r;
  int e;
  mustache_template *tpl;
  root.child = &root;
  for (i = 0; i < 40; i++) strcat(source, "{{#a}}");
  for (i = 0; i < 40; i++) strcat(source, "{{/a}}");
  tpl = compiled(source);
  r = mustache_render(tpl, &root, release, &u, &string);
  e = errno;
  mustache_dispose_template(tpl);
  CHECK(r == -1 && e == ELOOP, "nesting gives ELOOP");
  return NULL;
}

/* mustache_next names the setter that is still missing, one after the
 * other, and MUSTACHE_RENDER when none is. A render on a template with
 * a missing callback does not start: it gives the same code and only
 * releases the root. */
const char *
c_api_next_names_the_missing_setter(void)
{
  struct user u = {{0}};
  void *string = NULL;
  mustache_template *tpl = NULL;
  const struct value root = {MUSTACHE_KIND_MAP, NULL, NULL};
  const int before = released;
  int r;
  mustache_compile("x", 1, &tpl);
  CHECK(mustache_next(tpl) == MUSTACHE_SET_FIND, "the first missing setter is find");
  r = mustache_render(tpl, &root, release, &u, &string);
  CHECK(r == MUSTACHE_SET_FIND, "a render without callbacks names the missing setter");
  CHECK(released == before + 1, "a render that does not start releases its root once");
  mustache_set_find(tpl, find);
  mustache_set_kind(tpl, kind);
  CHECK(mustache_next(tpl) == MUSTACHE_SET_TEXT, "after find and kind, text is missing");
  r = mustache_set_text(tpl, NULL);
  CHECK(r == -1 && errno == EINVAL, "a null callback gives EINVAL");
  mustache_dispose_template(tpl);
  tpl = compiled("x");
  r = mustache_next(tpl);
  mustache_dispose_template(tpl);
  CHECK(r == MUSTACHE_RENDER, "with every callback set the template can render");
  return NULL;
}

/* Every handle starts with a magic value. A pointer to anything else
 * gives EBADF, and so does NULL. */
const char *
c_api_foreign_handle_gives_ebadf(void)
{
  unsigned long long other = 0;
  const char *message = NULL;
  size_t size = 0;
  CHECK(mustache_next((const mustache_template *)&other) == -1 && errno == EBADF, "a foreign pointer gives EBADF");
  CHECK(mustache_message(NULL, &message, &size) == -1 && errno == EBADF, "NULL gives EBADF");
  return NULL;
}

/* A callback that renders the template it runs in gets EBUSY, and the
 * outer render goes on. */
const char *
c_api_render_inside_render_gives_ebusy(void)
{
  struct user u = {{0}};
  void *string = NULL;
  const struct value world = {MUSTACHE_KIND_TEXT, "World", NULL};
  const struct value root = {MUSTACHE_KIND_MAP, NULL, &world};
  mustache_template *tpl = compiled("Hello {{a}}!");
  int r;
  reentered = tpl;
  mustache_set_kind(tpl, kind_that_renders_again);
  r = mustache_render(tpl, &root, release, &u, &string);
  mustache_dispose_template(tpl);
  CHECK(r == 0 && strcmp(u.answer, "Hello World!") == 0, "the outer render goes on");
  CHECK(reentered_result == -1 && reentered_errno == EBUSY, "a render inside a render gives EBUSY");
  return NULL;
}
