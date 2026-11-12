// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_BASE_PORT_POLICY_REFERENCE_H_
#define NET_BASE_PORT_POLICY_REFERENCE_H_

#include <stddef.h>
#include <stdint.h>

#include <string_view>

#include "base/component_export.h"
#include "base/containers/span.h"
#include "net/base/ip_endpoint.h"
#include "net/base/net_export.h"

namespace net::port_policy_reference {

// Checks if |port| is in the valid range (0 to 65535, though 0 is technically
// reserved).  Should be used before casting a port to a uint16_t.
COMPONENT_EXPORT(PORT_POLICY_REFERENCE)
bool IsPortValid(int port);

// Returns true if the port is in the range [0, 1023]. These ports are
// registered by IANA and typically need root access to listen on.
COMPONENT_EXPORT(PORT_POLICY_REFERENCE)
bool IsWellKnownPort(int port);

// Checks if the port is allowed for the specified scheme.  Ports set as allowed
// with SetExplicitlyAllowedPorts() or by using ScopedPortException() will be
// considered allowed for any scheme.
COMPONENT_EXPORT(PORT_POLICY_REFERENCE)
bool IsPortAllowedForScheme(int port, std::string_view url_scheme);

COMPONENT_EXPORT(PORT_POLICY_REFERENCE)
bool IsPortAllowedForIpEndpoint(const IPEndPoint& endpoint);

// Returns the number of explicitly allowed ports; for testing.
COMPONENT_EXPORT(PORT_POLICY_REFERENCE)
size_t GetCountOfExplicitlyAllowedPorts();

// Set the list of ports to be allowed that otherwise would not be. This
// replaces the list of allowed ports with the list passed to this function. An
// empty list will remove all ports. This will reset any ScopedPortExceptions
// currently active, so it's best to avoid calling this when any of those are
// active.
COMPONENT_EXPORT(PORT_POLICY_REFERENCE)
void SetExplicitlyAllowedPorts(base::span<const uint16_t> allowed_ports);

// Returns true for ports which are permitted to be passed to
// SetExplicitlyAllowedPorts(). This is not currently enforced by
// SetExplicitlyAllowedPorts() itself, as there are still callers that pass
// other ports.
COMPONENT_EXPORT(PORT_POLICY_REFERENCE)
bool IsAllowablePort(int port);

// For testing: reloads the restricted ports set by the feature flag
// `kRestrictAbusePortsOnLocalhost`.
COMPONENT_EXPORT(PORT_POLICY_REFERENCE)
void ReloadLocalhostRestrictedPortsForTesting();

class COMPONENT_EXPORT(PORT_POLICY_REFERENCE) ScopedPortException {
 public:
  explicit ScopedPortException(int port);
  ScopedPortException(const ScopedPortException&) = delete;
  ScopedPortException& operator=(const ScopedPortException&) = delete;
  ~ScopedPortException();

 private:
  int port_;
};

// Adds a port to the set permitted by GetAllowablePorts(). Cannot be nested.
class COMPONENT_EXPORT(PORT_POLICY_REFERENCE) ScopedAllowablePortForTesting {
 public:
  explicit ScopedAllowablePortForTesting(int port);
  ScopedAllowablePortForTesting(const ScopedAllowablePortForTesting&) = delete;
  ScopedAllowablePortForTesting& operator=(
      const ScopedAllowablePortForTesting&) = delete;
  ~ScopedAllowablePortForTesting();
};

}  // namespace net::port_policy_reference

#endif  // NET_BASE_PORT_POLICY_REFERENCE_H_
