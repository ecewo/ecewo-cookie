#include "ecewo-cookie.h"

#include "ecewo.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_COOKIE_NAME_LEN 256
#define MAX_COOKIE_VALUE_LEN 4096
#define MAX_COOKIE_SIZE 4096
#define MAX_COOKIES_PER_REQUEST 50

struct ecewo_cookie_options_s {
  int max_age;
  const char *path;
  const char *domain;
  ecewo_cookie_samesite_t same_site;
  bool http_only;
  bool secure;
};

typedef struct {
  char *items;
  size_t count;
  size_t capacity;
} cookie_sb_t;

// RFC 2616 token characters: VCHAR except separators.
static bool is_token_char(unsigned char c) {
  if (c < 0x21 || c > 0x7E)
    return false;

  switch (c) {
  case '(':
  case ')':
  case '<':
  case '>':
  case '@':
  case ',':
  case ';':
  case ':':
  case '\\':
  case '"':
  case '/':
  case '[':
  case ']':
  case '?':
  case '=':
  case '{':
  case '}':
  case ' ':
  case '\t':
    return false;
  default:
    return true;
  }
}

static bool is_valid_cookie_name(const char *name) {
  if (!name || *name == '\0')
    return false;

  size_t len = strlen(name);
  if (len > MAX_COOKIE_NAME_LEN)
    return false;

  for (size_t i = 0; i < len; i++) {
    if (!is_token_char((unsigned char)name[i]))
      return false;
  }
  return true;
}

static int hex_nibble(char c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  return -1;
}

static char *url_decode(ecewo_arena_t *arena, const char *src, size_t src_len) {
  char *decoded = ecewo_alloc(arena, src_len + 1);
  if (!decoded)
    return NULL;

  size_t i = 0, j = 0;
  while (i < src_len) {
    if (src[i] == '%' && i + 2 < src_len) {
      int h = hex_nibble(src[i + 1]);
      int l = hex_nibble(src[i + 2]);
      if (h >= 0 && l >= 0) {
        decoded[j++] = (char)((h << 4) | l);
        i += 3;
        continue;
      }
    }
    decoded[j++] = src[i++];
  }
  decoded[j] = '\0';
  return decoded;
}

static void trim_whitespace(const char **start, size_t *len) {
  while (*len > 0 && isspace((unsigned char)**start)) {
    (*start)++;
    (*len)--;
  }
  while (*len > 0 && isspace((unsigned char)(*start)[*len - 1])) {
    (*len)--;
  }
}

static bool needs_encoding(unsigned char c) {
  if (c < 0x21 || c > 0x7E)
    return true;

  switch (c) {
  case '"':
  case ',':
  case ';':
  case '\\':
  case ' ':
    return true;
  default:
    return false;
  }
}

static bool sb_append_byte(ecewo_arena_t *arena, cookie_sb_t *sb, char c) {
  size_t old_cap = sb->capacity;
  if (sb->count + 1 > old_cap) {
    size_t new_cap = old_cap == 0 ? 128 : old_cap * 2;
    char *grown = ecewo_realloc(arena, sb->items, old_cap, new_cap);
    if (!grown)
      return false;
    sb->items = grown;
    sb->capacity = new_cap;
  }
  sb->items[sb->count++] = c;
  return true;
}

static bool sb_append_bytes(ecewo_arena_t *arena, cookie_sb_t *sb, const char *data, size_t len) {
  size_t old_cap = sb->capacity;
  if (sb->count + len > old_cap) {
    size_t new_cap = old_cap == 0 ? 128 : old_cap;
    while (sb->count + len > new_cap)
      new_cap *= 2;
    char *grown = ecewo_realloc(arena, sb->items, old_cap, new_cap);
    if (!grown)
      return false;
    sb->items = grown;
    sb->capacity = new_cap;
  }
  memcpy(sb->items + sb->count, data, len);
  sb->count += len;
  return true;
}

static bool sb_append_cstr(ecewo_arena_t *arena, cookie_sb_t *sb, const char *s) {
  return sb_append_bytes(arena, sb, s, strlen(s));
}

static bool sb_append_encoded(ecewo_arena_t *arena, cookie_sb_t *sb, const char *value) {
  static const char hex[] = "0123456789ABCDEF";
  for (const char *p = value; *p; p++) {
    unsigned char c = (unsigned char)*p;
    if (needs_encoding(c)) {
      char esc[3] = { '%', hex[c >> 4], hex[c & 0xF] };
      if (!sb_append_bytes(arena, sb, esc, 3))
        return false;
    } else {
      if (!sb_append_byte(arena, sb, (char)c))
        return false;
    }
  }
  return true;
}

