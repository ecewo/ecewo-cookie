#include "ecewo-cookie.h"
#include "ecewo-mock.h"
#include "tester.h"
#include "tests.h"

#include <string.h>

void handler_set_simple_cookie(ecewo_request_t *req, ecewo_response_t *res) {
  (void)req;
  ecewo_cookie_set(res, "theme", "dark", NULL);
  ecewo_send_text(res, ECEWO_OK, "Cookie set");
}

void handler_set_complex_cookie(ecewo_request_t *req, ecewo_response_t *res) {
  (void)req;
  ecewo_cookie_options_t *opts = ecewo_cookie_options_new();
  ecewo_cookie_options_set_max_age(opts, 3600);
  ecewo_cookie_options_set_path(opts, "/");
  ecewo_cookie_options_set_same_site(opts, ECEWO_COOKIE_SAMESITE_STRICT);
  ecewo_cookie_options_set_http_only(opts, true);
  ecewo_cookie_options_set_secure(opts, false);

  ecewo_cookie_set(res, "session_id", "abc123", opts);
  ecewo_cookie_options_free(opts);

  ecewo_send_text(res, ECEWO_OK, "Complex cookie set");
}

void handler_get_cookie(ecewo_request_t *req, ecewo_response_t *res) {
  const char *value = ecewo_cookie_get(req, "user");
  if (value) {
    ecewo_send_text(res, ECEWO_OK, value);
  } else {
    ecewo_send_text(res, ECEWO_NOT_FOUND, "Cookie not found");
  }
}

void handler_delete_cookie(ecewo_request_t *req, ecewo_response_t *res) {
  (void)req;
  ecewo_cookie_options_t *opts = ecewo_cookie_options_new();
  ecewo_cookie_options_set_max_age(opts, 0);

  ecewo_cookie_set(res, "session_id", "", opts);
  ecewo_cookie_options_free(opts);

  ecewo_send_text(res, ECEWO_OK, "Cookie deleted");
}

void handler_utf8_cookie(ecewo_request_t *req, ecewo_response_t *res) {
  (void)req;
  ecewo_cookie_set(res, "greeting", "merhaba dünya", NULL);
  ecewo_send_text(res, ECEWO_OK, "UTF-8 cookie set");
}

int test_cookie_set_simple(void) {
  MockParams params = {
    .method = MOCK_GET,
    .path = "/set-simple",
    .body = NULL,
    .headers = NULL,
    .header_count = 0
  };

  MockResponse res = request(&params);

  ASSERT_EQ(200, res.status_code);
  ASSERT_EQ_STR("Cookie set", res.body);

  const char *set_cookie = mock_get_header(&res, "Set-Cookie");
  ASSERT_NOT_NULL(set_cookie);
  ASSERT_NOT_NULL(strstr(set_cookie, "theme=dark"));
  ASSERT_NOT_NULL(strstr(set_cookie, "Path=/"));

  free_request(&res);
  RETURN_OK();
}

int test_cookie_set_complex(void) {
  MockParams params = {
    .method = MOCK_GET,
    .path = "/set-complex",
    .body = NULL,
    .headers = NULL,
    .header_count = 0
  };

  MockResponse res = request(&params);

  ASSERT_EQ(200, res.status_code);
  ASSERT_EQ_STR("Complex cookie set", res.body);

  const char *set_cookie = mock_get_header(&res, "Set-Cookie");
  ASSERT_NOT_NULL(set_cookie);
  ASSERT_NOT_NULL(strstr(set_cookie, "session_id=abc123"));
  ASSERT_NOT_NULL(strstr(set_cookie, "Max-Age=3600"));
  ASSERT_NOT_NULL(strstr(set_cookie, "Expires="));
  ASSERT_NOT_NULL(strstr(set_cookie, "SameSite=Strict"));
  ASSERT_NOT_NULL(strstr(set_cookie, "HttpOnly"));

  free_request(&res);
  RETURN_OK();
}

int test_cookie_get(void) {
  MockHeaders headers[] = {
    { "Cookie", "user=john_doe" }
  };

  MockParams params = {
    .method = MOCK_GET,
    .path = "/get-cookie",
    .body = NULL,
    .headers = headers,
    .header_count = 1
  };

  MockResponse res = request(&params);

  ASSERT_EQ(200, res.status_code);
  ASSERT_EQ_STR("john_doe", res.body);

  free_request(&res);
  RETURN_OK();
}

int test_cookie_get_not_found(void) {
  MockParams params = {
    .method = MOCK_GET,
    .path = "/get-cookie",
    .body = NULL,
    .headers = NULL,
    .header_count = 0
  };

  MockResponse res = request(&params);

  ASSERT_EQ(404, res.status_code);
  ASSERT_EQ_STR("Cookie not found", res.body);

  free_request(&res);
  RETURN_OK();
}

int test_cookie_get_multiple(void) {
  MockHeaders headers[] = {
    { "Cookie", "first=one; user=target_value; last=three" }
  };

  MockParams params = {
    .method = MOCK_GET,
    .path = "/get-cookie",
    .body = NULL,
    .headers = headers,
    .header_count = 1
  };

  MockResponse res = request(&params);

  ASSERT_EQ(200, res.status_code);
  ASSERT_EQ_STR("target_value", res.body);

  free_request(&res);
  RETURN_OK();
}

int test_cookie_delete(void) {
  MockParams params = {
    .method = MOCK_GET,
    .path = "/delete-cookie",
    .body = NULL,
    .headers = NULL,
    .header_count = 0
  };

  MockResponse res = request(&params);

  ASSERT_EQ(200, res.status_code);
  ASSERT_EQ_STR("Cookie deleted", res.body);

  const char *set_cookie = mock_get_header(&res, "Set-Cookie");
  ASSERT_NOT_NULL(set_cookie);
  ASSERT_NOT_NULL(strstr(set_cookie, "Max-Age=0"));

  free_request(&res);
  RETURN_OK();
}

int test_cookie_url_encoded(void) {
  MockHeaders headers[] = {
    { "Cookie", "user=hello%20world" }
  };

  MockParams params = {
    .method = MOCK_GET,
    .path = "/get-cookie",
    .body = NULL,
    .headers = headers,
    .header_count = 1
  };

  MockResponse res = request(&params);

  ASSERT_EQ(200, res.status_code);
  ASSERT_EQ_STR("hello world", res.body);

  free_request(&res);
  RETURN_OK();
}
