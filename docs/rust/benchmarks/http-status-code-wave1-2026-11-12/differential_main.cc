#include <bit>
#include <cstdint>
#include <cstdio>
#include "net/http/http_status_code_differential.h"
int main() {
 net::http_status_code_test::Exhaustive();
 uint32_t x = 0x59328741;
 for (int i = 0; i < 1000000; ++i) {
   x ^= x << 13; x ^= x >> 17; x ^= x << 5;
   net::http_status_code_test::Check(std::bit_cast<int32_t>(x));
 }
 puts("PASS all frozen entries, 131073 broad integers, boundaries, 1000000 differential fuzz executions; three fallback layouts each");
}


