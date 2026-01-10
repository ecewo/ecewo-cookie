#include "ecewo-mock.h"
#include "tests.h"
#include "tester.h"

void setup_all_routes(void) {
  get("/set-simple", handler_set_simple_cookie);
  get("/set-complex", handler_set_complex_cookie);
  get("/get-cookie", handler_get_cookie);
  get("/delete-cookie", handler_delete_cookie);
  get("/utf8-cookie", handler_utf8_cookie);
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