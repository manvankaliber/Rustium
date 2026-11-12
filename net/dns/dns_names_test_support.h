// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_DNS_DNS_NAMES_TEST_SUPPORT_H_
#define NET_DNS_DNS_NAMES_TEST_SUPPORT_H_

#include "base/check.h"
#include "base/strings/string_view_util.h"
#include "net/dns/dns_names_reference.h"
#include "net/dns/dns_names_util.h"

namespace net::dns_names_test {
inline void Check(base::span<const uint8_t> input, bool services = true) {
  const auto text = base::as_string_view(input);
  CHECK(dns_names_util::DottedNameToNetwork(text) ==
        dns_names_reference::DottedNameToNetwork(text));
  CHECK(dns_names_util::IsValidDnsName(text) ==
        dns_names_reference::IsValidDnsName(text));
  const auto middle = text.size() / 2;
  CHECK(dns_names_util::DomainNameComparator()(text.substr(0, middle),
                                               text.substr(middle)) ==
        dns_names_reference::DomainNameComparator()(text.substr(0, middle),
                                                    text.substr(middle)));
  if (services) {
    CHECK(dns_names_util::DottedNameToNetwork(text, true) ==
          dns_names_reference::DottedNameToNetwork(text, true));
    CHECK(dns_names_util::IsValidDnsRecordName(text) ==
          dns_names_reference::IsValidDnsRecordName(text));
    CHECK(dns_names_util::UrlCanonicalizeNameIfAble(text) ==
          dns_names_reference::UrlCanonicalizeNameIfAble(text));
  }
  for (bool complete : {false, true}) {
    CHECK(dns_names_util::NetworkToDottedName(input, complete) ==
          dns_names_reference::NetworkToDottedName(input, complete));
    base::SpanReader a(input), b(input);
    CHECK(dns_names_util::NetworkToDottedName(a, complete) ==
          dns_names_reference::NetworkToDottedName(b, complete));
    CHECK(a.num_read() == b.num_read());
    CHECK(a.remaining_span().data() == b.remaining_span().data());
  }
  for (bool wide : {false, true}) {
    base::SpanReader a(input), b(input);
    // Nonempty sentinel catches accidental writes on failure.
    base::span<const uint8_t> x = input, y = input;
    const bool ok = wide ? dns_names_util::ReadU16LengthPrefixed(a, &x)
                         : dns_names_util::ReadU8LengthPrefixed(a, &x);
    const bool ref = wide ? dns_names_reference::ReadU16LengthPrefixed(b, &y)
                          : dns_names_reference::ReadU8LengthPrefixed(b, &y);
    CHECK(ok == ref);
    CHECK(x.data() == y.data());
    CHECK(x.size() == y.size());
    CHECK(a.num_read() == b.num_read());
    CHECK(a.remaining_span().data() == b.remaining_span().data());
  }
}
}  // namespace net::dns_names_test

#endif  // NET_DNS_DNS_NAMES_TEST_SUPPORT_H_
