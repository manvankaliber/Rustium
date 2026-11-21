// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_BASE_HOST_MAPPING_RUST_TEST_SUPPORT_H_
#define NET_BASE_HOST_MAPPING_RUST_TEST_SUPPORT_H_
#include <string>
#include <string_view>

#include "base/check.h"
#include "base/compiler_specific.h"
#include "net/base/host_mapping_rules.h"
#include "net/base/host_mapping_rust_reference.h"
#include "net/base/host_port_pair.h"
#include "url/gurl.h"
namespace net::host_mapping_rust_test {
inline void CheckRewrite(const HostMappingRules& actual,
                         const HostMappingRustReference& original,
                         std::string_view host,
                         uint16_t port) {
  HostPortPair a(host, port), b(host, port);
  CHECK_EQ(actual.RewriteHost(&a), original.RewriteHost(&b));
  CHECK_EQ(a.host(), b.host());
  CHECK_EQ(a.port(), b.port());
}
inline void CheckUrls(const HostMappingRules& actual,
                      const HostMappingRustReference& original) {
  for (const auto* input :
       {"http://initial.test:123/a?b#c", "https://foo.com/", "http://[::1]:80/",
        "file://foo.com/a", "wss://initial.test/"}) {
    GURL a(input), b(input);
    CHECK_EQ(static_cast<int>(actual.RewriteUrl(a)),
             static_cast<int>(original.RewriteUrl(b)));
    CHECK_EQ(a.possibly_invalid_spec(), b.possibly_invalid_spec());
  }
}
// Entire mutable API sequence, not a direct Rust candidate invocation.
inline void State(std::string_view data) {
  HostMappingRules a;
  HostMappingRustReference b;
  const std::string_view fixed[] = {"MAP * replacement:123",
                                    "MAP *.com:80 initial.test",
                                    "EXCLUDE initial.test",
                                    "MAP initial.test [::1]:65535",
                                    "MAP * ^NOTFOUND",
                                    "MAP * invalid/url",
                                    "MAP * one, MAP * two",
                                    "MAP  bad rule",
                                    "EXCLUDE ",
                                    "MAP ? end:0",
                                    "MAP [::1]:80 ipv6",
                                    "MAP * first, EXCLUDE *.com"};
  size_t cursor = 0;
  while (cursor < data.size()) {
    const auto op = static_cast<unsigned char>(data[cursor++]);
    const size_t len = std::min<size_t>(op % 31, data.size() - cursor);
    const auto part = data.substr(cursor, len);
    cursor += len;
    const auto rule = (op & 8) ? fixed[op % std::size(fixed)] : part;
    switch (op % 6) {
      case 0:
        CHECK_EQ(a.AddRuleFromString(rule), b.AddRuleFromString(rule));
        break;
      case 1:
        a.SetRulesFromString(rule);
        b.SetRulesFromString(rule);
        break;
      case 2: {
        HostMappingRules copy(a);
        HostMappingRustReference ref(b);
        a.SetRulesFromString("MAP * transient");
        b.SetRulesFromString("MAP * transient");
        a = copy;
        b = ref;
        break;
      }
      case 3: {
        const auto* ap = &a;
        const auto* bp = &b;
        a = *ap;
        b = *bp;
        break;
      }
      case 4: {
        // Input view aliases HostPortPair output; rules must own their copy.
        HostPortPair pair(std::string(rule), 80);
        CHECK_EQ(a.AddRuleFromString(pair.host()),
                 b.AddRuleFromString(pair.host()));
        pair.set_host("mutated");
        break;
      }
      default:
        if (data.find('\0') == std::string_view::npos) {
          CheckUrls(a, b);
        }
        break;
    }
    CheckRewrite(a, b, "initial.test", 123);
    CheckRewrite(a, b, "foo.com", 80);
    CheckRewrite(a, b, "::1", 80);
    // Avoid documented HostForURL DCHECK/DFATAL preconditions here; all byte
    // values (including NUL) are covered below with a hostname-only match.
    std::string host(part);
    for (char& c : host) {
      if (c == '\0' || c == '[') {
        c = 'x';
      }
    }
    CheckRewrite(a, b, host, static_cast<uint16_t>(op * 257));
  }
  if (data.find('\0') == std::string_view::npos) {
    CheckUrls(a, b);
  }
  a.SetRulesFromString("MAP * sentinel:0");
  b.SetRulesFromString("MAP * sentinel:0");
  CheckRewrite(a, b, data, 65535);
}
}  // namespace net::host_mapping_rust_test
#endif
