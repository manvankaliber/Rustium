#include <windows.h>

#include <process.h>

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>
#include <string_view>

#include "base/compiler_specific.h"
#include "base/debug/debugger.h"
#include "net/base/load_flags_to_string.h"
#include "net/base/net_diagnostics_debug_formatting_ffi.h"
#include "net/base/net_diagnostics_load_flags_reference.h"
#include "net/base/net_diagnostics_privacy_mode_reference.h"
#include "net/base/privacy_mode.h"

namespace {
constexpr uint32_t kDefinedMask = (1u << 20) - 1;
[[noreturn]] void Fail(const char* what, int value) {
  std::fprintf(stderr, "FAIL %s value=%d\\n", what, value);
  std::abort();
}
void CheckValidFlags(int flags) {
  const std::string expected =
      net::net_diagnostics_load_flags_reference::LoadFlagsToString(flags);
  const std::string actual = net::LoadFlagsToString(flags);
  if (actual != expected) {
    Fail("load flags differential", flags);
  }
}
void CheckDeath(const char* self, const char* mode) {
  const intptr_t result = _spawnl(_P_WAIT, self, self, mode, nullptr);
  if (result == 0 || result == -1) {
    Fail("expected child CHECK/NOTREACHED", static_cast<int>(result));
  }
}
int Child(const char* mode) {
  SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX |
               SEM_NOOPENFILEERRORBOX);
  base::debug::SetSuppressDebugUI(true);
  if (!std::strcmp(mode, "--invalid-load-negative")) {
    (void)net::LoadFlagsToString(-1);
  } else if (!std::strcmp(mode, "--invalid-load-unknown")) {
    (void)net::LoadFlagsToString(1 << 30);
  } else if (!std::strcmp(mode, "--invalid-load-negative-high")) {
    (void)net::LoadFlagsToString(std::numeric_limits<int>::min());
  } else if (!std::strcmp(mode, "--invalid-privacy-negative")) {
    (void)net::PrivacyModeToDebugString(static_cast<net::PrivacyMode>(-1));
  } else if (!std::strcmp(mode, "--invalid-privacy-high")) {
    (void)net::PrivacyModeToDebugString(static_cast<net::PrivacyMode>(4));
  } else {
    return -1;
  }
  return 0;  // Parent expects Chromium CHECK/NOTREACHED to terminate first.
}
void CheckRustFfiOwnership() {
  constexpr int kIterations = 100000;
  for (int i = 0; i < kIterations; ++i) {
    const int flags = static_cast<int>(
        (static_cast<uint32_t>(i) * 2654435761u) & kDefinedMask);
    size_t length = std::numeric_limits<size_t>::max();
    uint8_t* bytes = net_diagnostics_load_flags_format(flags, &length);
    if (!bytes || length == 0) {
      Fail("valid Rust FFI allocation", flags);
    }
    // SAFETY: The FFI contract ties `bytes` and `length` to this owned live
    // Rust allocation; the view is consumed before the matching free.
    const std::string formatted = UNSAFE_BUFFERS(
        std::string(reinterpret_cast<const char*>(bytes), length));
    const std::string expected =
        net::net_diagnostics_load_flags_reference::LoadFlagsToString(flags);
    if (formatted != expected) {
      Fail("Rust FFI bytes", flags);
    }
    net_diagnostics_load_flags_free(bytes, length);
  }
  size_t unchanged = 0xfeed;
  if (net_diagnostics_load_flags_format(1 << 30, &unchanged) != nullptr ||
      unchanged != 0xfeed) {
    Fail("invalid FFI leaves length untouched", 1 << 30);
  }
  if (net_diagnostics_load_flags_format(0, nullptr) != nullptr) {
    Fail("null out length rejected", 0);
  }
}
void CheckPrivacyModes() {
  constexpr std::array<const char*, 4> kExpected = {
      "disabled", "enabled", "enabled without client certs",
      "enabled partitioned state allowed"};
  for (int mode = 0; mode < static_cast<int>(kExpected.size()); ++mode) {
    const auto privacy = static_cast<net::PrivacyMode>(mode);
    const char* expected =
        net::net_diagnostics_privacy_mode_reference::PrivacyModeToDebugString(
            mode);
    const char* actual = net::PrivacyModeToDebugString(privacy);
    if (std::strcmp(expected, kExpected[mode]) ||
        std::strcmp(actual, expected)) {
      Fail("privacy public API", mode);
    }
    const char* raw_first = net_diagnostics_privacy_mode(mode);
    const char* raw_second = net_diagnostics_privacy_mode(mode);
    if (!raw_first || raw_first != raw_second ||
        std::strcmp(raw_first, expected)) {
      Fail("privacy static FFI lifetime", mode);
    }
  }
  if (net_diagnostics_privacy_mode(-1) || net_diagnostics_privacy_mode(4)) {
    Fail("invalid privacy FFI returns null", -1);
  }
}
}  // namespace

int main(int argc, char** argv) {
  if (argc == 2) {
    return Child(argv[1]);
  }
  CheckValidFlags(0);
  for (uint32_t flags = 1; flags <= kDefinedMask; ++flags) {
    CheckValidFlags(static_cast<int>(flags));
  }
  uint32_t x = 0x4d595df4u;
  for (int i = 0; i < 1000000; ++i) {
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    const int flags = static_cast<int>(x & kDefinedMask);
    CheckValidFlags(flags);
  }
  CheckPrivacyModes();
  CheckRustFfiOwnership();
  for (const char* mode :
       {"--invalid-load-negative", "--invalid-load-unknown",
        "--invalid-load-negative-high", "--invalid-privacy-negative",
        "--invalid-privacy-high"}) {
    CheckDeath(argv[0], mode);
  }
  std::puts(
      "PASS: exhaustive 2^20 and 1,000,000 bounded defined C++ public-API "
      "differentials; four privacy modes; 100,000 direct Rust FFI "
      "allocations/frees; five isolated invalid-input fatal checks");
}
