// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/base/net_errors.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <string>

#include "base/check_op.h"
#include "base/notreached.h"
#include "net/base/net_error_rust_ffi.h"
#include "net/third_party/quiche/src/quiche/quic/core/quic_error_codes.h"
namespace net {
#define NET_ERROR(label, value) \
  static_assert(value < 0, "ERR_" #label " should be negative");
#include "net/base/net_error_list.h"
#undef NET_ERROR
#include "net/base/net_error_rust_enum_checks.h"
namespace {
std::string Format(int error, bool full, const char* quic_name = nullptr) {
  if (!quic_name && !net_error_rust_defined(error)) {
    // Preserve Chromium's build-dependent dump severity and offending value.
    DUMP_WILL_BE_NOTREACHED() << error;
  }
  const auto* suffix = reinterpret_cast<const uint8_t*>(quic_name);
  const size_t suffix_length = quic_name ? std::strlen(quic_name) : 0;
  // Rust initializes only the emitted bytes of this native scratch storage.
  // A long future QUICHE name uses the same policy with native owned storage.
  std::array<uint8_t, 256> buffer;
  const size_t length = net_error_rust_format(
      error, full, suffix, suffix_length, buffer.data(), buffer.size());
  if (length <= buffer.size()) {
    return std::string(reinterpret_cast<const char*>(buffer.data()), length);
  }
  std::string result(length, '\0');
  net_error_rust_format(error, full, suffix, suffix_length,
                        reinterpret_cast<uint8_t*>(result.data()),
                        result.size());
  return result;
}
}  // namespace
std::string ErrorToString(int error) {
  return Format(error, true);
}
std::string ErrorToShortString(int error) {
  return Format(error, false);
}
std::string ExtendedErrorToString(int error, int extended_error_code) {
  const char* name = nullptr;
  if (net_error_rust_extended(error, extended_error_code)) {
    name = quic::QuicErrorCodeToString(
        static_cast<quic::QuicErrorCode>(extended_error_code));
  }
  return Format(error, true, name);
}
bool IsCertificateError(int error) {
  return net_error_rust_certificate(error);
}
bool IsClientCertificateError(int error) {
  return net_error_rust_client_certificate(error);
}
bool IsHostnameResolutionError(int error) {
  const int8_t result = net_error_rust_hostname(error);
  if (result < 0) {
    DCHECK_NE(ERR_NAME_RESOLUTION_FAILED, error);
  }
  return result > 0;
}
bool IsRequestBlockedError(int error) {
  return net_error_rust_blocked(error);
}
bool IsOkOrDefinedError(int error) {
  return net_error_rust_defined(error);
}
Error FileErrorToNetError(base::File::Error file_error) {
  const int32_t result = net_error_rust_file(file_error);
  if (result == net_error_rust::kUnknown) {
    NOTREACHED();
  }
  return static_cast<Error>(result);
}
}  // namespace net
