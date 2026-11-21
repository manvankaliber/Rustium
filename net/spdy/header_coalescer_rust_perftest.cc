// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <array>
#include <chrono>
#include <cstdio>
#include <random>

#include "net/log/test_net_log.h"
#include "net/spdy/header_coalescer_rust_test_support.h"

namespace net::header_coalescer_rust_test {
namespace {
struct Fixture {
  const char* name;
  uint32_t limit = UINT32_MAX;
  Headers input;
  Headers expected;
  const char* error = nullptr;
};
std::vector<Fixture> Fixtures() {
  std::vector<Fixture> fixtures;
  fixtures.push_back({"request",
                      UINT32_MAX,
                      {{":method", "GET"},
                       {":scheme", "https"},
                       {":authority", "example.test"},
                       {":path", "/resource"},
                       {"accept", "*/*"}},
                      {{":method", "GET"},
                       {":scheme", "https"},
                       {":authority", "example.test"},
                       {":path", "/resource"},
                       {"accept", "*/*"}}});
  for (const bool cookie : {false, true}) {
    Fixture f{cookie ? "cookies" : "duplicates"};
    std::string joined;
    for (unsigned i = 0; i < 32; ++i) {
      const auto value = std::to_string(i);
      f.input.emplace_back(cookie ? "cookie" : "x-value", value);
      if (i != 0) {
        joined += cookie ? "; " : std::string(1, '\0');
      }
      joined += value;
    }
    f.expected.emplace_back(cookie ? "cookie" : "x-value", joined);
    fixtures.push_back(std::move(f));
  }
  const std::string large(32768, 'v');
  fixtures.push_back(
      {"large_value", UINT32_MAX, {{"a", large}}, {{"a", large}}});
  std::string obs;
  for (unsigned c = 128; c <= 255; ++c) {
    obs += static_cast<char>(c);
  }
  fixtures.push_back({"obs_text", UINT32_MAX, {{"a", obs}}, {{"a", obs}}});
  fixtures.push_back({"invalid_name",
                      UINT32_MAX,
                      {{"A?", "\n"}},
                      {},
                      "Invalid character in header name."});
  fixtures.push_back({"uppercase",
                      UINT32_MAX,
                      {{"A", "ok"}},
                      {},
                      "Upper case characters in header name."});
  fixtures.push_back({"ordering",
                      UINT32_MAX,
                      {{"a", "ok"}, {":late", "x"}},
                      {{"a", "ok"}},
                      "Pseudo header must not follow regular headers."});
  fixtures.push_back({"size_limit",
                      34,
                      {{"a", "x"}, {"b", "\n"}},
                      {{"a", "x"}},
                      "Header list too large."});
  fixtures.push_back({"invalid_value",
                      UINT32_MAX,
                      {{"cookie", "secret\r\n"}},
                      {},
                      "Invalid character 0x0D in header value."});
  return fixtures;
}
uint64_t Mix(uint64_t sum, uint64_t n) {
  return (sum ^ n) * 0x100000001b3ull;
}
uint64_t Text(uint64_t sum, std::string_view text) {
  sum = Mix(sum, text.size());
  for (unsigned char c : text) {
    sum = Mix(sum, c);
  }
  return sum;
}
volatile uint64_t sink;

// The identical fixtures are used by both implementations. Construction,
// destruction, in-memory NetLog ownership/recording, all public calls, FFI,
// native block ownership/append/release, and output checksum are inside Run.
// Verification adds an independent literal-output oracle before any clocks.
template <class Coalescer>
uint64_t Run(const Fixture& f, size_t iterations, bool verify) {
  uint64_t sum = 0xcbf29ce484222325ull;
  for (size_t i = 0; i < iterations; ++i) {
    NetLog* log = NetLog::Get();
    const auto source = NetLogWithSource::Make(log, NetLogSourceType::NONE);
    RecordingNetLogObserver observer(log, NetLogCaptureMode::kIncludeSensitive);
    Coalescer coalescer(f.limit, source);
    coalescer.OnHeaderBlockStart();
    for (const auto& [key, value] : f.input) {
      coalescer.OnHeader(key, value);
    }
    coalescer.OnHeaderBlockEnd(i, i * 2);
    sum = Mix(sum, coalescer.error_seen());
    if (verify) {
      CHECK(coalescer.error_seen() == (f.error != nullptr));
    }
    if (f.error) {
      coalescer.OnHeader("ignored", "after failure");
    }
    const auto result = coalescer.release_headers();
    if (verify) {
      CHECK(Copy(result) == f.expected);
    }
    sum = Mix(sum, result.size());
    for (const auto& [key, value] : result) {
      sum = Text(Text(sum, key), value);
    }
    const auto events = observer.GetEntries();
    if (verify) {
      CHECK(events.size() == (f.error ? 1u : 0u));
    }
    sum = Mix(sum, events.size());
    for (const auto& event : events) {
      CHECK(event.source.id == source.source().id);
      const auto* error = event.params.FindString("error");
      const auto* key = event.params.FindString("header_name");
      const auto* value = event.params.FindString("header_value");
      CHECK(error && key && value);
      if (verify) {
        CHECK(*error == f.error);
      }
      sum = Text(Text(Text(sum, *error), *key), *value);
    }
  }
  return sum;
}
template <class Coalescer>
double Measure(const Fixture& f, size_t iterations, uint64_t expected) {
  const auto start = std::chrono::steady_clock::now();
  const auto result = Run<Coalescer>(f, iterations, false);
  const auto stop = std::chrono::steady_clock::now();
  sink = result;
  CHECK(result == expected);
  return std::chrono::duration<double, std::nano>(stop - start).count() /
         iterations;
}
}  // namespace
}  // namespace net::header_coalescer_rust_test

