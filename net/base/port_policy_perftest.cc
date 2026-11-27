// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <array>
#include <chrono>
#include <cstdio>
#include <random>
#include <ratio>
#include <string_view>

#include "base/check.h"
#include "net/base/ip_address.h"
#include "net/base/port_policy_reference.h"
#include "net/base/port_util.h"
namespace {
namespace old = net::port_policy_reference;
struct Case {
  const char* key;
  int kind;
  int port;
};
volatile uint64_t sink;
template <bool rust>
uint64_t Run(const Case& c, size_t n) {
  uint64_t value = 0;
  const net::IPEndPoint endpoint(net::IPAddress::IPv4Localhost(), 444);
  const std::array<uint16_t, 3> allowed = {22, 22, 444};
  for (size_t i = 0; i < n; ++i) {
    if (c.kind == 0) {
      value += rust ? net::IsPortAllowedForScheme(c.port, "https")
                    : old::IsPortAllowedForScheme(c.port, "https");
    } else if (c.kind == 1) {
      value += rust ? net::IsPortAllowedForIpEndpoint(endpoint)
                    : old::IsPortAllowedForIpEndpoint(endpoint);
    } else if (c.kind == 2) {
      if constexpr (rust) {
        net::ScopedPortException exception(22);
        value += net::IsPortAllowedForScheme(22, "https");
      } else {
        old::ScopedPortException exception(22);
        value += old::IsPortAllowedForScheme(22, "https");
      }
    } else {
      if constexpr (rust) {
        net::SetExplicitlyAllowedPorts(allowed);
        value += net::GetCountOfExplicitlyAllowedPorts();
        net::SetExplicitlyAllowedPorts({});
      } else {
        old::SetExplicitlyAllowedPorts(allowed);
        value += old::GetCountOfExplicitlyAllowedPorts();
        old::SetExplicitlyAllowedPorts({});
      }
    }
  }
  return value;
}
template <bool rust>
double Measure(const Case& c, size_t n) {
  auto start = std::chrono::steady_clock::now();
  auto value = Run<rust>(c, n);
  auto end = std::chrono::steady_clock::now();
  sink = value;
  return std::chrono::duration<double, std::nano>(end - start).count() / n;
}
}  // namespace
int main(int argc, char** argv) {
  const std::array<Case, 6> cases = {{{"port_invalid", 0, -1},
                                      {"port_restricted", 0, 22},
                                      {"port_regular", 0, 443},
                                      {"port_localhost", 1, 444},
                                      {"port_scoped", 2, 22},
                                      {"port_reset", 3, 0}}};
  bool verify = argc > 1 && std::string_view(argv[1]) == "--verify-only";
  std::mt19937 rng(20261112);
  constexpr size_t n = 20000;
  for (const auto& c : cases) {
    net::SetExplicitlyAllowedPorts({});
    old::SetExplicitlyAllowedPorts({});
    CHECK_EQ(Run<false>(c, 200), Run<true>(c, 200));
    if (verify) {
      continue;
    }
    for (unsigned i = 0; i < 20; ++i) {
      bool first = (rng() & 1) != 0;
      double baseline, candidate;
      if (first) {
        candidate = Measure<true>(c, n);
        baseline = Measure<false>(c, n);
      } else {
        baseline = Measure<false>(c, n);
        candidate = Measure<true>(c, n);
      }
      UNSAFE_BUFFERS(std::printf(
          "{\"case\":\"%s\",\"repeat\":%u,\"baseline_ns\":%.9f,\"rust_ns\":%."
          "9f,\"iterations\":%zu,\"rust_first\":%s}\n",
          c.key, i, baseline, candidate, n, first ? "true" : "false"));
    }
  }
}
