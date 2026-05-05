#ifndef ECEWO_COOKIE_H
#define ECEWO_COOKIE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ecewo-cookie-export.h"

typedef struct ecewo_request_s ecewo_request_t;
typedef struct ecewo_response_s ecewo_response_t;
typedef struct ecewo_cookie_options_s ecewo_cookie_options_t;

/** SameSite attribute values for the Set-Cookie header. */
typedef enum {
  ECEWO_COOKIE_SAMESITE_UNSET = 0,
  ECEWO_COOKIE_SAMESITE_STRICT = 1,
  ECEWO_COOKIE_SAMESITE_LAX = 2,
  ECEWO_COOKIE_SAMESITE_NONE = 3
} ecewo_cookie_samesite_t;

/** Allocate a zero-initialized options builder. Returns NULL on allocation failure. */
ECEWO_COOKIE_EXPORT ecewo_cookie_options_t *ecewo_cookie_options_new(void);

/** Free an options builder created by ecewo_cookie_options_new(). Safe to call with NULL. */
ECEWO_COOKIE_EXPORT void ecewo_cookie_options_free(ecewo_cookie_options_t *opts);

/** Set Max-Age in seconds. Use a negative value for a session cookie (default).
 *  Use 0 together with an empty value to delete a cookie. */
ECEWO_COOKIE_EXPORT void ecewo_cookie_options_set_max_age(ecewo_cookie_options_t *opts, int seconds);

/** Set the Path attribute (default "/"). The pointer must remain valid until ecewo_cookie_set() returns. */
ECEWO_COOKIE_EXPORT void ecewo_cookie_options_set_path(ecewo_cookie_options_t *opts, const char *path);

/** Set the Domain attribute (default unset). The pointer must remain valid until ecewo_cookie_set() returns. */
ECEWO_COOKIE_EXPORT void ecewo_cookie_options_set_domain(ecewo_cookie_options_t *opts, const char *domain);

/** Set the SameSite attribute. */
ECEWO_COOKIE_EXPORT void ecewo_cookie_options_set_same_site(ecewo_cookie_options_t *opts, ecewo_cookie_samesite_t same_site);

/** Set the HttpOnly flag (non-zero = enabled). */
ECEWO_COOKIE_EXPORT void ecewo_cookie_options_set_http_only(ecewo_cookie_options_t *opts, int http_only);

/** Set the Secure flag (non-zero = enabled). Required when SameSite=None. */
ECEWO_COOKIE_EXPORT void ecewo_cookie_options_set_secure(ecewo_cookie_options_t *opts, int secure);

/**
 * Look up a cookie by name in the incoming Cookie header.
 *
 * Returns the URL-decoded value on success, allocated in the per-request arena,
 * or NULL if the cookie is not present or the name is invalid. The returned
 * pointer is valid until the response is sent.
 */
ECEWO_COOKIE_EXPORT const char *ecewo_cookie_get(const ecewo_request_t *req, const char *name);

/**
 * Append a Set-Cookie response header for the given name and value.
 *
 * `value` is URL-encoded automatically. `options` may be NULL for defaults
 * (session cookie, Path=/).
 *
 * Returns 0 on success, -1 on validation failure or allocation error.
 */
ECEWO_COOKIE_EXPORT int ecewo_cookie_set(ecewo_response_t *res, const char *name, const char *value, const ecewo_cookie_options_t *options);

#ifdef __cplusplus
}
#endif

#endif
