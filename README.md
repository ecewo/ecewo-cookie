# ecewo-cookie

A Cookie plugin for [ecewo](https://github.com/ecewo/ecewo) v4.

## Table of Contents

1. [Installation](#installation)
2. [API](#api)
3. [Getting Cookies](#getting-cookies)
4. [Setting Cookies](#setting-cookies)
5. [Deleting Cookies](#deleting-cookies)

## Installation

Add to your `CMakeLists.txt`:

```cmake
ecewo_add(cookie)

target_link_libraries(app PRIVATE
  ecewo::ecewo
  ecewo::cookie
)
```

## API

```c
#include "ecewo-cookie.h"

typedef enum {
  ECEWO_COOKIE_SAMESITE_UNSET = 0,
  ECEWO_COOKIE_SAMESITE_STRICT,
  ECEWO_COOKIE_SAMESITE_LAX,
  ECEWO_COOKIE_SAMESITE_NONE
} ecewo_cookie_samesite_t;

typedef struct ecewo_cookie_options_s ecewo_cookie_options_t;

ecewo_cookie_options_t *ecewo_cookie_options_new(void);
void ecewo_cookie_options_free(ecewo_cookie_options_t *opts);

void ecewo_cookie_options_set_max_age(ecewo_cookie_options_t *opts, int seconds);
void ecewo_cookie_options_set_path(ecewo_cookie_options_t *opts, const char *path);
void ecewo_cookie_options_set_domain(ecewo_cookie_options_t *opts, const char *domain);
void ecewo_cookie_options_set_same_site(ecewo_cookie_options_t *opts, ecewo_cookie_samesite_t same_site);
void ecewo_cookie_options_set_http_only(ecewo_cookie_options_t *opts, int http_only);
void ecewo_cookie_options_set_secure(ecewo_cookie_options_t *opts, int secure);

const char *ecewo_cookie_get(const ecewo_request_t *req, const char *name);
int ecewo_cookie_set(ecewo_response_t *res, const char *name, const char *value, const ecewo_cookie_options_t *options);
```

The options builder is opaque, so all setters are stable across ABI changes, making the plugin safe to call across FFI boundaries.

Returned strings from `ecewo_cookie_get()` are allocated in the per-request arena and stay valid until the response is sent.

## Getting Cookies

Values are URL-decoded automatically; UTF-8 is supported.

```c
#include "ecewo.h"
#include "ecewo-cookie.h"

void cookie_reader(ecewo_request_t *req, ecewo_response_t *res) {
  const char *session_id = ecewo_cookie_get(req, "session_id");

  if (session_id) {
    ecewo_send_text(res, ECEWO_OK, "Welcome back!");
  } else {
    ecewo_send_text(res, ECEWO_UNAUTHORIZED, "No session");
  }
}
```

## Setting Cookies

Values are URL-encoded automatically. Cookie names must be RFC 6265 tokens (ASCII), but values can be full UTF-8.

```c
#include "ecewo.h"
#include "ecewo-cookie.h"

void login_handler(ecewo_request_t *req, ecewo_response_t *res) {
  // Simple cookie with defaults
  ecewo_cookie_set(res, "theme", "dark", NULL);

  // Full options
  ecewo_cookie_options_t *opts = ecewo_cookie_options_new();
  ecewo_cookie_options_set_max_age(opts, 3600); // 1 hour
  ecewo_cookie_options_set_path(opts, "/");
  ecewo_cookie_options_set_same_site(opts, ECEWO_COOKIE_SAMESITE_NONE);
  ecewo_cookie_options_set_http_only(opts, 1);
  ecewo_cookie_options_set_secure(opts, 1); // required for SameSite=None

  ecewo_cookie_set(res, "session_id", "session_id_here", opts);
  ecewo_cookie_options_free(opts);

  ecewo_send_text(res, ECEWO_OK, "Logged in");
}
```

## Deleting Cookies

Set `max_age` to `0` and the value to an empty string:

```c
void logout_handler(ecewo_request_t *req, ecewo_response_t *res) {
  ecewo_cookie_options_t *opts = ecewo_cookie_options_new();
  ecewo_cookie_options_set_max_age(opts, 0);

  ecewo_cookie_set(res, "session_id", "", opts);
  ecewo_cookie_options_free(opts);

  ecewo_send_text(res, ECEWO_OK, "Logged out");
}
```
