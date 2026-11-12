#include <cstddef>
#include <cstdint>
#include <cstring>
#include "net/base/load_flags_to_string.h"
#include "net/base/privacy_mode.h"
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  if (size < sizeof(int32_t)) return 0;
  int32_t value;
  std::memcpy(&value, data, sizeof(value));
  // Successful fuzz inputs are restricted to defined load flags; invalid CHECK
  // behavior is covered by Rust unit cases and isolated death tests.
  constexpr uint32_t kMask = (1u << 20) - 1;
  int flags = static_cast<int>(static_cast<uint32_t>(value) & kMask);
  (void)net::LoadFlagsToString(flags);
  (void)net::PrivacyModeToDebugString(static_cast<net::PrivacyMode>(static_cast<uint32_t>(value) % 4));
  return 0;
}
