// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <array>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "base/logging.h"
#include "base/test/scoped_logging_settings.h"
#include "build/build_config.h"
#include "net/base/net_error_rust_reference.h"
#include "net/base/net_errors.h"
#include "testing/gtest/include/gtest/gtest.h"
#if BUILDFLAG(IS_WIN)
#include <winsock2.h>
#else
#include <errno.h>
#endif
namespace {
namespace ref = net_error_rust_reference;
std::string* diagnostic = nullptr;
bool Capture(int, const char*, int, size_t start, const std::string& text) {
  *diagnostic = text.substr(start);
  return true;
}
#if BUILDFLAG(IS_WIN)
struct SystemLogTrace {
  bool production = false;
  bool reentered = false;
  int nested_error = std::numeric_limits<int>::min();
  int nested_zero = std::numeric_limits<int>::min();
  std::vector<std::pair<int, std::string>> events;
};
SystemLogTrace* system_trace = nullptr;
bool CaptureSystemTrace(int severity,
                        const char*,
                        int,
                        size_t start,
                        const std::string& text) {
  auto& trace = *system_trace;
  trace.events.emplace_back(severity, text.substr(start));
  if (severity == logging::LOGGING_WARNING && !trace.reentered) {
    trace.reentered = true;
    // The native warning callback may reenter both known and zero mappings.
    // No Rust borrow or policy state can remain live across this handler.
    trace.nested_error = trace.production
                             ? static_cast<int>(net::MapSystemError(WSAEINVAL))
                             : static_cast<int>(ref::MapSystemError(WSAEINVAL));
    trace.nested_zero = trace.production
                            ? static_cast<int>(net::MapSystemError(0))
                            : static_cast<int>(ref::MapSystemError(0));
  }
  return true;
}
#endif

constexpr int kErrors[] = {
    0,
#define NET_ERROR(label, value) value,
#include "net/base/net_error_list.h"
#undef NET_ERROR
};
class NetErrorRustTest : public testing::Test {
 public:
  void SetUp() override {
    old_ = logging::GetLogMessageHandler();
    logging::SetLogMessageHandler(Quiet);
  }
  void TearDown() override { logging::SetLogMessageHandler(old_); }
  static bool Quiet(int severity,
                    const char*,
                    int,
                    size_t,
                    const std::string&) {
    return severity < logging::LOGGING_FATAL;
  }

