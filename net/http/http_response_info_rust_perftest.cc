// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Parent-only timing in a quiet window; --validate-only does no timing.
#include <chrono>
#include <random>
#include <ratio>

#include "net/http/http_response_info_rust_test_support.h"
namespace {
struct Case {
  const char* key;
  int operation;
  uint64_t seed;
  size_t iterations;
};
volatile uint64_t sink = 0;
template <class T>
struct Fixture {
  T value;
  std::string pickle;
  explicit Fixture(const Case& c) {
    Fill(value, c.seed);
    value.ssl_info.connection_status = (6 << 20) | 0x1301;
    if (c.operation == 9) {
      pickle = Bytes(Legacy(0xefffff03u, 0, 123));
    } else {
      pickle = Bytes(*value.MakePickle(false, false));
    }
    if (c.operation == 10) {
      pickle.resize(pickle.size() / 2);
      uint32_t n = static_cast<uint32_t>(pickle.size() - 4);
      base::as_writable_byte_span(pickle).first<4>().copy_from(
          base::byte_span_from_ref(n));
    }
  }
};
template <class T>
NOINLINE uint64_t Run(const Case& c, const Fixture<T>& f, size_t count) {
  uint64_t result = 0;
  for (size_t i = 0; i < count; ++i) {
    switch (c.operation) {
      case 0: {
        T v;
        result += v.was_cached + static_cast<int>(v.alternate_protocol_usage);
        break;
      }
      case 1: {
        T v(f.value);
        result += v.dns_aliases.size() + v.ssl_info.cert_status;
        break;
      }
      case 2: {
        T v;
        v = f.value;
        result += v.dns_aliases.size() + v.ssl_info.cert_status;
        break;
      }
      case 3: {
        result += f.value.DidUseQuic() + f.value.WasFetchedViaProxy();
        break;
      }
      case 4:
      case 5:
      case 6: {
        auto p = f.value.MakePickle(c.operation == 6, true);
        result += p->size();
        break;
      }
      case 7: {
        auto p = f.value.MakePickleWithSignedBodySizeForTesting(true, true, -1);
        result += p->size();
        break;
      }
      case 8:
      case 9:
      case 10: {
        T v;
        bool truncated = false;
        result += v.InitFromPickle(
            base::PickleIterator::WithData(base::as_byte_span(f.pickle)),
            &truncated);
        result += v.was_cached + v.dns_aliases.size() + truncated;
        break;
      }
      default:
        NOTREACHED();
    }
  }
  return result;
}
template <class T>
double Measure(const Case& c, const Fixture<T>& f) {
  auto start = std::chrono::steady_clock::now();
  sink = Run(c, f, c.iterations);
  return std::chrono::duration<double, std::nano>(
             std::chrono::steady_clock::now() - start)
             .count() /
         c.iterations;
}
}  // namespace
int main(int argc, char** argv) {
  bool validate = argc > 1 && std::string_view(argv[1]) == "--validate-only";
  auto cert_owner = LoadCertificate();
  certificate = cert_owner.get();
  constexpr Case cases[] = {
      {"response_init", 0, 0, 100000},
      {"response_copy_plain", 1, 0, 50000},
      {"response_copy_metadata", 1, 0x17ffff, 20000},
      {"response_assign_metadata", 2, 0x17ffff, 20000},
      {"response_transport", 3, 0x17ffff, 100000},
      {"response_pickle_plain", 4, 0, 20000},
      {"response_pickle_tls", 5, 0x17ffff, 5000},
      {"response_pickle_filtered", 6, 0x17ffff, 5000},
      {"response_pickle_signed_legacy", 7, 0x17ffff, 5000},
      {"response_restore_plain", 8, 0, 20000},
      {"response_restore_tls", 8, 0x17ffff, 5000},
      {"response_restore_md5", 8, 0x7ffff, 5000},
      {"response_restore_legacy", 9, 0, 5000},
      {"response_restore_partial", 10, 0x17ffff, 10000}};
  std::mt19937 random(20261113);
  for (const auto& c : cases) {
    Fixture<HttpResponseInfoOriginal> cpp(c);
    Fixture<HttpResponseInfo> rust(c);
    CHECK(cpp.pickle == rust.pickle);
    CHECK(Run(c, cpp, 64) == Run(c, rust, 64));
    if (validate) {
      continue;
    }
    for (int repeat = 0; repeat < 20; ++repeat) {
      bool rust_first = random() & 1;
      double cpp_ns, rust_ns;
      if (rust_first) {
        rust_ns = Measure(c, rust);
        cpp_ns = Measure(c, cpp);
      } else {
        cpp_ns = Measure(c, cpp);
        rust_ns = Measure(c, rust);
      }
      // SAFETY: all format arguments point at live NUL-terminated case
      // literals.
      UNSAFE_BUFFERS(std::printf(
          "{\"case\":\"%s\",\"iterations\":%zu,\"repeat\":%d,\"rust_first\":%s,"
          "\"baseline_ns\":%.9f,\"rust_ns\":%.9f}\n",
          c.key, c.iterations, repeat, rust_first ? "true" : "false", cpp_ns,
          rust_ns));
      std::fflush(stdout);
    }
  }
  if (validate) {
    std::puts(
        "PASS: all 14 paired full-public-API workloads; no timings collected");
  }
}
