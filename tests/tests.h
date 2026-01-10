#ifndef ECEWO_COOKIE_TESTS
#define ECEWO_COOKIE_TESTS

#include "ecewo.h"

void handler_set_simple_cookie(Req *req, Res *res);
void handler_set_complex_cookie(Req *req, Res *res);
void handler_get_cookie(Req *req, Res *res);
void handler_delete_cookie(Req *req, Res *res);
void handler_utf8_cookie(Req *req, Res *res);

int test_cookie_set_simple(void);
int test_cookie_set_complex(void);
int test_cookie_get(void);
int test_cookie_get_not_found(void);
int test_cookie_get_multiple(void);
int test_cookie_delete(void);
int test_cookie_url_encoded(void);

#endif