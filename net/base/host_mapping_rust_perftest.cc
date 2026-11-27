// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <array>
#include <chrono>
#include <cstdio>
#include <random>
#include <ratio>
#include <string_view>
#include <type_traits>

#include "base/check_op.h"
#include "base/compiler_specific.h"
#include "net/base/host_mapping_rules.h"
#include "net/base/host_mapping_rust_reference.h"
#include "net/base/host_port_pair.h"
#include "url/gurl.h"
namespace {
struct Case {
  const char* key;
  int kind;
  const char* rules;
  const char* host;
  uint16_t port;
};
volatile uint64_t sink;
template <bool rust>
uint64_t Run(const Case& c, size_t count) {
  using Rules = std::conditional_t<rust, net::HostMappingRules,
                                   net::HostMappingRustReference>;
  Rules rules;
  rules.SetRulesFromString(c.rules);
  uint64_t sum = 0;
  for (size_t i = 0; i < count; ++i) {
    switch (c.kind) {
      case 0: {
        net::HostPortPair pair(c.host, c.port);
        sum += rules.RewriteHost(&pair);
        sum += pair.host().size() + pair.port();
        break;
      }
      case 1: {
        Rules parsed;
        sum += parsed.AddRuleFromString(c.rules);
        break;
      }
      case 2: {
        Rules parsed;
        parsed.SetRulesFromString(c.rules);
        net::HostPortPair pair(c.host, c.port);
        sum += parsed.RewriteHost(&pair);
        sum += pair.host().size();
        break;
      }
      case 3: {
        GURL url(c.host);
        sum += static_cast<int>(rules.RewriteUrl(url));
        sum += url.spec().size();
        break;
      }
      case 4: {
        Rules copy(rules);
        Rules assigned;
        assigned = copy;
        net::HostPortPair pair(c.host, c.port);
        sum += assigned.RewriteHost(&pair);
        sum += pair.host().size();
        break;
      }
    }
  }
  return sum;
}
template <bool rust>
double Measure(const Case& c, size_t n) {
  const auto start = std::chrono::steady_clock::now();
  const auto value = Run<rust>(c, n);
  const auto end = std::chrono::steady_clock::now();
  sink = value;
  return std::chrono::duration<double, std::nano>(end - start).count() / n;
}
}  // namespace
int main(int argc, char** argv) {
  const std::array<Case, 12> cases = {
      {{"mapping_host", 0, "MAP initial.test replacement.test", "initial.test",
        80},
       {"mapping_port", 0, "MAP initial.test:443 replacement.test:123",
        "initial.test", 443},
       {"mapping_wildcard", 0, "MAP *.example.test replacement.test:0",
        "www.example.test", 65535},
       {"mapping_excluded", 0, "MAP * replacement.test, EXCLUDE *.example.test",
        "www.example.test", 80},
       {"mapping_miss", 0, "MAP *.example.test replacement.test",
        "unmapped.invalid", 80},
       {"mapping_ipv6", 0, "MAP [::1]:443 [2001:db8::42]:8443", "::1", 443},
       {"mapping_add", 1, "MAP *.example.test [2001:db8::42]:443",
        "www.example.test", 80},
       {"mapping_set", 2,
        "MAP initial.test first, MAP *.example.test second:90, EXCLUDE "
        "blocked.example.test",
        "www.example.test", 80},
       {"mapping_copy", 4,
        "MAP *.example.test replacement.test, EXCLUDE blocked.example.test",
        "www.example.test", 80},
       {"mapping_url", 3, "MAP initial.test replacement.test:8443",
        "https://initial.test/path?query#fragment", 443},
       {"mapping_invalid_url", 3, "MAP initial.test ^NOTFOUND",
        "https://initial.test/path", 443},
       {"mapping_file_url", 3, "MAP initial.test replacement.test:8443",
        "file://initial.test/path", 0}}};
  const bool verify = argc > 1 && std::string_view(argv[1]) == "--verify-only";
  std::mt19937 random(20261113);
  constexpr size_t count = 20000;
  for (const auto& c : cases) {
    CHECK_EQ(Run<false>(c, 200), Run<true>(c, 200));
    if (verify) {
      continue;
    }
    for (unsigned repeat = 0; repeat < 20; ++repeat) {
      const bool rust_first = (random() & 1) != 0;
      double baseline, candidate;
      if (rust_first) {
        candidate = Measure<true>(c, count);
        baseline = Measure<false>(c, count);
      } else {
        baseline = Measure<false>(c, count);
        candidate = Measure<true>(c, count);
      }
      UNSAFE_BUFFERS(std::printf(
          "{\"case\":\"%s\",\"repeat\":%u,\"baseline_ns\":%.9f,\"rust_ns\":%."
          "9f,\"iterations\":%zu,\"rust_first\":%s}\n",
          c.key, repeat, baseline, candidate, count,
          rust_first ? "true" : "false"));
    }
  }
}
