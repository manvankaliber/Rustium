// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <array>
#include <chrono>
#include <cstdio>
#include <random>
#include <ratio>
#include <string_view>

#include "base/check_op.h"
#include "net/base/host_port_pair.h"
#include "net/base/host_port_rust_reference.h"
#include "net/base/ip_address.h"
#include "net/base/ip_endpoint.h"
#include "net/base/url_util.h"
#include "url/gurl.h"
#include "url/scheme_host_port.h"

namespace {
struct Case {
  const char* key;
  int kind;
  const char* input;
};
volatile uint64_t sink;
template <bool rust>
uint64_t Run(const Case& c, size_t count) {
  using Pair =
      std::conditional_t<rust, net::HostPortPair, net::HostPortRustReference>;
  const Pair sample(c.input, 443), other("other.example", 80);
  const auto value = sample.ToValue();
  const GURL url("https://[::1]:443/path");
  const url::SchemeHostPort scheme(url);
  const net::IPEndPoint endpoint(net::IPAddress::IPv4Localhost(), 443);
  uint64_t total = 0;
  for (size_t i = 0; i < count; ++i) {
    switch (c.kind) {
      case 0: {
        auto p = Pair::FromString(c.input);
        total += p.host().size() + p.port();
        break;
      }
      case 1: {
        auto s = sample.ToString();
        total += s.size();
        break;
      }
      case 2:
        total += sample < other;
        break;
      case 3: {
        auto p = Pair::FromValue(value);
        total += p ? p->host().size() + p->port() : 0;
        break;
      }
      case 4: {
        std::string host;
        int port = 0;
        bool ok;
        if constexpr (rust) {
          ok = net::ParseHostAndPort(c.input, &host, &port);
        } else {
          ok = net::HostPortRustOriginalParseHostAndPort(c.input, &host, &port);
        }
        total += ok ? host.size() + static_cast<unsigned>(port + 1) : 0;
        break;
      }
      case 5: {
        auto p = Pair::FromURL(url);
        total += p.host().size() + p.port();
        break;
      }
      case 6: {
        auto p = Pair::FromSchemeHostPort(scheme);
        total += p.host().size() + p.port();
        break;
      }
      case 7: {
        auto p = Pair::FromIPEndPoint(endpoint);
        total += p.host().size() + p.port();
        break;
      }
      case 8: {
        auto v = sample.ToValue();
        total += v.GetDict().size();
        break;
      }
      case 9: {
        auto host = sample.HostForURL();
        total += host.size();
        break;
      }
      case 10: {
        Pair p;
        p.set_host(c.input);
        p.set_port(443);
        total += !p.IsEmpty() && p == sample;
        break;
      }
    }
  }
  return total;
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
  const std::array<Case, 15> cases = {
      {{"pair_parse_dns", 0, "www.example.com:443"},
       {"pair_parse_ipv6", 0, "[2001:db8::42]:443"},
       {"pair_parse_invalid", 0, "example:bad"},
       {"pair_format_dns", 1, "www.example.com"},
       {"pair_format_ipv6", 1, "2001:db8::42"},
       {"pair_compare", 2, "www.example.com"},
       {"pair_value", 3, "www.example.com"},
       {"helper_ipv6", 4, "[2001:db8::42]:443"},
       {"helper_unusual", 4, "::1"},
       {"pair_from_url", 5, "example"},
       {"pair_from_scheme", 6, "example"},
       {"pair_from_endpoint", 7, "example"},
       {"pair_to_value", 8, "example"},
       {"pair_host_for_url", 9, "2001:db8::42"},
       {"pair_set_empty_equal", 10, "www.example.com"}}};
  bool verify = argc > 1 && std::string_view(argv[1]) == "--verify-only";
  std::mt19937 rng(20261112);
  constexpr size_t n = 20000;
  for (const auto& c : cases) {
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
