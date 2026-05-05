#include "ecewo.h"
#include "ecewo-mock.h"
#include "tester.h"
#include "tests.h"

#include <stdio.h>

static void setup_all_routes(ecewo_app_t *app) {
  ECEWO_GET(app, "/set-simple", handler_set_simple_cookie);
  ECEWO_GET(app, "/set-complex", handler_set_complex_cookie);
  ECEWO_GET(app, "/get-cookie", handler_get_cookie);
  ECEWO_GET(app, "/delete-cookie", handler_delete_cookie);
  ECEWO_GET(app, "/utf8-cookie", handler_utf8_cookie);
}

int main(void) {
  if (mock_init(setup_all_routes) != 0) {
    printf("ERROR: Failed to initialize mock server\n");
    return 1;
  }

  RUN_TEST(test_cookie_set_simple);
  RUN_TEST(test_cookie_set_complex);
  RUN_TEST(test_cookie_get);
  RUN_TEST(test_cookie_get_not_found);
  RUN_TEST(test_cookie_get_multiple);
  RUN_TEST(test_cookie_delete);
  RUN_TEST(test_cookie_url_encoded);

  mock_cleanup();
  return 0;
}
