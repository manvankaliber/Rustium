// Whole public API benchmark: setup/warmup excluded, all returned views and
// optionals consumed inside timed loops. No allocation is required by either
// API.
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <random>
#include <ratio>

#include "base/compiler_specific.h"
#include "net/http/http_status_code_reference.h"
namespace {
volatile uint64_t sink = 0;
struct Case {
  const char* key;
  int code;
  bool phrase;
};
double Measure(Case c, bool rust, int iterations) {
  uint64_t sum = 0;
  const auto start = std::chrono::steady_clock::now();
  for (int i = 0; i < iterations; ++i) {
    if (c.phrase) {
      auto v =
          rust ? net::GetHttpReasonPhrase(c.code, std::string_view("x\0z", 3))
               : net::http_status_code_reference::GetHttpReasonPhrase(
                     static_cast<net::HttpStatusCode>(c.code),
                     std::string_view("x\0z", 3));
      sum += v.size() + static_cast<unsigned char>(v[0]);
    } else {
      auto v =
          rust
              ? net::TryToGetHttpStatusCode(c.code)
              : net::http_status_code_reference::TryToGetHttpStatusCode(c.code);
      sum += v ? static_cast<int>(*v) : 1;
    }
  }
  sink = sum;
  return std::chrono::duration<double, std::nano>(
             std::chrono::steady_clock::now() - start)
             .count() /
         iterations;
}
}  // namespace
int main() {
  constexpr Case cases[] = {{"status-phrase-ok", 200, true},
                            {"status-phrase-long", 511, true},
                            {"status-phrase-unknown", -1, true},
                            {"status-parse-known", 600, false},
                            {"status-parse-unknown", 599, false}};
  std::mt19937 rng(20261112);
  for (auto c : cases) {
    Measure(c, false, 4096);
    Measure(c, true, 4096);
    for (int repeat = 0; repeat < 20; ++repeat) {
      double baseline, rust;
      const bool rust_first = (rng() & 1) != 0;
      if (rust_first) {
        rust = Measure(c, true, 1000000);
        baseline = Measure(c, false, 1000000);
      } else {
        baseline = Measure(c, false, 1000000);
        rust = Measure(c, true, 1000000);
      }
      // SAFETY: c.key is one of the NUL-terminated literals in cases above.
      UNSAFE_BUFFERS(printf(
          "{\"case\":\"%s\",\"repeat\":%d,\"baseline_ns\":%.9f,\"rust_ns\":%."
          "9f,\"rust_first\":%s}\n",
          c.key, repeat, baseline, rust, rust_first ? "true" : "false"));
    }
  }
}
