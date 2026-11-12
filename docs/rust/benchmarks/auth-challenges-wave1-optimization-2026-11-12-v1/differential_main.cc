#include <cstdint>
#include <cstdio>

#include "net/http/auth_challenge_test_support.h"
int main() {
  const char alphabet[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/= "
      "\t\r\n,\"";
  uint64_t x = 0x123456789abcdefULL;
  for (unsigned i = 0; i < 1000000; ++i) {
    std::string s = i % 4 == 0 ? "Negotiate " : i % 4 == 1 ? "NTLM " : "";
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    const size_t n = x % 49;
    for (size_t j = 0; j < n; ++j) {
      x ^= x << 13;
      x ^= x >> 7;
      x ^= x << 17;
      s.push_back(i % 3 == 0 ? static_cast<char>(x)
                             : alphabet[x % (sizeof(alphabet) - 1)]);
    }
    net::auth_challenge_test::Check(s);
  }
  std::puts(
      "PASS: 1000000 whole-API differential inputs; each tests both schemes "
      "and five output layouts");
}
