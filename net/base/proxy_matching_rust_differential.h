// Copyright 2026 The Chromium Authors
#ifndef NET_BASE_PROXY_MATCHING_RUST_DIFFERENTIAL_H_
#define NET_BASE_PROXY_MATCHING_RUST_DIFFERENTIAL_H_
#include <array>
#include <cstdio>
#include <cstdlib>
#include <source_location>
#include <string>
#include <string_view>

#include "net/base/proxy_matching_rust_reference.h"
#include "net/proxy_resolution/proxy_host_matching_rules.h"
#include "url/gurl.h"
namespace net::proxy_matching_rust_test {
inline constexpr std::array<std::string_view, 24> kRules = {
    "",
    "*",
    "<local>",
    "<-loopback>",
    ".example.com",
    "https://*.example.com:443",
    "127.0.0.1:0",
    "http://[0:0::1]:80",
    "192.168.0.0/16",
    "::ffff:192.168.0.0/112",
    "HTTP://127.0.0.0/8",
    "foo:65535",
    "foo:65536",
    "a?b*",
    R"(a\*b)",
    "[::]/0",
    " <-LOOPBACK> ; * ; <LOCAL>",
    "localhost",
    "169.254.0.0/16",
    "fe80::/10",
    "0.0.0.0:0",
    "x://foo:0001",
    "a://",
    "a;bad:/,b"};
inline constexpr std::array<std::string_view, 24> kUrls = {
    "http://foo",
    "http://localhost",
    "http://localhost.",
    "http://a.localhost",
    "http://loopback.",
    "http://127.1",
    "http://[::1]",
    "http://[::ffff:127.0.0.1]",
    "http://169.254.1.2",
    "http://[::ffff:169.254.1.2]",
    "http://[fe80::1]",
    "http://0",
    "http://[::]",
    "http://[::ffff:0:0]",
    "http://example.com",
    "https://a.example.com",
    "ftp://example.com:22",
    "http://192.168.1.1",
    "http://[::ffff:c0a8:101]",
    "data:x",
    "",
    "http://%41",
    "http://xn--flw351e.cn",
    "http://foobar:65535"};
static std::string_view current_raw, current_url;
inline void Check(bool v,
                  std::source_location loc = std::source_location::current()) {
  if (!v) {
    std::fprintf(stderr, "differential mismatch line=%u raw=", loc.line());
    for (unsigned char c : current_raw) {
      std::fprintf(stderr, "%02x", c);
    }
    std::fprintf(stderr, " url=");
    for (unsigned char c : current_url) {
      std::fprintf(stderr, "%02x", c);
    }
    std::fprintf(stderr, "\n");
    std::abort();
  }
}
template <class A, class B>
void Compare(const A& a, const B& b, const GURL& url) {
  Check(a.ToString() == b.ToString());
  Check(a.rules().size() == b.rules().size());
  for (size_t i = 0; i < a.rules().size(); ++i) {
    Check(a.rules()[i]->ToString() == b.rules()[i]->ToString());
    Check(a.rules()[i]->IsHostnamePatternRule() ==
          b.rules()[i]->IsHostnamePatternRule());
    Check(static_cast<int>(a.rules()[i]->Evaluate(url)) ==
          static_cast<int>(b.rules()[i]->Evaluate(url)));
  }
}
inline void Differential(std::string raw, std::string url_raw, uint64_t n) {
  using Ref = proxy_matching_reference::ProxyHostMatchingRules;
  current_raw = raw;
  current_url = url_raw;
  GURL url(url_raw);
  ProxyHostMatchingRules a;
  Ref b;
  a.ParseFromString(raw);
  b.ParseFromString(raw);
  Compare(a, b, url);
  Check(a.Matches(url) == b.Matches(url));
  Check(a.Matches(url, true) == b.Matches(url, true));
  Check(a.MatchesImplicitRules(url) == b.MatchesImplicitRules(url));
  ProxyHostMatchingRules ac(a);
  Ref bc(b);
  Check((a == ac) == (b == bc));
  const auto* ap = &ac;
  const auto* bp = &bc;
  ac = *ap;
  bc = *bp;
  Compare(ac, bc, url);
  std::string addition(kRules[n % kRules.size()]);
  Check(a.AddRuleFromString(addition) == b.AddRuleFromString(addition));
  if (n & 1) {
    a.PrependRuleToBypassSimpleHostnames();
    b.PrependRuleToBypassSimpleHostnames();
  }
  if (n & 2) {
    a.AddRulesToSubtractImplicit();
    b.AddRulesToSubtractImplicit();
  }
  if (!a.rules().empty() && (n & 4)) {
    auto x = SchemeHostPortMatcherRule::FromUntrimmedRawString("*.example.com");
    auto y = proxy_matching_reference::SchemeHostPortMatcherRule::
        FromUntrimmedRawString("*.example.com");
    size_t i = n % a.rules().size();
    a.ReplaceRule(i, std::move(x));
    b.ReplaceRule(i, std::move(y));
  }
  Compare(a, b, url);
  Check(a.Matches(url) == b.Matches(url));
  Check(a.Matches(url, true) == b.Matches(url, true));
  Check((a == ac) == (b == bc));
  ac = a;
  bc = b;
  Compare(ac, bc, url);
  ProxyHostMatchingRules am(std::move(ac));
  Ref bm(std::move(bc));
  Compare(am, bm, url);
  ac = std::move(am);
  bc = std::move(bm);
  Compare(ac, bc, url);
  ac.Clear();
  bc.Clear();
  Compare(ac, bc, url);
  auto ma = SchemeHostPortMatcher::FromRawString(raw);
  auto mb = proxy_matching_reference::SchemeHostPortMatcher::FromRawString(raw);
  Compare(ma, mb, url);
  Check(static_cast<int>(ma.Evaluate(url)) ==
        static_cast<int>(mb.Evaluate(url)));
  Check(ma.Includes(url) == mb.Includes(url));
  auto ra = SchemeHostPortMatcherRule::FromUntrimmedRawString(raw);
  auto rb = proxy_matching_reference::SchemeHostPortMatcherRule::
      FromUntrimmedRawString(raw);
  Check(bool(ra) == bool(rb));
  if (ra) {
    Check(ra->ToString() == rb->ToString());
    Check(static_cast<int>(ra->Evaluate(url)) ==
          static_cast<int>(rb->Evaluate(url)));
    if (ra->IsHostnamePatternRule()) {
      auto sa = static_cast<SchemeHostPortMatcherHostnamePatternRule*>(ra.get())
                    ->GenerateSuffixMatchingRule();
      auto sb =
          static_cast<proxy_matching_reference::
                          SchemeHostPortMatcherHostnamePatternRule*>(rb.get())
              ->GenerateSuffixMatchingRule();
      Check(sa->ToString() == sb->ToString());
      Check(sa->Evaluate(url) == sb->Evaluate(url));
    }
  }
}
}  // namespace net::proxy_matching_rust_test
#endif