 private:
  logging::LogMessageHandlerFunction old_ = nullptr;
};
void ComparePredicates(int e) {
  EXPECT_EQ(net::IsCertificateError(e), ref::IsCertificateError(e)) << e;
  EXPECT_EQ(net::IsClientCertificateError(e), ref::IsClientCertificateError(e))
      << e;
  EXPECT_EQ(net::IsRequestBlockedError(e), ref::IsRequestBlockedError(e)) << e;
  EXPECT_EQ(net::IsOkOrDefinedError(e), ref::IsOkOrDefinedError(e)) << e;
  if (e != net::ERR_NAME_RESOLUTION_FAILED) {
    EXPECT_EQ(net::IsHostnameResolutionError(e),
              ref::IsHostnameResolutionError(e))
        << e;
  }
}
TEST_F(NetErrorRustTest, EveryNameAndQuicCode) {
  for (int e : kErrors) {
    EXPECT_EQ(net::ErrorToString(e), ref::ErrorToString(e));
    EXPECT_EQ(net::ErrorToShortString(e), ref::ErrorToShortString(e));
    ComparePredicates(e);
    for (int q : {-1, 0, 1, 2, 500, std::numeric_limits<int>::min(),
                  std::numeric_limits<int>::max()}) {
      EXPECT_EQ(net::ExtendedErrorToString(e, q),
                ref::ExtendedErrorToString(e, q));
    }
  }
  // Includes every QUIC enum value and every hole, plus unsigned extremes
  // above.
  for (int q = -2; q < 1024; ++q) {
    EXPECT_EQ(net::ExtendedErrorToString(net::ERR_QUIC_PROTOCOL_ERROR, q),
              ref::ExtendedErrorToString(ref::ERR_QUIC_PROTOCOL_ERROR, q))
        << q;
  }
}
TEST_F(NetErrorRustTest, EnumAndIntegerBoundaries) {
  for (int e = -2048; e < 2048; ++e) {
    ComparePredicates(e);
  }
  for (int e :
       {std::numeric_limits<int>::min(), std::numeric_limits<int>::max()}) {
    ComparePredicates(e);
  }
  for (int e = -16; e <= 0; ++e) {
    auto f = static_cast<base::File::Error>(e);
    EXPECT_EQ(static_cast<int>(net::FileErrorToNetError(f)),
              static_cast<int>(ref::FileErrorToNetError(f)));
  }
}
TEST_F(NetErrorRustTest, PlatformConstantsAndUnsignedBoundaries) {
#if BUILDFLAG(IS_WIN)
  constexpr logging::SystemErrorCode constants[] = {0,
#define NET_ERROR_RUST_CONSTANT(name) name,
#include "net/base/net_error_rust_win_constants.inc"
#undef NET_ERROR_RUST_CONSTANT
                                                    0x7fffffffUL, 0x80000000UL,
                                                    0xffffffffUL};
#else
  constexpr logging::SystemErrorCode constants[] = {
      0,
#define NET_ERROR_RUST_CONSTANT(name) name,
#include "net/base/net_error_rust_posix_constants.inc"
#undef NET_ERROR_RUST_CONSTANT
      std::numeric_limits<int>::min(), -1, std::numeric_limits<int>::max()};
#endif
  for (auto e : constants) {
    EXPECT_EQ(static_cast<int>(net::MapSystemError(e)),
              static_cast<int>(ref::MapSystemError(e)))
        << e;
  }
  for (uint32_t e = 0; e < 65536; ++e) {
    EXPECT_EQ(static_cast<int>(net::MapSystemError(e)),
              static_cast<int>(ref::MapSystemError(e)))
        << e;
  }
}
TEST_F(NetErrorRustTest, MillionDifferentialPublicApiInputs) {
  uint64_t state = 0x438f9babe0123456ULL;
  for (size_t i = 0; i < 1048576; ++i) {
    state ^= state << 13;
    state ^= state >> 7;
    state ^= state << 17;
    const int e = static_cast<int>(static_cast<int32_t>(state));
    ComparePredicates(e);
    int known = kErrors[(state >> 32) % std::size(kErrors)];
    EXPECT_EQ(net::ErrorToShortString(known), ref::ErrorToShortString(known));
    EXPECT_EQ(net::ErrorToString(known), ref::ErrorToString(known));
    const int extended = static_cast<int32_t>(state >> 32);
    EXPECT_EQ(net::ExtendedErrorToString(
                  (i % 2) ? known : net::ERR_QUIC_PROTOCOL_ERROR, extended),
              ref::ExtendedErrorToString(
                  (i % 2) ? known : ref::ERR_QUIC_PROTOCOL_ERROR, extended));
    const auto file =
        static_cast<base::File::Error>(-static_cast<int>(state % 17));
    EXPECT_EQ(static_cast<int>(net::FileErrorToNetError(file)),
              static_cast<int>(ref::FileErrorToNetError(file)));
    const auto os = static_cast<logging::SystemErrorCode>(state);
    EXPECT_EQ(static_cast<int>(net::MapSystemError(os)),
              static_cast<int>(ref::MapSystemError(os)));
  }
}
// Each death runs in a fresh process. The original enum's representable range
// includes -18; extreme out-of-range enum casts would be UB and are excluded.
TEST_F(NetErrorRustTest, FatalDiagnosticSubprocesses) {
#if !defined(OFFICIAL_BUILD) || DCHECK_IS_ON()
  EXPECT_DEATH(
      {
        logging::SetLogMessageHandler(nullptr);
        net::ErrorToShortString(-813);
      },
      "NOTREACHED hit");
  EXPECT_DEATH(
      {
        logging::SetLogMessageHandler(nullptr);
        ref::ErrorToShortString(-813);
      },
      "NOTREACHED hit");
  EXPECT_DEATH(
      {
        logging::SetLogMessageHandler(nullptr);
        net::ErrorToString(1);
      },
      "NOTREACHED hit");
  EXPECT_DEATH(
      {
        logging::SetLogMessageHandler(nullptr);
        ref::ErrorToString(1);
      },
      "NOTREACHED hit");
  EXPECT_DEATH(
      {
        logging::SetLogMessageHandler(nullptr);
        net::ExtendedErrorToString(-813, 1);
      },
      "NOTREACHED hit");
  EXPECT_DEATH(
      {
        logging::SetLogMessageHandler(nullptr);
        ref::ExtendedErrorToString(-813, 1);
      },
      "NOTREACHED hit");
#endif
  EXPECT_DEATH(net::FileErrorToNetError(base::File::FILE_ERROR_MAX), "");
  EXPECT_DEATH(ref::FileErrorToNetError(base::File::FILE_ERROR_MAX), "");
  EXPECT_DEATH(net::FileErrorToNetError(static_cast<base::File::Error>(-18)),
               "");
  EXPECT_DEATH(ref::FileErrorToNetError(static_cast<base::File::Error>(-18)),
               "");
#if DCHECK_IS_ON()
  EXPECT_DEATH(net::IsHostnameResolutionError(net::ERR_NAME_RESOLUTION_FAILED),
               "");
  EXPECT_DEATH(ref::IsHostnameResolutionError(ref::ERR_NAME_RESOLUTION_FAILED),
               "");
#endif
}
TEST_F(NetErrorRustTest, ExactUnknownSystemDiagnostic) {
  std::string message;
  diagnostic = &message;
  logging::SetLogMessageHandler(Capture);
#if BUILDFLAG(IS_WIN)
  EXPECT_EQ(net::MapSystemError(0xffffffffUL), net::ERR_FAILED);
  EXPECT_EQ(message, "Unknown error 4294967295 mapped to net::ERR_FAILED\n");
  const std::string production = message;
  EXPECT_EQ(ref::MapSystemError(0xffffffffUL), ref::ERR_FAILED);
  EXPECT_EQ(message, production);
#endif
  logging::SetLogMessageHandler(Quiet);
  diagnostic = nullptr;
}
TEST_F(NetErrorRustTest, UnknownFormattingFallbackInOfficialBuild) {
#if defined(OFFICIAL_BUILD) && !DCHECK_IS_ON()
  for (int e : {-813, 1, std::numeric_limits<int>::min(),
                std::numeric_limits<int>::max()}) {
    EXPECT_EQ(net::ErrorToString(e), ref::ErrorToString(e));
    EXPECT_EQ(net::ErrorToShortString(e), "ERR_<unknown>");
    EXPECT_EQ(net::ExtendedErrorToString(e, 1), "net::ERR_<unknown>");
  }
#else
  GTEST_SKIP()
      << "Unknown native formatting is fatal in this build; subprocesses "
         "verify it. Safe Rust fallback bytes are tested separately.";
#endif
}
#if BUILDFLAG(IS_WIN)
TEST_F(NetErrorRustTest, SystemDiagnosticOrderSeverityAndReentry) {
  const int old_min_level = logging::GetMinLogLevel();
  logging::ScopedVmoduleSwitches verbose;
  verbose.InitWithSwitches("*=2");
  std::vector<std::pair<int, std::string>> original;
  for (bool production : {false, true}) {
    SystemLogTrace trace;
    trace.production = production;
    system_trace = &trace;
    logging::SetLogMessageHandler(CaptureSystemTrace);
    const int result =
        production ? static_cast<int>(net::MapSystemError(0xffffffffUL))
                   : static_cast<int>(ref::MapSystemError(0xffffffffUL));
    logging::SetLogMessageHandler(Quiet);
    system_trace = nullptr;
    EXPECT_EQ(result, net::ERR_FAILED);
    EXPECT_TRUE(trace.reentered);
    EXPECT_EQ(trace.nested_error, net::ERR_INVALID_ARGUMENT);
    EXPECT_EQ(trace.nested_zero, 0);
#if DCHECK_IS_ON()
    const std::vector<std::pair<int, std::string>> expected = {
        {-2, "Error 4294967295\n"},
        {logging::LOGGING_WARNING,
         "Unknown error 4294967295 mapped to net::ERR_FAILED\n"},
        {-2, "Error 10022\n"},
    };
#else
    const std::vector<std::pair<int, std::string>> expected = {
        {logging::LOGGING_WARNING,
         "Unknown error 4294967295 mapped to net::ERR_FAILED\n"},
    };
#endif
    EXPECT_EQ(trace.events, expected);
    if (production) {
      EXPECT_EQ(trace.events, original);
    } else {
      original = trace.events;
    }
  }
  logging::SetMinLogLevel(old_min_level);
}
#endif
}  // namespace