static bool sb_append_max_age(ecewo_arena_t *arena, cookie_sb_t *sb, int seconds) {
  char buf[24];
  int n = snprintf(buf, sizeof(buf), "%d", seconds);
  if (n < 0 || (size_t)n >= sizeof(buf))
    return false;
  return sb_append_bytes(arena, sb, buf, (size_t)n);
}

static bool sb_append_expires(ecewo_arena_t *arena, cookie_sb_t *sb, int max_age_seconds) {
  time_t expire_time = time(NULL) + max_age_seconds;
  struct tm gmt;
#ifdef _WIN32
  if (gmtime_s(&gmt, &expire_time) != 0)
    return false;
#else
  if (!gmtime_r(&expire_time, &gmt))
    return false;
#endif

  char buf[64];
  size_t n = strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", &gmt);
  if (n == 0)
    return false;
  return sb_append_bytes(arena, sb, buf, n);
}

static const char *same_site_str(ecewo_cookie_samesite_t s) {
  switch (s) {
  case ECEWO_COOKIE_SAMESITE_STRICT:
    return "Strict";
  case ECEWO_COOKIE_SAMESITE_LAX:
    return "Lax";
  case ECEWO_COOKIE_SAMESITE_NONE:
    return "None";
  case ECEWO_COOKIE_SAMESITE_UNSET:
  default:
    return NULL;
  }
}

ecewo_cookie_options_t *ecewo_cookie_options_new(void) {
  ecewo_cookie_options_t *opts = calloc(1, sizeof(*opts));
  if (!opts)
    return NULL;
  opts->max_age = -1;
  return opts;
}

void ecewo_cookie_options_free(ecewo_cookie_options_t *opts) {
  free(opts);
}

void ecewo_cookie_options_set_max_age(ecewo_cookie_options_t *opts, int seconds) {
  if (opts)
    opts->max_age = seconds;
}

void ecewo_cookie_options_set_path(ecewo_cookie_options_t *opts, const char *path) {
  if (opts)
    opts->path = path;
}

void ecewo_cookie_options_set_domain(ecewo_cookie_options_t *opts, const char *domain) {
  if (opts)
    opts->domain = domain;
}

void ecewo_cookie_options_set_same_site(ecewo_cookie_options_t *opts, ecewo_cookie_samesite_t same_site) {
  if (opts)
    opts->same_site = same_site;
}

void ecewo_cookie_options_set_http_only(ecewo_cookie_options_t *opts, int http_only) {
  if (opts)
    opts->http_only = http_only != 0;
}

void ecewo_cookie_options_set_secure(ecewo_cookie_options_t *opts, int secure) {
  if (opts)
    opts->secure = secure != 0;
}

const char *ecewo_cookie_get(const ecewo_request_t *req, const char *name) {
  if (!req || !name)
    return NULL;

  ecewo_arena_t *arena = ecewo_req_arena(req);
  if (!arena)
    return NULL;

  if (!is_valid_cookie_name(name)) {
    fprintf(stderr, "ecewo_cookie_get: invalid cookie name '%s' (must be RFC 6265 token)\n", name);
    return NULL;
  }

  const char *cookie_header = ecewo_header_get(req, "Cookie");
  if (!cookie_header)
    return NULL;

  size_t name_len = strlen(name);
  const char *pos = cookie_header;
  int cookie_count = 0;

  while (pos && cookie_count < MAX_COOKIES_PER_REQUEST) {
    while (*pos && isspace((unsigned char)*pos))
      pos++;

    if (!*pos)
      break;

    cookie_count++;

    const char *cookie_end = strchr(pos, ';');
    size_t cookie_len = cookie_end ? (size_t)(cookie_end - pos) : strlen(pos);

    if (cookie_len > MAX_COOKIE_SIZE) {
      fprintf(stderr, "ecewo_cookie_get: cookie too large (%zu bytes)\n", cookie_len);
      pos = cookie_end ? cookie_end + 1 : NULL;
      continue;
    }

    const char *eq = memchr(pos, '=', cookie_len);
    if (!eq) {
      pos = cookie_end ? cookie_end + 1 : NULL;
      continue;
    }

    const char *current_name = pos;
    size_t current_name_len = (size_t)(eq - pos);
    trim_whitespace(&current_name, &current_name_len);

    if (current_name_len == name_len && strncmp(current_name, name, name_len) == 0) {
      const char *value_start = eq + 1;
      size_t value_len = cookie_len - (size_t)(eq - pos) - 1;
      trim_whitespace(&value_start, &value_len);

      if (value_len == 0) {
        char *result = ecewo_alloc(arena, 1);
        if (result)
          result[0] = '\0';
        return result;
      }

      if (value_len >= 2 && value_start[0] == '"' && value_start[value_len - 1] == '"') {
        value_start++;
        value_len -= 2;
      }

      return url_decode(arena, value_start, value_len);
    }

    pos = cookie_end ? cookie_end + 1 : NULL;
  }

  if (cookie_count >= MAX_COOKIES_PER_REQUEST)
    fprintf(stderr, "ecewo_cookie_get: too many cookies in request\n");

  return NULL;
}

