// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <cstddef>
#include <cstdint>
#include <vector>

#include "base/check.h"
#include "net/base/port_policy_reference.h"
#include "net/base/port_util.h"
namespace ref = net::port_policy_reference;
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  std::vector<uint16_t> ports;
  for (size_t i = 0; i + 1 < size && ports.size() < 24; i += 2) {
    ports.push_back(static_cast<uint16_t>((data[i] << 8) | data[i + 1]));
  }
  net::SetExplicitlyAllowedPorts({});
  ref::SetExplicitlyAllowedPorts({});
  for (uint16_t port : ports) {
    net::ScopedPortException actual(port);
    ref::ScopedPortException baseline(port);
    CHECK_EQ(net::GetCountOfExplicitlyAllowedPorts(),
             ref::GetCountOfExplicitlyAllowedPorts());
    CHECK_EQ(net::IsPortAllowedForScheme(port, "https"),
             ref::IsPortAllowedForScheme(port, "https"));
    CHECK_EQ(net::IsPortValid(port), ref::IsPortValid(port));
    CHECK_EQ(net::IsWellKnownPort(port), ref::IsWellKnownPort(port));
  }
  net::SetExplicitlyAllowedPorts(ports);
  ref::SetExplicitlyAllowedPorts(ports);
  CHECK_EQ(net::GetCountOfExplicitlyAllowedPorts(),
           ref::GetCountOfExplicitlyAllowedPorts());
  for (uint16_t port : ports) {
    CHECK_EQ(net::IsPortAllowedForScheme(port, "https"),
             ref::IsPortAllowedForScheme(port, "https"));
  }
  net::SetExplicitlyAllowedPorts({});
  ref::SetExplicitlyAllowedPorts({});
  return 0;
}
