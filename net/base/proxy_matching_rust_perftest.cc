// Copyright 2026 The Chromium Authors
#include <chrono>
#include <cstdio>
#include <random>
#include <ratio>
#include <type_traits>

#include "base/check.h"
#include "base/i18n/icu_util.h"
#include "net/base/proxy_matching_rust_differential.h"
namespace {
struct Case {
  const char* key;
  int operation;
  const char* rules;
  const char* url;
};
constexpr std::array<Case, 15> kCases = {
    {{"parse_hostname", 0, "example.com;*.example.org;https://foo:443",
      "http://example.com"},
     {"parse_ip", 0, "127.0.0.1;[::1]:80;192.168.0.0/16;http://fe80::/10",
      "http://127.0.0.1"},
     {"parse_mixed", 0, "<local>;*.example.com;http://127.0.0.0/8;<-loopback>",
      "http://a.example.com"},
     {"parse_invalid", 0, "foo:65536;http://;[::ffff::]/129;bad:port;;,",
      "http://example.com"},
     {"matches_host", 1, "foo;bar;*.example.com", "https://a.example.com"},
     {"matches_miss", 1, "foo;bar;*.example.com", "http://example.org"},
     {"matches_cidr", 1, "192.168.0.0/16;[::1];127.0.0.0/8",
      "http://[::ffff:c0a8:101]"},
     {"implicit_local", 1, "", "http://a.localhost"},
     {"implicit_linklocal", 1, "", "http://[fe80::1234]"},
     {"implicit_unspecified", 1, "", "http://[::ffff:0:0]"},
     {"reverse_subtraction", 1, "<local>;<-loopback>", "http://[::1]"},
     {"serialize_mixed", 2,
      "<local>;https://*.example.com:443;192.168.0.0/16;<-loopback>",
      "http://example.com"},
     {"copy_equal", 3, "<local>;*.example.com;127.0.0.0/8;<-loopback>",
      "http://example.com"},
     {"mutate_replace", 4, "*.example.com;foo;bar", "http://a.example.com"},
     {"scheme_suffix", 5, "https://.example.com:443",
      "https://a.example.com"}}};
volatile uint64_t sink;
template <bool rust>
uint64_t Run(const Case& c, size_t n) {
  using Proxy =
      std::conditional_t<rust, net::ProxyHostMatchingRules,
                         net::proxy_matching_reference::ProxyHostMatchingRules>;
  using Rule = std::conditional_t<
      rust, net::SchemeHostPortMatcherRule,
      net::proxy_matching_reference::SchemeHostPortMatcherRule>;
  using Host = std::conditional_t<
      rust, net::SchemeHostPortMatcherHostnamePatternRule,
      net::proxy_matching_reference::SchemeHostPortMatcherHostnamePatternRule>;
  const std::string raw(c.rules);
  const GURL url(c.url);
  Proxy p;
  p.ParseFromString(raw);
  uint64_t value = 0;
  for (size_t i = 0; i < n; ++i) {
    switch (c.operation) {
      case 0: {
        Proxy parsed;
        parsed.ParseFromString(raw);
        value += parsed.rules().size();
        break;
      }
      case 1:
        value += p.Matches(url);
        value += p.Matches(url, true);
        break;
      case 2:
        value += p.ToString().size();
        break;
      case 3: {
        Proxy copy(p);
        value += (copy == p);
        break;
      }
      case 4: {
        p.ReplaceRule(
            i % p.rules().size(),
            Rule::FromUntrimmedRawString(i & 1 ? "*.example.com" : "<local>"));
        value += p.Matches(url);
        break;
      }
      case 5: {
        auto rule = Rule::FromUntrimmedRawString(raw);
        auto suffix =
            static_cast<Host*>(rule.get())->GenerateSuffixMatchingRule();
        value += static_cast<uint64_t>(suffix->Evaluate(url));
        value += suffix->ToString().size();
        break;
      }
    }
  }
  return value;
}
template <bool rust>
double Measure(const Case& c, size_t n) {
  // Run includes warm setup once per sample; timing includes that setup
  // equally, all public calls, FFI, RuleList/Rust allocations, and destruction.
  auto begin = std::chrono::steady_clock::now();
  auto value = Run<rust>(c, n);
  auto end = std::chrono::steady_clock::now();
  sink = value;
  return std::chrono::duration<double, std::nano>(end - begin).count() / n;
}
}  // namespace
int main(int argc, char** argv) {
  base::i18n::InitializeICU();
  bool verify = argc > 1 && std::string_view(argv[1]) == "--verify-only";
  std::mt19937 rng(20261113);
  constexpr size_t n = 10000;
  for (const auto& c : kCases) {
    CHECK_EQ(Run<false>(c, 100), Run<true>(c, 100));
    if (verify) {
      continue;
    }
    for (unsigned pair = 0; pair < 20; ++pair) {
      bool first = (rng() & 1) != 0;
      double a, b;
      if (first) {
        b = Measure<true>(c, n);
        a = Measure<false>(c, n);
      } else {
        a = Measure<false>(c, n);
        b = Measure<true>(c, n);
      }
      UNSAFE_BUFFERS(std::printf(
          "{\"case\":\"%s\",\"repeat\":%u,\"baseline_ns\":%.9f,\"rust_ns\":%."
          "9f,\"iterations\":%zu,\"rust_first\":%s}\n",
          c.key, pair, a, b, n, first ? "true" : "false"));
    }
  }
}
