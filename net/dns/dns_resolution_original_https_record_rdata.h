// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_DNS_DNS_RESOLUTION_ORIGINAL_HTTPS_RECORD_RDATA_H_
#define NET_DNS_DNS_RESOLUTION_ORIGINAL_HTTPS_RECORD_RDATA_H_

#include <stdint.h>

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/containers/span.h"
#include "net/base/ip_address.h"
#include "net/base/net_export.h"
#include "net/dns/public/dns_protocol.h"
#include "net/dns/dns_resolution_original_record_rdata.h"

#include "base/component_export.h"
#include "net/dns/dns_resolution_rust_original_types.h"
#include "net/dns/dns_resolution_result_types_rust_original.h"
#include "net/base/host_port_rust_reference.h"

namespace net::dns_resolution_original {
using HostPortPair = ::net::HostPortRustReference;


using HttpsRecordPriority = uint16_t;

class AliasFormHttpsRecordRdata;
class ServiceFormHttpsRecordRdata;

class COMPONENT_EXPORT(DNS_RESOLUTION_ORIGINAL) HttpsRecordRdata : public RecordRdata {
 public:
  static const uint16_t kType = dns_protocol::kTypeHttps;

  // Returns `nullptr` on malformed input.
  static std::unique_ptr<HttpsRecordRdata> Parse(
      base::span<const uint8_t> data);

  HttpsRecordRdata(const HttpsRecordRdata& rdata) = delete;
  HttpsRecordRdata& operator=(const HttpsRecordRdata& rdata) = delete;

  ~HttpsRecordRdata() override;

  bool IsEqual(const RecordRdata* other) const override;
  virtual bool IsEqual(const HttpsRecordRdata* other) const = 0;
  uint16_t Type() const override;

  virtual bool IsAlias() const = 0;
  AliasFormHttpsRecordRdata* AsAliasForm();
  const AliasFormHttpsRecordRdata* AsAliasForm() const;
  ServiceFormHttpsRecordRdata* AsServiceForm();
  const ServiceFormHttpsRecordRdata* AsServiceForm() const;

 protected:
  HttpsRecordRdata() = default;
};

class COMPONENT_EXPORT(DNS_RESOLUTION_ORIGINAL) AliasFormHttpsRecordRdata : public HttpsRecordRdata {
 public:
  explicit AliasFormHttpsRecordRdata(std::string alias_name);
  static std::unique_ptr<AliasFormHttpsRecordRdata> Parse(
      base::span<const uint8_t> data);

  bool IsEqual(const HttpsRecordRdata* other) const override;
  bool IsAlias() const override;

  std::string_view alias_name() const { return alias_name_; }

 private:
  AliasFormHttpsRecordRdata() = default;

  const std::string alias_name_;
};

class COMPONENT_EXPORT(DNS_RESOLUTION_ORIGINAL) ServiceFormHttpsRecordRdata : public HttpsRecordRdata {
 public:
  ServiceFormHttpsRecordRdata(
      uint16_t priority,
      std::string service_name,
      base::flat_set<uint16_t> mandatory_keys,
      std::vector<std::string> alpn_ids,
      bool default_alpn,
      std::optional<uint16_t> port,
      std::vector<IPAddress> ipv4_hint,
      base::span<const uint8_t> ech_config,
      std::vector<IPAddress> ipv6_hint,
      std::vector<std::vector<uint8_t>> trust_anchor_ids);
  static std::unique_ptr<ServiceFormHttpsRecordRdata> Parse(
      base::span<const uint8_t> data);

  ~ServiceFormHttpsRecordRdata() override;

  bool IsEqual(const HttpsRecordRdata* other) const override;
  bool IsAlias() const override;

  HttpsRecordPriority priority() const { return priority_; }
  std::string_view service_name() const { return service_name_; }
  const base::flat_set<uint16_t>& mandatory_keys() const {
    return mandatory_keys_;
  }
  const std::vector<std::string>& alpn_ids() const { return alpn_ids_; }
  bool default_alpn() const { return default_alpn_; }
  std::optional<uint16_t> port() const { return port_; }
  const std::vector<IPAddress>& ipv4_hint() const { return ipv4_hint_; }
  base::span<const uint8_t> ech_config() const { return ech_config_; }
  const std::vector<IPAddress>& ipv6_hint() const { return ipv6_hint_; }
  const std::vector<std::vector<uint8_t>>& trust_anchor_ids() const {
    return trust_anchor_ids_;
  }

  // Returns whether or not this rdata parser is considered "compatible" with
  // the parsed rdata. That is that all keys listed by mandatory_keys() (and all
  // keys considered default mandatory for HTTPS records) are parsable by this
  // parser.
  bool IsCompatible() const;

 private:
  static bool IsSupportedKey(uint16_t key);

  const HttpsRecordPriority priority_;
  const std::string service_name_;

  // Supported service parameters.
  const base::flat_set<uint16_t> mandatory_keys_;
  const std::vector<std::string> alpn_ids_;
  const bool default_alpn_;
  const std::optional<uint16_t> port_;
  const std::vector<IPAddress> ipv4_hint_;
  std::vector<uint8_t> ech_config_;
  const std::vector<IPAddress> ipv6_hint_;
  std::vector<std::vector<uint8_t>> trust_anchor_ids_;
};

}  // namespace net

#endif  // NET_DNS_HTTPS_RECORD_RDATA_H_
