// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <array>
#include <memory>
#include <vector>

#include "base/test/metrics/histogram_tester.h"
#include "base/test/scoped_feature_list.h"
#include "net/base/features.h"
#include "net/base/ip_address.h"
#include "net/base/port_policy_reference.h"
#include "net/base/port_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {
namespace ref = port_policy_reference;
TEST(PortPolicyRust, StatefulDifferential) {
  base::test::ScopedFeatureList features;
  features.InitWithFeaturesAndParameters(
      {{features::kRestrictAbusePorts,
        {{"restrict_ports", "444,445,01, 2147483648,0"},
         {"monitor_ports", "445,446"}}},
       {features::kRestrictAbusePortsOnLocalhost,
        {{"localhost_restrict_ports", "444,445,01"}}}},
      {});
  SetExplicitlyAllowedPorts({});
  ref::SetExplicitlyAllowedPorts({});
  ReloadLocalhostRestrictedPortsForTesting();
  ref::ReloadLocalhostRestrictedPortsForTesting();
  const std::array<int, 15> ports = {-1,   0,     1,     21,    22,
                                     443,  444,   445,   446,   1023,
                                     1024, 10080, 65535, 65536, 2147483647};
  for (int p : ports) {
    EXPECT_EQ(IsPortValid(p), ref::IsPortValid(p));
    EXPECT_EQ(IsWellKnownPort(p), ref::IsWellKnownPort(p));
    EXPECT_EQ(IsAllowablePort(p), ref::IsAllowablePort(p));
    EXPECT_EQ(IsPortAllowedForScheme(p, "https"),
              ref::IsPortAllowedForScheme(p, "https"));
  }
  const IPAddress addresses[] = {
      IPAddress::IPv4Localhost(),
      IPAddress::IPv4AllZeros(),
      IPAddress::IPv6Localhost(),
      IPAddress::IPv6AllZeros(),
      IPAddress(8, 8, 8, 8),
      ConvertIPv4ToIPv4MappedIPv6(IPAddress::IPv4Localhost()),
      ConvertIPv4ToIPv4MappedIPv6(IPAddress::IPv4AllZeros()),
      ConvertIPv4ToIPv4MappedIPv6(IPAddress(8, 8, 8, 8))};
  for (const auto& address : addresses) {
    for (uint16_t p : {uint16_t{444}, uint16_t{445}, uint16_t{446}}) {
      IPEndPoint ep(address, p);
      EXPECT_EQ(IsPortAllowedForIpEndpoint(ep),
                ref::IsPortAllowedForIpEndpoint(ep));
    }
  }
  {
    ScopedAllowablePortForTesting actual(444);
    ref::ScopedAllowablePortForTesting baseline(444);
    EXPECT_EQ(IsAllowablePort(444), ref::IsAllowablePort(444));
    EXPECT_EQ(IsAllowablePort(0), ref::IsAllowablePort(0));
  }
  {
    ScopedPortException actual(22);
    ref::ScopedPortException baseline(22);
    const std::array<uint16_t, 2> replacements = {22, 22};
    SetExplicitlyAllowedPorts(replacements);
    ref::SetExplicitlyAllowedPorts(replacements);
  }
  EXPECT_EQ(GetCountOfExplicitlyAllowedPorts(),
            ref::GetCountOfExplicitlyAllowedPorts());
  SetExplicitlyAllowedPorts({});
  ref::SetExplicitlyAllowedPorts({});
  {
    base::HistogramTester hist;
    EXPECT_EQ(IsPortAllowedForScheme(445, "https"),
              ref::IsPortAllowedForScheme(445, "https"));
    EXPECT_EQ(hist.GetBucketCount("Net.RestrictedPorts", 445), 2);
    EXPECT_EQ(IsPortAllowedForScheme(446, "https"),
              ref::IsPortAllowedForScheme(446, "https"));
    EXPECT_EQ(hist.GetBucketCount("Net.RestrictedPorts", 446), 2);
    {
      ScopedPortException actual(445);
      ref::ScopedPortException baseline(445);
      EXPECT_EQ(IsPortAllowedForScheme(445, "https"),
                ref::IsPortAllowedForScheme(445, "https"));
      EXPECT_EQ(hist.GetBucketCount("Net.RestrictedPorts", 445), 2);
    }
    IPEndPoint ep(IPAddress::IPv4Localhost(), 444);
    EXPECT_EQ(IsPortAllowedForIpEndpoint(ep),
              ref::IsPortAllowedForIpEndpoint(ep));
    EXPECT_EQ(hist.GetBucketCount("Net.RestrictedLocalhostPorts", 444), 2);
  }
  uint32_t seed = 0x12345678;
  auto next = [&] {
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    return seed;
  };
  // Each iteration is a bounded multi-operation sequence, with duplicate
  // inserts, reset, nested exceptions, and queries against both independent
  // states.
  for (int iteration = 0; iteration < 1000000; ++iteration) {
    uint16_t a = static_cast<uint16_t>(next()),
             b = (next() & 1) ? a : static_cast<uint16_t>(next());
    std::array<uint16_t, 3> initial = {a, a, b};
    SetExplicitlyAllowedPorts(initial);
    ref::SetExplicitlyAllowedPorts(initial);
    {
      ScopedPortException x(a), y(b);
      ref::ScopedPortException rx(a), ry(b);
      ASSERT_EQ(GetCountOfExplicitlyAllowedPorts(),
                ref::GetCountOfExplicitlyAllowedPorts());
      ASSERT_EQ(IsPortAllowedForScheme(a, "https"),
                ref::IsPortAllowedForScheme(a, "https"));
      ASSERT_EQ(IsPortAllowedForScheme(b, "http"),
                ref::IsPortAllowedForScheme(b, "http"));
    }
    ASSERT_EQ(GetCountOfExplicitlyAllowedPorts(),
              ref::GetCountOfExplicitlyAllowedPorts());
    SetExplicitlyAllowedPorts({});
    ref::SetExplicitlyAllowedPorts({});
  }
}
}  // namespace net
