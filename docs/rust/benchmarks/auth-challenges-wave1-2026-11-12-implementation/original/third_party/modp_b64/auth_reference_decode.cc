#include "modp_b64.h"
extern "C" size_t auth_reference_decode(char* out, const char* in, size_t n) {
  return modp_b64_decode(out, in, n, ModpDecodePolicy::kStrict);
}
