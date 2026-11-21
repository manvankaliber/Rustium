// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Parent-owned quiet-window execution only. No timings are run by the worker.
#include <chrono>
#include <cstdio>
#include <iterator>
#include <random>
#include <ratio>
#include <string>
#include <string_view>

#include "base/compiler_specific.h"
#include "net/http/http_vary_data_rust_test_support.h"
namespace {
struct Case {
  const char* key;
  std::string vary;
  std::string value;
  int kind;
  int operation;  // 0 whole init/match/persist/load/match; 1 load/persist/fail
                  // sequence
  std::vector<uint8_t> payload;
};
volatile uint64_t sink = 0;
template <class V>
uint64_t Run(const Case& c,
             const net::HttpRequestHeaders& request,
             const net::HttpResponseHeaders& response,
             const base::Pickle& input,
             size_t n) {
  uint64_t result = 0;
  for (size_t i = 0; i < n; ++i) {
    V vary;
    if (c.operation == 0) {
      result += vary.Init(request, response,
                          static_cast<typename V::HashType>(c.kind));
      result += vary.MatchesRequest(request, response);
      base::Pickle p;
      if (vary.is_valid()) {
        vary.Persist(&p);
        V loaded;
        base::PickleIterator iter(p);
        result += loaded.InitFromPickle(&iter);
        result += loaded.MatchesRequest(request, response);
        result += iter.RemainingBytes() + p.size();
      }
    } else {
      // Include previous state so failures measure the retention policy too.
      result += vary.Init(request, response,
                          static_cast<typename V::HashType>(c.kind));
      base::PickleIterator iter(input);
      result += vary.InitFromPickle(&iter);
      result += iter.RemainingBytes() + static_cast<int>(vary.hash_type());
      if (vary.is_valid()) {
        base::Pickle output;
        vary.Persist(&output);
        result += output.size();
        for (auto b : base::span(output)) {
          result += b;
        }
      }
    }
    result += vary.is_valid();
  }
  return result;
}
template <class V>
double Measure(const Case& c,
               const net::HttpRequestHeaders& request,
               const net::HttpResponseHeaders& response,
               const base::Pickle& p) {
  constexpr size_t n = 20000;
  const auto start = std::chrono::steady_clock::now();
  sink = Run<V>(c, request, response, p, n);
  return std::chrono::duration<double, std::nano>(
             std::chrono::steady_clock::now() - start)
             .count() /
         n;
}
}  // namespace
int main(int argc, char** argv) {
  const bool verify_only =
      argc == 2 && UNSAFE_BUFFERS(std::string_view(argv[1])) == "--verify-only";
  if (argc != 1 && !verify_only) {
    // SAFETY: the diagnostic is a static NUL-terminated literal.
    UNSAFE_BUFFERS(
        std::fputs("Use --verify-only or no arguments for timing\n", stderr));
    return 2;
  }
  std::vector<uint8_t> current(36, 0x5a);
  current[0] = 1;
  current[1] = current[2] = current[3] = 0;
  std::vector<uint8_t> md5(20, 0x35);
  md5[0] = md5[1] = md5[2] = md5[3] = 0;
  const Case cases[] = {
      {"vary-absent", "", "", 1, 0, {}},
      {"vary-wildcard", "Vary: foo,*,missing", "value", 0, 0, {}},
      {"vary-sha-small", "Vary: foo", "abcdefgh", 1, 0, {}},
      {"vary-sha-large",
       "Vary: foo,bar,missing",
       std::string(4096, 'x'),
       1,
       0,
       {}},
      {"vary-md5-ordered",
       "Vary: foo,bar,missing\nVary: foo,bar",
       "value",
       0,
       0,
       {}},
      {"vary-load-legacy", "Vary: foo", "value", 1, 1,
       std::vector<uint8_t>(16, 0x21)},
      {"vary-load-sha", "Vary: foo", "value", 1, 1, current},
      {"vary-load-md5", "Vary: foo", "value", 1, 1, md5},
      {"vary-load-truncated",
       "Vary: foo",
       "value",
       0,
       1,
       {1, 0, 0, 0, 2, 3, 4, 5}},
      {"vary-load-invalid", "Vary: foo", "value", 1, 1,
       std::vector<uint8_t>(24, 0xff)},
  };
  std::mt19937 random(20261113);
  for (const auto& c : cases) {
    net::HttpRequestHeaders request;
    request.SetHeader("Foo", c.value);
    request.SetHeader("bar", "");
    auto response = net::http_vary_data_rust_test::Response(c.vary);
    base::Pickle input;
    input.WriteBytes(c.payload);
    net::http_vary_data_rust_test::Require(
        Run<net::HttpVaryData>(c, request, *response, input, 1) ==
        Run<net::HttpVaryDataOriginal>(c, request, *response, input, 1));
    if (verify_only) {
      continue;
    }
    sink = Run<net::HttpVaryData>(c, request, *response, input, 500);
    sink = Run<net::HttpVaryDataOriginal>(c, request, *response, input, 500);
    for (unsigned repeat = 0; repeat < 20; ++repeat) {
      const bool rust_first = (random() & 1) != 0;
      double cpp = 0, rust = 0;
      if (rust_first) {
        rust = Measure<net::HttpVaryData>(c, request, *response, input);
        cpp = Measure<net::HttpVaryDataOriginal>(c, request, *response, input);
      } else {
        cpp = Measure<net::HttpVaryDataOriginal>(c, request, *response, input);
        rust = Measure<net::HttpVaryData>(c, request, *response, input);
      }
      // SAFETY: case names and boolean strings are static NUL-terminated
      // literals; numeric arguments match the fixed format.
      UNSAFE_BUFFERS(std::printf(
          "{\"case\":\"%s\",\"repeat\":%u,\"baseline_ns\":%.9f,\"rust_ns\":%."
          "9f,\"rust_first\":%s,\"iterations\":20000}\n",
          c.key, repeat, cpp, rust, rust_first ? "true" : "false"));
    }
  }
  if (verify_only) {
    std::printf("Validated all %zu paired workloads without timing\n",
                std::size(cases));
  }
}