int main(int argc, char** argv) {
  using namespace net;
  using namespace net::header_coalescer_rust_test;
  const bool verify =
      argc == 2 && UNSAFE_BUFFERS(std::string_view(argv[1])) == "--verify-only";
  const bool timing =
      argc == 2 && UNSAFE_BUFFERS(std::string_view(argv[1])) == "--timing";
  if (!verify && !timing) {
    UNSAFE_BUFFERS(std::fputs("Specify --verify-only or --timing\n", stderr));
    return 2;
  }
  const auto fixtures = Fixtures();  // Excluded fixture construction.
  std::mt19937 order(20261113);
  for (const auto& f : fixtures) {
    const auto expected = Run<HeaderCoalescerRustOriginal>(f, 32, true);
    CHECK(expected == Run<HeaderCoalescer>(f, 32, true));
    if (verify) {
      UNSAFE_BUFFERS(
          std::printf("{\"case\":\"%s\",\"verified\":true,\"iterations\":32,"
                      "\"checksum\":\"%016llx\"}\n",
                      f.name, static_cast<unsigned long long>(expected)));
      continue;  // No clock is read on this path.
    }
    constexpr size_t iterations = 128;
    const auto checksum = Run<HeaderCoalescerRustOriginal>(f, iterations, true);
    CHECK(checksum == Run<HeaderCoalescer>(f, iterations, true));
    for (unsigned repeat = 0; repeat < 20; ++repeat) {
      double baseline_ns, rust_ns;
      const bool rust_first = (order() & 1) != 0;
      if (rust_first) {
        rust_ns = Measure<HeaderCoalescer>(f, iterations, checksum);
        baseline_ns =
            Measure<HeaderCoalescerRustOriginal>(f, iterations, checksum);
      } else {
        baseline_ns =
            Measure<HeaderCoalescerRustOriginal>(f, iterations, checksum);
        rust_ns = Measure<HeaderCoalescer>(f, iterations, checksum);
      }
      UNSAFE_BUFFERS(std::printf(
          "{\"case\":\"%s\",\"repeat\":%u,\"baseline_ns\":%.9f,\"rust_ns\":%."
          "9f,\"iterations\":%zu,\"rust_first\":%s,\"checksum\":\"%016llx\"}\n",
          f.name, repeat, baseline_ns, rust_ns, iterations,
          rust_first ? "true" : "false",
          static_cast<unsigned long long>(checksum)));
    }
  }
}
