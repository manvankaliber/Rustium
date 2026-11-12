// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/base/port_util.h"

#include <memory>
#include <string>
#include <string_view>

#include "base/check.h"
#include "base/feature_list.h"
#include "base/logging.h"
#include "base/metrics/field_trial_params.h"
#include "base/metrics/histogram_functions.h"
#include "base/notreached.h"
#include "net/base/features.h"
#include "net/base/ip_address.h"
#include "net/base/port_policy_ffi.h"
#include "net/base/port_policy_ffi.rs.h"

namespace net {
namespace port_policy_internal {
namespace {
const base::Feature& Feature(uint8_t kind) {
  return kind == 2 ? features::kRestrictAbusePortsOnLocalhost
                   : features::kRestrictAbusePorts;
}
const char* ParamName(uint8_t kind) {
  switch (kind) {
    case 0:
      return "restrict_ports";
    case 1:
      return "monitor_ports";
    case 2:
      return "localhost_restrict_ports";
  }
  NOTREACHED();
}
}  // namespace
bool port_feature_enabled(bool localhost) {
  return base::FeatureList::IsEnabled(Feature(localhost ? 2 : 0));
}
rust::Vec<uint8_t> port_feature_param(uint8_t kind) {
  const std::string param =
      base::GetFieldTrialParamValueByFeature(Feature(kind), ParamName(kind));
  rust::Vec<uint8_t> result;
  result.reserve(param.size());
  for (unsigned char byte : param) {
    result.push_back(byte);
  }
  return result;
}
void port_invalid_token(uint8_t kind, rust::Slice<const uint8_t> token) {
  DLOG(ERROR) << "Ignoring invalid port for " << ParamName(kind) << ": "
              << std::string_view(reinterpret_cast<const char*>(token.data()),
                                  token.size());
}
}  // namespace port_policy_internal
namespace {
bool MayConnectToLocalhost(const IPAddress& address) {
  const IPAddress unmapped = address.IsIPv4MappedIPv6()
                                 ? ConvertIPv4MappedIPv6ToIPv4(address)
                                 : address;
  return unmapped.IsLoopback() || unmapped.IsZero();
}
bool RecordDecision(uint8_t decision, const char* histogram, int port) {
  CHECK_LE(decision, 3u);
  if (decision >= 2) {
    base::UmaHistogramSparse(histogram, port);
  }
  return (decision & 1) == 0;
}
}  // namespace
bool IsPortValid(int port) {
  return port_policy_internal::port_valid(port);
}
bool IsWellKnownPort(int port) {
  return port_policy_internal::port_well_known(port);
}
bool IsPortAllowedForScheme(int port, std::string_view url_scheme) {
  return RecordDecision(port_policy_internal::port_scheme(port),
                        "Net.RestrictedPorts", port);
}
bool IsPortAllowedForIpEndpoint(const IPEndPoint& endpoint) {
  const bool enabled = port_policy_internal::port_feature_enabled(true);
  const bool local = enabled && MayConnectToLocalhost(endpoint.address());
  return RecordDecision(
      port_policy_internal::port_endpoint(endpoint.port(), enabled, local),
      "Net.RestrictedLocalhostPorts", endpoint.port());
}

size_t GetCountOfExplicitlyAllowedPorts() {
  return port_policy_internal::port_count();
}
void SetExplicitlyAllowedPorts(base::span<const uint16_t> ports) {
  port_policy_internal::port_reset(
      rust::Slice<const uint16_t>(ports.data(), ports.size()));
}
ScopedPortException::ScopedPortException(int port) : port_(port) {
  port_policy_internal::port_insert(port);
}
ScopedPortException::~ScopedPortException() {
  if (!port_policy_internal::port_erase(port_)) {
    NOTREACHED();
  }
}
bool IsAllowablePort(int port) {
  return port_policy_internal::port_allowable(port);
}
ScopedAllowablePortForTesting::ScopedAllowablePortForTesting(int port) {
  DCHECK_EQ(port_policy_internal::port_scoped_allowable(), 0);
  port_policy_internal::port_set_allowable(port);
}
ScopedAllowablePortForTesting::~ScopedAllowablePortForTesting() {
  port_policy_internal::port_set_allowable(0);
}
void ReloadLocalhostRestrictedPortsForTesting() {
  port_policy_internal::port_reload_localhost();
}
}  // namespace net
