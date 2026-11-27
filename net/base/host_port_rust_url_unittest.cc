// Extracted unchanged test cases from the original url_util_unittest.cc; calls
// the Rust-backed wrapper.
#include "net/base/host_port_rust_ffi.h"
#include "testing/gtest/include/gtest/gtest.h"
namespace net {
TEST(UrlUtilTest, ParseHostAndPort) {
  const struct {
    const char* const input;
    bool success;
    const char* const expected_host;
    int expected_port;
  } tests[] = {
      // Valid inputs:
      {"foo:10", true, "foo", 10},
      {"foo", true, "foo", -1},
      {"[1080:0:0:0:8:800:200C:4171]:11", true, "1080:0:0:0:8:800:200C:4171",
       11},
      {"[1080:0:0:0:8:800:200C:4171]", true, "1080:0:0:0:8:800:200C:4171", -1},

      // Because no validation is done on the host, the following are accepted,
      // even though they are invalid names.
      {"]", true, "]", -1},
      {"::1", true, ":", 1},
      // Invalid inputs:
      {"foo:bar", false, "", -1},
      {"foo:", false, "", -1},
      {":", false, "", -1},
      {":80", false, "", -1},
      {"", false, "", -1},
      {"porttoolong:300000", false, "", -1},
      {"usrname@host", false, "", -1},
      {"usrname:password@host", false, "", -1},
      {":password@host", false, "", -1},
      {":password@host:80", false, "", -1},
      {":password@host", false, "", -1},
      {"@host", false, "", -1},
      {"[", false, "", -1},
      {"[]", false, "", -1},
  };

  for (const auto& test : tests) {
    std::string host;
    int port;
    bool ok = host_port_rust::ParseHostAndPort(test.input, &host, &port);
    EXPECT_EQ(test.success, ok);

    if (test.success) {
      EXPECT_EQ(test.expected_host, host);
      EXPECT_EQ(test.expected_port, port);
    }
  }
}

}  // namespace net
