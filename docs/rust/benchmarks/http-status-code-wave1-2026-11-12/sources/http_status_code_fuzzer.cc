#include <cstdint>
#include <cstring>
#include "net/http/http_status_code_differential.h"
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  if (size >= 4) { int32_t code; std::memcpy(&code, data, 4);
    net::http_status_code_test::Check(code); }
  return 0;
}
