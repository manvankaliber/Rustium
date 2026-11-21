// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_BASE_HOST_PORT_RUST_REFERENCE_H_
#define NET_BASE_HOST_PORT_RUST_REFERENCE_H_

#include <stdint.h>

#include <optional>
#include <string>
#include <string_view>
#include <tuple>

#include "base/component_export.h"
#include "base/values.h"
#include "net/base/net_export.h"

class GURL;

namespace url {
class SchemeHostPort;
}  // namespace url

namespace net {

class IPEndPoint;

// Frozen local policy; URL parsing primitives remain shared services.
COMPONENT_EXPORT(HOST_PORT_RUST_REFERENCE)
bool HostPortRustOriginalParseHostAndPort(std::string_view input,
                                          std::string* host,
                                          int* port);

class COMPONENT_EXPORT(HOST_PORT_RUST_REFERENCE) HostPortRustReference {
 public:
  HostPortRustReference();
  // If |in_host| represents an IPv6 address, it should not bracket the address.
  HostPortRustReference(std::string_view in_host, uint16_t in_port);
  HostPortRustReference(const char* in_host, uint16_t in_port);
  HostPortRustReference(std::string&& in_host, uint16_t in_port);

  // Creates a HostPortRustReference for the origin of |url|.
  static HostPortRustReference FromURL(const GURL& url);

  static HostPortRustReference FromSchemeHostPort(
      const url::SchemeHostPort& scheme_host_port);

  // Creates a HostPortRustReference from an IPEndPoint.
  static HostPortRustReference FromIPEndPoint(const IPEndPoint& ipe);

  // Creates a HostPortRustReference from a string formatted in same manner as
  // ToString().
  static HostPortRustReference FromString(std::string_view str);

  // Nullopt if `value` is malformed to be deserialized to
  // HostPortRustReference.
  static std::optional<HostPortRustReference> FromValue(
      const base::Value& value);

  friend bool operator==(const HostPortRustReference&,
                         const HostPortRustReference&) = default;
  friend auto operator<=>(const HostPortRustReference&,
                          const HostPortRustReference&) = default;

  bool IsEmpty() const { return host_.empty() && port_ == 0; }

  const std::string& host() const { return host_; }

  uint16_t port() const { return port_; }

  void set_host(std::string_view in_host) { host_ = in_host; }

  void set_port(uint16_t in_port) { port_ = in_port; }

  // ToString() will convert the HostPortRustReference to "host:port".  If
  // |host_| is an IPv6 literal, it will add brackets around |host_|.
  std::string ToString() const;

  // Returns |host_|, adding IPv6 brackets if needed.
  std::string HostForURL() const;

  base::Value ToValue() const;

 private:
  uint16_t port_;
  // If |host_| represents an IPv6 address, this string will not contain
  // brackets around the address.
  std::string host_;
};

}  // namespace net

#endif  // NET_BASE_HOST_PORT_RUST_REFERENCE_H_
