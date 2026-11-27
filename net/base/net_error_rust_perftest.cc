// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <random>
#include <ratio>
#include <string_view>

#include "base/check.h"
#include "base/compiler_specific.h"
#include "base/logging.h"
#include "build/build_config.h"
#include "net/base/net_error_rust_reference.h"
#include "net/base/net_errors.h"
#if BUILDFLAG(IS_WIN)
#include <winsock2.h>
#else
#include <errno.h>
#endif
namespace {
namespace ref = net_error_rust_reference;
constexpr std::array<const char*, 10> kCases = {
    "short_names",      "full_names",     "extended_quic", "extended_zero",
    "extended_nonquic", "classification", "file_all",      "system_hits",
    "system_misses",    "system_zero"};
constexpr int kErrors[] = {
    0,
#define NET_ERROR(label, value) value,
#include "net/base/net_error_list.h"
#undef NET_ERROR
};
#if BUILDFLAG(IS_WIN)
constexpr logging::SystemErrorCode kSystem[] = {
    0,
#define NET_ERROR_RUST_CONSTANT(name) name,
#include "net/base/net_error_rust_win_constants.inc"
#undef NET_ERROR_RUST_CONSTANT
};
#else
constexpr logging::SystemErrorCode kSystem[] = {
    0,
#define NET_ERROR_RUST_CONSTANT(name) name,
#include "net/base/net_error_rust_posix_constants.inc"
#undef NET_ERROR_RUST_CONSTANT
};
#endif
// Distinct per-implementation fixtures: no output computed by one is fed to
// the other. Inputs and API ABI are identical and own no cross-DLL objects.
struct Fixture {
  std::array<int, 256> error;
  std::array<int, 256> extended;
  std::array<base::File::Error, 256> file;
  std::array<logging::SystemErrorCode, 256> system;
  Fixture() {
    for (size_t i = 0; i < error.size(); ++i) {
      error[i] = UNSAFE_BUFFERS(kErrors[(i * 73) % std::size(kErrors)]);
      extended[i] = static_cast<int>(i * 13) - 1;
      file[i] = static_cast<base::File::Error>(-static_cast<int>(i % 17));
      system[i] = UNSAFE_BUFFERS(kSystem[(i * 7) % std::size(kSystem)]);
    }
  }
};
uint64_t Hash(std::string_view text) {
  uint64_t sum = 0;
  for (unsigned char c : text) {
    sum = sum * 131 + c;
  }
  return sum;
}
volatile uint64_t sink;
template <bool rust>
uint64_t Run(size_t kind, size_t count, const Fixture& f) {
  uint64_t result = 0;
  for (size_t i = 0; i < count; ++i) {
    const size_t j = i % 256;
    const int e = f.error[j];
    switch (kind) {
      case 0:
        result += Hash(rust ? net::ErrorToShortString(e)
                            : ref::ErrorToShortString(e));
        break;
      case 1:
        result += Hash(rust ? net::ErrorToString(e) : ref::ErrorToString(e));
        break;
      case 2:
        result += Hash(rust ? net::ExtendedErrorToString(
                                  net::ERR_QUIC_PROTOCOL_ERROR, f.extended[j])
                            : ref::ExtendedErrorToString(
                                  ref::ERR_QUIC_PROTOCOL_ERROR, f.extended[j]));
        break;
      case 3:
        result += Hash(rust ? net::ExtendedErrorToString(e, 0)
                            : ref::ExtendedErrorToString(e, 0));
        break;
      case 4: {
        const int nonquic = e == net::ERR_QUIC_PROTOCOL_ERROR ? 0 : e;
        result +=
            Hash(rust ? net::ExtendedErrorToString(nonquic, f.extended[j])
                      : ref::ExtendedErrorToString(nonquic, f.extended[j]));
        break;
      }
      case 5: {
        const int host = e == net::ERR_NAME_RESOLUTION_FAILED ? 0 : e;
        if constexpr (rust) {
          result += net::IsCertificateError(e) +
                    2 * net::IsClientCertificateError(e) +
                    4 * net::IsHostnameResolutionError(host) +
                    8 * net::IsRequestBlockedError(e) +
                    16 * net::IsOkOrDefinedError(e);
        } else {
          result += ref::IsCertificateError(e) +
                    2 * ref::IsClientCertificateError(e) +
                    4 * ref::IsHostnameResolutionError(host) +
                    8 * ref::IsRequestBlockedError(e) +
                    16 * ref::IsOkOrDefinedError(e);
        }
        break;
      }
      case 6:
        result += static_cast<int>(
            rust ? static_cast<int>(net::FileErrorToNetError(f.file[j]))
                 : static_cast<int>(ref::FileErrorToNetError(f.file[j])));
        break;
      case 7:
        result += rust ? static_cast<int>(net::MapSystemError(f.system[j]))
                       : static_cast<int>(ref::MapSystemError(f.system[j]));
        break;
      case 8: {
        const auto unknown =
            static_cast<logging::SystemErrorCode>(0x7fff0000u + j);
        result += rust ? static_cast<int>(net::MapSystemError(unknown))
                       : static_cast<int>(ref::MapSystemError(unknown));
        break;
      }
      case 9:
        result += rust ? static_cast<int>(net::MapSystemError(0))
                       : static_cast<int>(ref::MapSystemError(0));
        break;
    }
  }
  return result;
}
bool Quiet(int, const char*, int, size_t, const std::string&) {
  return true;
}
// Independent expected semantics are checked for each implementation before
// clocks.
template <bool rust>
void VerifySemantics() {
  CHECK((rust ? net::ErrorToString(0) : ref::ErrorToString(0)) == "net::OK");
  CHECK((rust ? net::ErrorToShortString(-2) : ref::ErrorToShortString(-2)) ==
        "ERR_FAILED");
  CHECK((rust ? net::ExtendedErrorToString(-356, 1)
              : ref::ExtendedErrorToString(-356, 1)) ==
        "net::ERR_QUIC_PROTOCOL_ERROR.QUIC_INTERNAL_ERROR");
  CHECK((rust ? net::ExtendedErrorToString(-356, 0)
              : ref::ExtendedErrorToString(-356, 0)) ==
        "net::ERR_QUIC_PROTOCOL_ERROR");
  CHECK((rust ? net::ExtendedErrorToString(-2, 1)
              : ref::ExtendedErrorToString(-2, 1)) == "net::ERR_FAILED");
  CHECK(rust ? net::IsCertificateError(-200) : ref::IsCertificateError(-200));
  CHECK(
      !(rust ? net::IsCertificateError(-220) : ref::IsCertificateError(-220)));
  CHECK(rust
            ? net::IsClientCertificateError(net::ERR_BAD_SSL_CLIENT_AUTH_CERT)
            : ref::IsClientCertificateError(ref::ERR_BAD_SSL_CLIENT_AUTH_CERT));
  CHECK(rust ? net::IsHostnameResolutionError(net::ERR_NAME_NOT_RESOLVED)
             : ref::IsHostnameResolutionError(ref::ERR_NAME_NOT_RESOLVED));
  CHECK(rust ? net::IsRequestBlockedError(net::ERR_BLOCKED_BY_CLIENT)
             : ref::IsRequestBlockedError(ref::ERR_BLOCKED_BY_CLIENT));
  CHECK(
      !(rust ? net::IsOkOrDefinedError(-813) : ref::IsOkOrDefinedError(-813)));
  CHECK((rust ? static_cast<int>(
                    net::FileErrorToNetError(base::File::FILE_ERROR_NOT_FOUND))
              : static_cast<int>(ref::FileErrorToNetError(
                    base::File::FILE_ERROR_NOT_FOUND))) == -6);
  CHECK((rust ? static_cast<int>(net::MapSystemError(0))
              : static_cast<int>(ref::MapSystemError(0))) == 0);
  CHECK((rust
             ? static_cast<int>(net::MapSystemError(
                   static_cast<logging::SystemErrorCode>(0x7fff0000u)))
             : static_cast<int>(ref::MapSystemError(
                   static_cast<logging::SystemErrorCode>(0x7fff0000u)))) == -2);
}
template <bool rust>
double Measure(size_t kind, size_t n, const Fixture& f) {
  const auto start = std::chrono::steady_clock::now();
  const uint64_t value = Run<rust>(kind, n, f);
  const auto end = std::chrono::steady_clock::now();
  sink = value;
  return std::chrono::duration<double, std::nano>(end - start).count() / n;
}
}  // namespace
int main(int argc, char** argv) {
  const bool verify =
      argc == 2 && UNSAFE_BUFFERS(std::string_view(argv[1])) == "--verify-only";
  const bool timing =
      argc == 2 && UNSAFE_BUFFERS(std::string_view(argv[1])) == "--timing";
  if (!verify && !timing) {
    return 2;
  }
  logging::SetLogMessageHandler(Quiet);
  VerifySemantics<false>();
  VerifySemantics<true>();
  const Fixture baseline_fixture, rust_fixture;
  constexpr size_t iterations = 16384;
  std::mt19937 rng(20261113);
  for (size_t kind = 0; kind < kCases.size(); ++kind) {
    const uint64_t expected = Run<false>(kind, iterations, baseline_fixture);
    CHECK(expected == Run<true>(kind, iterations, rust_fixture));
    if (verify) {
      UNSAFE_BUFFERS(std::printf(
          "{\"case\":\"%s\",\"verified\":true,\"iterations\":%zu,\"checksum\":"
          "\"%016llx\"}\n",
          kCases[kind], iterations, static_cast<unsigned long long>(expected)));
      continue;  // Never enters Measure or reads a timing clock.
    }
    for (unsigned repeat = 0; repeat < 20; ++repeat) {
      const bool rust_first = (rng() & 1) != 0;
      double baseline_ns, rust_ns;
      if (rust_first) {
        rust_ns = Measure<true>(kind, iterations, rust_fixture);
        CHECK(sink == expected);
        baseline_ns = Measure<false>(kind, iterations, baseline_fixture);
      } else {
        baseline_ns = Measure<false>(kind, iterations, baseline_fixture);
        CHECK(sink == expected);
        rust_ns = Measure<true>(kind, iterations, rust_fixture);
      }
      CHECK(sink == expected);
      UNSAFE_BUFFERS(std::printf(
          "{\"case\":\"%s\",\"repeat\":%u,\"baseline_ns\":%.9f,\"rust_ns\":%."
          "9f,\"iterations\":%zu,\"rust_first\":%s,\"checksum\":\"%016llx\"}\n",
          kCases[kind], repeat, baseline_ns, rust_ns, iterations,
          rust_first ? "true" : "false",
          static_cast<unsigned long long>(expected)));
    }
  }
}