int ecewo_cookie_set(ecewo_response_t *res, const char *name, const char *value, const ecewo_cookie_options_t *options) {
  if (!res || !name || !value) {
    fprintf(stderr, "ecewo_cookie_set: invalid arguments\n");
    return -1;
  }

  if (!is_valid_cookie_name(name)) {
    fprintf(stderr, "ecewo_cookie_set: invalid cookie name '%s' (must be RFC 6265 token)\n", name);
    return -1;
  }

  if (strlen(value) > MAX_COOKIE_VALUE_LEN) {
    fprintf(stderr, "ecewo_cookie_set: value too large (%zu bytes, max %d)\n",
            strlen(value), MAX_COOKIE_VALUE_LEN);
    return -1;
  }

  int max_age = options ? options->max_age : -1;
  const char *path = (options && options->path) ? options->path : "/";
  const char *domain = options ? options->domain : NULL;
  ecewo_cookie_samesite_t ss = options ? options->same_site : ECEWO_COOKIE_SAMESITE_UNSET;
  bool http_only = options ? options->http_only : false;
  bool secure = options ? options->secure : false;

  if (ss == ECEWO_COOKIE_SAMESITE_NONE && !secure) {
    fprintf(stderr, "ecewo_cookie_set: SameSite=None requires Secure\n");
    return -1;
  }

  ecewo_arena_t *scratch = ecewo_arena_borrow();
  if (!scratch) {
    fprintf(stderr, "ecewo_cookie_set: failed to borrow scratch arena\n");
    return -1;
  }

  cookie_sb_t sb = { 0 };
  bool ok = sb_append_cstr(scratch, &sb, name);
  ok = ok && sb_append_byte(scratch, &sb, '=');
  ok = ok && sb_append_encoded(scratch, &sb, value);

  if (max_age >= 0) {
    ok = ok && sb_append_cstr(scratch, &sb, "; Max-Age=");
    ok = ok && sb_append_max_age(scratch, &sb, max_age);
    ok = ok && sb_append_cstr(scratch, &sb, "; Expires=");
    ok = ok && sb_append_expires(scratch, &sb, max_age);
  }

  ok = ok && sb_append_cstr(scratch, &sb, "; Path=");
  ok = ok && sb_append_cstr(scratch, &sb, path);

  if (domain && *domain) {
    ok = ok && sb_append_cstr(scratch, &sb, "; Domain=");
    ok = ok && sb_append_cstr(scratch, &sb, domain);
  }

  const char *ss_str = same_site_str(ss);
  if (ss_str) {
    ok = ok && sb_append_cstr(scratch, &sb, "; SameSite=");
    ok = ok && sb_append_cstr(scratch, &sb, ss_str);
  }

  if (http_only)
    ok = ok && sb_append_cstr(scratch, &sb, "; HttpOnly");

  if (secure)
    ok = ok && sb_append_cstr(scratch, &sb, "; Secure");

  ok = ok && sb_append_byte(scratch, &sb, '\0');

  if (!ok) {
    ecewo_arena_return(scratch);
    fprintf(stderr, "ecewo_cookie_set: scratch allocation failed\n");
    return -1;
  }

  if (sb.count - 1 > MAX_COOKIE_SIZE) {
    ecewo_arena_return(scratch);
    fprintf(stderr, "ecewo_cookie_set: final cookie too large (%zu bytes, max %d)\n",
            sb.count - 1, MAX_COOKIE_SIZE);
    return -1;
  }

  ecewo_header_set(res, "Set-Cookie", sb.items);
  ecewo_arena_return(scratch);
  return 0;
}
