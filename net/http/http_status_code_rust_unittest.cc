#include "net/http/http_status_code_differential.h"
#include "testing/gtest/include/gtest/gtest.h"
TEST(HttpStatusCodeRust, FrozenDifferential) {
  net::http_status_code_test::Exhaustive();
}
