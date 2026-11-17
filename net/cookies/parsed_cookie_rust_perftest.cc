// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Prepared only; the parent runs timing in a coordinated quiet window.
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <random>
#include <ratio>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "base/compiler_specific.h"
#include "net/cookies/parsed_cookie_rust_test_support.h"
namespace {
struct Case {
  const char* key;
  std::string input;
  unsigned mode;
  size_t iterations;
};
volatile uint64_t sink = 0;
uint64_t Consume(std::string_view s) {
  return s.size() * 257 + (s.empty()
                               ? 0
                               : static_cast<unsigned char>(s.front()) +
                                     static_cast<unsigned char>(s.back()));
}
template <bool Rust>
uint64_t Run(const Case& c, size_t count) {
  using Cookie = std::conditional_t<Rust, net::ParsedCookie,
                                    net::ParsedCookieRustOriginal>;
  using Status = std::conditional_t<Rust, net::CookieInclusionStatus,
                                    net::CookieInclusionStatusRustOriginal>;
  uint64_t sum = 0;
  for (size_t i = 0; i < count; ++i) {
    if (c.mode == 5) {
      sum += Cookie::IsValidCookieName(c.input) +
             Cookie::IsValidCookieValue(c.input) +
             Cookie::CookieAttributeValueHasValidSize(c.input);
      sum += Consume(Cookie::ParseTokenString(c.input)) +
             Consume(Cookie::ParseValueString(c.input));
      sum += Cookie::ValueMatchesParsedValue(c.input) +
             (Cookie::FindFirstTerminator(c.input) - c.input.begin());
      sum += Cookie::IsValidCookieNameValuePair(c.input, "value");
      auto it = c.input.begin();
      auto start = it;
      auto end = it;
      sum += Cookie::ParseToken(&it, c.input.end(), &start, &end);
      Cookie::ParseValue(&it, c.input.end(), &start, &end);
      sum += end - start;
      continue;
    }
    Status status;
    Cookie cookie(c.input, &status);
    if (c.mode == 1) {
      sum += cookie.SetName("modified");
      sum += cookie.SetValue("new value");
      sum += cookie.SetPath(" /new ");
      sum += cookie.SetDomain("example.test") + cookie.SetExpires("tomorrow");
      sum += cookie.SetMaxAge("900");
      sum += cookie.SetSameSite("Lax");
      sum += cookie.SetPriority("high");
      sum += cookie.SetIsSecure(true);
      sum += cookie.SetIsHttpOnly(true) + cookie.SetIsPartitioned(true);
      sum += cookie.SetPath({});
      sum += cookie.SetIsSecure(false);
    } else if (c.mode == 2) {
      sum += cookie.SetName(std::string_view(cookie.Name()).substr(1));
      sum += cookie.SetValue(cookie.Name());
      sum += cookie.SetDomain(cookie.Value());
      sum += cookie.SetPath(*cookie.Domain());
      sum += cookie.SetSameSite(*cookie.Path());
      sum += cookie.SetPriority(std::string_view(cookie.Value()).substr(1));
    } else if (c.mode == 3) {
      sum += cookie.SetName("repaired");
      sum += cookie.SetValue("value");
      sum += cookie.SetPath("/");
      sum += cookie.SetIsSecure(true);
    }
    sum += cookie.IsValid() + cookie.HasInternalHtab() + status.IsInclude() +
           status.ShouldWarn();
    if (const auto metric = cookie.NamelessCookieLineParseTypeForMetrics()) {
      sum += static_cast<int>(*metric);
    }
    if (cookie.IsValid()) {
      sum += Consume(cookie.Name()) + Consume(cookie.Token()) +
             Consume(cookie.Value());
      sum += cookie.NumberOfAttributes() + cookie.IsSecure() +
             cookie.IsHttpOnly() + cookie.IsPartitioned();
      for (auto attr : {cookie.Path(), cookie.Domain(), cookie.Expires(),
                        cookie.MaxAge()}) {
        if (attr) {
          sum += Consume(*attr);
        }
      }
      const auto same_site = cookie.SameSite();
      sum += static_cast<int>(same_site.first) +
             static_cast<int>(same_site.second) + cookie.Priority();
      cookie.ForEachAttribute(
          [&](std::string_view name, std::string_view value) {
            sum += Consume(name) + Consume(value);
            return true;
          });
    }
    if (c.mode != 0) {
      sum += Consume(cookie.ToCookieLine());
    }
  }
  return sum;
}
template <bool Rust>
double Measure(const Case& c) {
  const auto start = std::chrono::steady_clock::now();
  sink = Run<Rust>(c, c.iterations);
  return std::chrono::duration<double, std::nano>(
             std::chrono::steady_clock::now() - start)
             .count() /
         c.iterations;
}
}  // namespace
int main(int argc, char** argv) {
  const bool validate_only =
      argc == 2 &&
      UNSAFE_BUFFERS(std::string_view(argv[1])) == "--validate-only";
  const std::string attrs =
      "session=abcdef; Path=/account; Domain=example.test; Expires=Wed, 09 Jun "
      "2027 10:18:14 GMT; Max-Age=3600; Secure; HttpOnly; SameSite=Lax; "
      "Priority=High; Partitioned; extension=opaque";
  const std::vector<Case> cases = {
      {"cookie_parse_simple", "session=abcdef", 0, 100000},
      {"cookie_parse_attributes", attrs, 0, 30000},
      {"cookie_parse_duplicates",
       "n=v; path=/first; path=/second; Secure=x; secure=y; samesite=lax; "
       "samesite=none; extension=a; extension=b",
       0, 30000},
      {"cookie_parse_rejected",
       std::string("n=v; path=/; invalid=embedded\0tail", 32), 0, 100000},
      {"cookie_parse_oversized",
       "n=v; path=" + std::string(1025, 'x') + "; domain=example.test", 0,
       30000},
      {"cookie_mutate_all", attrs, 1, 20000},
      {"cookie_mutate_alias", attrs, 2, 20000},
      {"cookie_repair_invalid", "n=v; path=first; invalid=control\tbyte", 3,
       40000},
      {"cookie_serialize_attributes", attrs, 4, 30000},
      {"cookie_static_scanners", " \tname \t= value \t; next=second", 5,
       100000}};
  std::mt19937 random(20261112);
  for (const auto& c : cases) {
    CHECK_EQ(Run<false>(c, 2048), Run<true>(c, 2048));
    if (validate_only) {
      continue;
    }
    for (unsigned repeat = 0; repeat < 20; ++repeat) {
      const bool first = (random() & 1) != 0;
      double cpp, rust;
      if (first) {
        rust = Measure<true>(c);
        cpp = Measure<false>(c);
      } else {
        cpp = Measure<false>(c);
        rust = Measure<true>(c);
      }
      // SAFETY: key points to one of the static NUL-terminated literals above.
      UNSAFE_BUFFERS(std::printf(
          "{\"case\":\"%s\",\"input_bytes\":%zu,\"iterations\":%zu,\"repeat\":%"
          "u,\"rust_first\":%s,\"baseline_ns\":%.6f,\"rust_ns\":%.6f}\n",
          c.key, c.input.size(), c.iterations, repeat, first ? "true" : "false",
          cpp, rust));
      std::fflush(stdout);
    }
  }
}
