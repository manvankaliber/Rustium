// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_COOKIES_PARSED_COOKIE_RUST_TEST_SUPPORT_H_
#define NET_COOKIES_PARSED_COOKIE_RUST_TEST_SUPPORT_H_
#include <array>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/check.h"
#include "net/cookies/cookie_inclusion_status.h"
#include "net/cookies/cookie_inclusion_status_rust_reference.h"
#include "net/cookies/parsed_cookie.h"
#include "net/cookies/parsed_cookie_rust_reference.h"
namespace net::parsed_cookie_rust_test {
using Original = ParsedCookieRustOriginal;
using OriginalStatus = CookieInclusionStatusRustOriginal;

// Each parser owns its own status policy, including the inline EnumSet paths.
// Compare values, never reinterpret or convert one implementation's status.
inline void CompareStatus(const OriginalStatus& a,
                          const CookieInclusionStatus& b) {
  CHECK_EQ(a.exclusion_reasons().ToEnumBitmask(),
           b.exclusion_reasons().ToEnumBitmask());
  CHECK_EQ(a.warning_reasons().ToEnumBitmask(),
           b.warning_reasons().ToEnumBitmask());
  CHECK_EQ(static_cast<int>(a.exemption_reason()),
           static_cast<int>(b.exemption_reason()));
  CHECK_EQ(a.IsInclude(), b.IsInclude());
  CHECK_EQ(a.ShouldWarn(), b.ShouldWarn());
  CHECK_EQ(a.GetDebugString(), b.GetDebugString());
}

template <typename Status>
void SeedStatus(Status& status) {
  status.AddExclusionReason(Status::ExclusionReason::EXCLUDE_FAILURE_TO_STORE);
}
using Attributes = std::vector<std::pair<std::string, std::string>>;
template <typename T>
Attributes Collect(const T& c) {
  Attributes out;
  CHECK(c.ForEachAttribute([&](std::string_view n, std::string_view v) {
    out.emplace_back(n, v);
    return true;
  }));
  for (size_t stop = 0; stop <= out.size(); ++stop) {
    size_t calls = 0;
    const bool completed =
        c.ForEachAttribute([&](std::string_view n, std::string_view v) {
          CHECK_EQ(n, out[calls].first);
          CHECK_EQ(v, out[calls].second);
          return calls++ != stop;
        });
    CHECK_EQ(completed, stop == out.size());
    CHECK_EQ(calls, stop < out.size() ? stop + 1 : out.size());
  }
  return out;
}
inline void Compare(const Original& a, const ParsedCookie& b) {
  CHECK_EQ(a.IsValid(), b.IsValid());
  CHECK_EQ(a.HasInternalHtab(), b.HasInternalHtab());
  CHECK(a.NamelessCookieLineParseTypeForMetrics() ==
        b.NamelessCookieLineParseTypeForMetrics());
  CHECK_EQ(a.ToCookieLine(), b.ToCookieLine());
  if (!a.IsValid()) {
    return;
  }
  CHECK_EQ(a.Name(), b.Name());
  CHECK_EQ(a.Token(), b.Token());
  CHECK_EQ(a.Value(), b.Value());
  CHECK(a.Path() == b.Path());
  CHECK(a.Domain() == b.Domain());
  CHECK(a.Expires() == b.Expires());
  CHECK(a.MaxAge() == b.MaxAge());
  CHECK_EQ(a.IsSecure(), b.IsSecure());
  CHECK_EQ(a.IsHttpOnly(), b.IsHttpOnly());
  CHECK_EQ(a.IsPartitioned(), b.IsPartitioned());
  CHECK(a.SameSite() == b.SameSite());
  CHECK_EQ(a.Priority(), b.Priority());
  CHECK_EQ(a.NumberOfAttributes(), b.NumberOfAttributes());
  CHECK(Collect(a) == Collect(b));
}
template <typename T>
bool Set(T& c, size_t op, std::string_view v, bool enabled) {
  switch (op) {
    case 0:
      return c.SetName(v);
    case 1:
      return c.SetValue(v);
    case 2:
      return c.SetPath(v);
    case 3:
      return c.SetDomain(v);
    case 4:
      return c.SetExpires(v);
    case 5:
      return c.SetMaxAge(v);
    case 6:
      return c.SetIsSecure(enabled);
    case 7:
      return c.SetIsHttpOnly(enabled);
    case 8:
      return c.SetSameSite(v);
    case 9:
      return c.SetPriority(v);
    default:
      return c.SetIsPartitioned(enabled);
  }
}
template <typename T>
std::string_view Alias(T& c, size_t selector) {
  if (!c.IsValid()) {
    return {};
  }
  if (selector == 0) {
    return c.Name();
  }
  if (selector == 1) {
    return c.Value();
  }
  std::string_view result;
  size_t i = 2;
  c.ForEachAttribute([&](std::string_view n, std::string_view v) {
    if (i++ == selector) {
      result = n;
      return false;
    }
    if (i++ == selector) {
      result = v;
      return false;
    }
    return true;
  });
  return result;
}
inline void Statics(std::string_view s) {
  CHECK(Original::FindFirstTerminator(s) ==
        ParsedCookie::FindFirstTerminator(s));
  CHECK_EQ(Original::ParseTokenString(s), ParsedCookie::ParseTokenString(s));
  CHECK_EQ(Original::ParseTokenString(s).data(),
           ParsedCookie::ParseTokenString(s).data());
  CHECK_EQ(Original::ParseValueString(s), ParsedCookie::ParseValueString(s));
  CHECK_EQ(Original::ParseValueString(s).data(),
           ParsedCookie::ParseValueString(s).data());
  CHECK_EQ(Original::ValueMatchesParsedValue(s),
           ParsedCookie::ValueMatchesParsedValue(s));
  CHECK_EQ(Original::IsValidCookieName(s), ParsedCookie::IsValidCookieName(s));
  CHECK_EQ(Original::IsValidCookieValue(s),
           ParsedCookie::IsValidCookieValue(s));
  CHECK_EQ(Original::CookieAttributeValueHasValidCharSet(s),
           ParsedCookie::CookieAttributeValueHasValidCharSet(s));
  CHECK_EQ(Original::CookieAttributeValueHasValidSize(s),
           ParsedCookie::CookieAttributeValueHasValidSize(s));
  OriginalStatus sa;
  CookieInclusionStatus sb;
  SeedStatus(sa);
  SeedStatus(sb);
  CHECK_EQ(Original::IsValidCookieNameValuePair(s, s, &sa),
           ParsedCookie::IsValidCookieNameValuePair(s, s, &sb));
  CompareStatus(sa, sb);
  auto ia = s.begin();
  auto ib = s.begin();
  auto as = s.end();
  auto ae = s.end();
  auto bs = s.end();
  auto be = s.end();
  CHECK_EQ(Original::ParseToken(&ia, s.end(), &as, &ae),
           ParsedCookie::ParseToken(&ib, s.end(), &bs, &be));
  CHECK(ia == ib && as == bs && ae == be);
  Original::ParseValue(&ia, s.end(), &as, &ae);
  ParsedCookie::ParseValue(&ib, s.end(), &bs, &be);
  CHECK(ia == ib && as == bs && ae == be);
  ia = ib = s.begin();
  Original::ParseValue(&ia, s.end(), &as, &ae);
  ParsedCookie::ParseValue(&ib, s.end(), &bs, &be);
  CHECK(ia == ib && as == bs && ae == be);
}
inline void Exercise(std::string_view line,
                     std::string_view mutation,
                     uint64_t bits) {
  OriginalStatus sa;
  CookieInclusionStatus sb;
  SeedStatus(sa);
  SeedStatus(sb);
  Original a(line, &sa);
  ParsedCookie b(line, &sb);
  CompareStatus(sa, sb);
  Compare(a, b);
  for (size_t step = 0; step < 6; ++step) {
    const size_t op = (bits >> (step * 7)) % 11;
    const bool enabled = ((bits >> (step + 7)) & 1) != 0;
    const size_t selector = (bits >> (step * 6 + 3)) % 26;
    auto av = selector < 14 ? Alias(a, selector) : mutation;
    auto bv = selector < 14 ? Alias(b, selector) : mutation;
    if ((bits >> (step + 42)) & 1) {
      av = av.substr(av.size() / 2);
      bv = bv.substr(bv.size() / 2);
    }
    CHECK_EQ(Set(a, op, av, enabled), Set(b, op, bv, enabled));
    Compare(a, b);
  }
  Statics(mutation);
}
}  // namespace net::parsed_cookie_rust_test
#endif
