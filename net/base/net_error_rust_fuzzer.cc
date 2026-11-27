// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "build/build_config.h"

#if BUILDFLAG(IS_WIN)
#include <winsock2.h>
#elif BUILDFLAG(IS_POSIX)
#include <errno.h>
#endif

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string>
#include <vector>

#include "base/check.h"
#include "base/check_op.h"
#include "base/compiler_specific.h"
#include "base/containers/span.h"
#include "base/logging.h"
#include "net/base/net_error_rust_reference.h"
#include "net/base/net_errors.h"

namespace {

namespace ref = net_error_rust_reference;

// Use the frozen list for valid formatting operands. Production's public
// defined-error predicate and actual strings are still compared independently.
constexpr int kErrors[] = {
    0,
#define NET_ERROR(label, value) value,
#include "net/base/net_error_rust_reference_list.h"
#undef NET_ERROR
};

constexpr logging::SystemErrorCode kSystemErrors[] = {
    0,
#define NET_ERROR_RUST_CONSTANT(name) name,
#if BUILDFLAG(IS_WIN)
#include "net/base/net_error_rust_win_constants.inc"
#elif BUILDFLAG(IS_POSIX)
#include "net/base/net_error_rust_posix_constants.inc"
#endif
#undef NET_ERROR_RUST_CONSTANT
};

uint8_t Byte(base::span<const uint8_t>& input) {
  return input.empty() ? 0 : input.take_first<1>()[0];
}

uint32_t Word(base::span<const uint8_t>& input) {
  uint32_t value = 0;
  for (unsigned shift = 0; shift < 32; shift += 8) {
    value |= static_cast<uint32_t>(Byte(input)) << shift;
  }
  return value;
}

struct Diagnostic {
  int severity;
  std::string message;
  bool operator==(const Diagnostic&) const = default;
};

thread_local std::vector<Diagnostic>* diagnostics = nullptr;

bool Capture(int severity,
             const char*,
             int,
             size_t message_start,
             const std::string& text) {
  // Do not intercept fatal diagnostics or logs from another thread. Expected
  // fatal contracts are excluded, never converted into successful fuzz inputs.
  if (!diagnostics || severity >= logging::LOGGING_FATAL) {
    return false;
  }
  diagnostics->push_back({severity, text.substr(message_start)});
  return true;
}

template <class Function>
int MapWithDiagnostics(Function function,
                       logging::SystemErrorCode error,
                       std::vector<Diagnostic>& trace) {
  CHECK(!diagnostics);
  const auto previous = logging::GetLogMessageHandler();
  diagnostics = &trace;
  logging::SetLogMessageHandler(Capture);
  const int mapped = static_cast<int>(function(error));
  logging::SetLogMessageHandler(previous);
  diagnostics = nullptr;
  return mapped;
}

void Predicates(int error) {
  CHECK_EQ(net::IsCertificateError(error), ref::IsCertificateError(error));
  CHECK_EQ(net::IsClientCertificateError(error),
           ref::IsClientCertificateError(error));
  CHECK_EQ(net::IsRequestBlockedError(error),
           ref::IsRequestBlockedError(error));
  CHECK_EQ(net::IsOkOrDefinedError(error), ref::IsOkOrDefinedError(error));
  // Original DCHECK forbids this operand; existing death tests cover it.
  if (error != ref::ERR_NAME_RESOLUTION_FAILED) {
    CHECK_EQ(net::IsHostnameResolutionError(error),
             ref::IsHostnameResolutionError(error));
  }
}

void Fuzz(base::span<const uint8_t> input) {
  // One complete API bundle even on empty input. Truncated operands zero-pad.
  for (size_t step = 0; step < 64; ++step) {
    const uint8_t mode = Byte(input);
    const uint32_t raw = Word(input);
    const int extended = std::bit_cast<int32_t>(Word(input));
    const uint32_t os_word = Word(input);
    // Exactly FILE_OK and the 16 valid negative enums; never the sentinel.
    const auto file = static_cast<base::File::Error>(-int{Byte(input) % 17});
    const size_t index_low = Byte(input);
    const size_t index = index_low + 256u * Byte(input);
    const int known = base::span(kErrors)[index % std::size(kErrors)];
    int error = std::bit_cast<int32_t>(raw);
    switch (mode % 4) {
      case 1:
        error = known;
        break;
      case 2:
        error = -static_cast<int>(raw % 2048);
        break;
      case 3:
        error = raw & 1 ? INT32_MIN : INT32_MAX;
        break;
    }
    Predicates(error);
    Predicates(known);
    CHECK_EQ(net::ErrorToString(known), ref::ErrorToString(known));
    CHECK_EQ(net::ErrorToShortString(known), ref::ErrorToShortString(known));
    const int format_error = mode & 4 ? ref::ERR_QUIC_PROTOCOL_ERROR : known;
    CHECK_EQ(net::ExtendedErrorToString(format_error, extended),
             ref::ExtendedErrorToString(format_error, extended));
    CHECK_EQ(static_cast<int>(net::FileErrorToNetError(file)),
             static_cast<int>(ref::FileErrorToNetError(file)));
#if BUILDFLAG(IS_WIN)
    const auto raw_system = static_cast<logging::SystemErrorCode>(os_word);
#else
    const auto raw_system =
        static_cast<logging::SystemErrorCode>(std::bit_cast<int32_t>(os_word));
#endif
    const auto system =
        mode & 8 ? base::span(kSystemErrors)[index % std::size(kSystemErrors)]
                 : raw_system;
    std::vector<Diagnostic> actual_diagnostics;
    std::vector<Diagnostic> expected_diagnostics;
    const int actual =
        MapWithDiagnostics(net::MapSystemError, system, actual_diagnostics);
    const int expected =
        MapWithDiagnostics(ref::MapSystemError, system, expected_diagnostics);
    CHECK_EQ(actual, expected);
    CHECK(actual_diagnostics == expected_diagnostics);
    if (input.empty()) {
      break;
    }
  }
}

}  // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  // SAFETY: libFuzzer supplies size initialized readable bytes for this call.
  // The boundary is the only raw span construction; all operand reads are
  // bound.
  const auto input = size == 0 ? base::span<const uint8_t>()
                               : UNSAFE_BUFFERS(base::span(data, size));
  Fuzz(input.first(std::min(input.size(), size_t{1024})));
  return 0;
}
