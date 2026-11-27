// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/raw_ptr.h"
#include "net/socket/connect_job_factory.h"
#include "net/socket/connect_job_factory_rust_ffi.h"
#include "net/socket/connection_jobs_upper_rust_ffi.h"
#include "base/notreached.h"

#include <memory>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

#include "base/check.h"
#include "base/memory/scoped_refptr.h"
#include "net/base/host_port_pair.h"
#include "net/base/network_anonymization_key.h"
#include "net/base/network_handle.h"
#include "net/base/privacy_mode.h"
#include "net/base/proxy_chain.h"
#include "net/base/request_priority.h"
#include "net/dns/public/secure_dns_policy.h"
#include "net/http/http_proxy_connect_job.h"
#include "net/socket/connect_job.h"
#include "net/socket/connect_job_params_factory.h"
#include "net/socket/socket_tag.h"
#include "net/socket/socks_connect_job.h"
#include "net/socket/ssl_connect_job.h"
#include "net/socket/transport_connect_job.h"
#include "net/ssl/ssl_config.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "url/scheme_host_port.h"

namespace net {

namespace {

template <typename T>
std::unique_ptr<T> CreateFactoryIfNull(std::unique_ptr<T> in) {
  if (connection_jobs_rust_factory_has_input(!!in)) {
    return in;
  }
  return std::make_unique<T>();
}

}  // namespace


// Invocation-local observer. Invalidated at the most-derived destructor entry.
// No typed native pointer crosses the Rust ABI or survives a service call.
struct connect_job_factoryRustBridge : connection_jobs_upper_rust::Invocation {
  using Op = connect_job_factory_rust::Operation;
  using Action = connect_job_factory_rust::Action;
  raw_ptr<const ConnectJobFactory> owner;
  raw_ptr<ConnectJobFactory> mutable_owner = nullptr;

  raw_ptr<ConnectJobParams> params = nullptr;
  raw_ptr<std::unique_ptr<ConnectJob>> output = nullptr;
  RequestPriority priority = DEFAULT_PRIORITY;
  raw_ptr<const SocketTag> tag = nullptr;
  raw_ptr<const CommonConnectJobParams> common = nullptr;
  raw_ptr<ConnectJob::Delegate> delegate = nullptr;

  explicit connect_job_factoryRustBridge(ConnectJobFactory* value) : Invocation(value), owner(value), mutable_owner(value) {
    Register(&Clear);
  }
  explicit connect_job_factoryRustBridge(const ConnectJobFactory* value) : Invocation(value), owner(value) {
    Register(&Clear);
  }
  ~connect_job_factoryRustBridge() { Unregister(); }
  static void Clear(Invocation* value) {
    auto& c = *static_cast<connect_job_factoryRustBridge*>(value);
    c.owner = nullptr; c.mutable_owner = nullptr;
    
  }
  int64_t Run(Op operation, int64_t argument = 0) {
    return connection_jobs_rust_run(3, static_cast<uint32_t>(operation),
                                    argument, this, &Service);
  }
  static bool Service(void* context, uint32_t module, uint32_t action,
                      [[maybe_unused]] int64_t argument, int64_t* output) {
    auto& c = *static_cast<connect_job_factoryRustBridge*>(context);
    *output = 0;
    if (!c.owner) return false;
    if (action == UINT32_MAX) return true;
    CHECK_EQ(module, 3u);
    const auto& j = *c.owner;
    // Mutable access is requested only in action arms that require it. Const
    // wrappers never obtain a mutable receiver and never commit a snapshot.
    switch (static_cast<Action>(action)) {
      case Action::IsSsl: { *output = c.params->is_ssl(); break; }
      case Action::IsTransport: { *output = c.params->is_transport(); break; }
      case Action::IsHttp: { *output = c.params->is_http_proxy(); break; }
      case Action::CheckSocks: { CHECK(c.params->is_socks()); break; }
      case Action::Ssl: { *c.output = j.ssl_connect_job_factory_->Create(c.priority, *c.tag,
 c.common, c.params->take_ssl(), c.delegate, nullptr); break; }
      case Action::Transport: { *c.output = j.transport_connect_job_factory_->Create(c.priority, *c.tag,
 c.common, c.params->take_transport(), c.delegate, nullptr); break; }
      case Action::Http: { *c.output = j.http_proxy_connect_job_factory_->Create(c.priority, *c.tag,
 c.common, c.params->take_http_proxy(), c.delegate, nullptr); break; }
      case Action::Socks: { *c.output = j.socks_connect_job_factory_->Create(c.priority, *c.tag,
 c.common, c.params->take_socks(), c.delegate, nullptr); break; }

      default: NOTREACHED();
    }
    return c.owner != nullptr;
  }
};
ConnectJobFactory::ConnectJobFactory(
    std::unique_ptr<HttpProxyConnectJob::Factory>
        http_proxy_connect_job_factory,
    std::unique_ptr<SOCKSConnectJob::Factory> socks_connect_job_factory,
    std::unique_ptr<SSLConnectJob::Factory> ssl_connect_job_factory,
    std::unique_ptr<TransportConnectJob::Factory> transport_connect_job_factory)
    : http_proxy_connect_job_factory_(
          CreateFactoryIfNull(std::move(http_proxy_connect_job_factory))),
      socks_connect_job_factory_(
          CreateFactoryIfNull(std::move(socks_connect_job_factory))),
      ssl_connect_job_factory_(
          CreateFactoryIfNull(std::move(ssl_connect_job_factory))),
      transport_connect_job_factory_(
          CreateFactoryIfNull(std::move(transport_connect_job_factory))) {}

ConnectJobFactory::~ConnectJobFactory() {
  connect_job_factoryRustBridge::Invalidate(this);
}

std::unique_ptr<ConnectJob> ConnectJobFactory::CreateConnectJob(
    url::SchemeHostPort endpoint,
    const ProxyChain& proxy_chain,
    const std::optional<NetworkTrafficAnnotationTag>& proxy_annotation_tag,
    const std::vector<SSLConfig::CertAndStatus>& allowed_bad_certs,
    ConnectJobFactory::AlpnMode alpn_mode,
    bool force_tunnel,
    PrivacyMode privacy_mode,
    const OnHostResolutionCallback& resolution_callback,
    RequestPriority request_priority,
    SocketTag socket_tag,
    const NetworkAnonymizationKey& network_anonymization_key,
    SecureDnsPolicy secure_dns_policy,
    bool disable_cert_network_fetches,
    const CommonConnectJobParams* common_connect_job_params,
    handles::NetworkHandle target_network,
    ConnectJob::Delegate* delegate) const {
  return CreateConnectJob(Endpoint(std::move(endpoint)), proxy_chain,
                          proxy_annotation_tag, allowed_bad_certs, alpn_mode,
                          force_tunnel, privacy_mode, resolution_callback,
                          request_priority, socket_tag,
                          network_anonymization_key, secure_dns_policy,
                          disable_cert_network_fetches,
                          common_connect_job_params, target_network, delegate);
}

std::unique_ptr<ConnectJob> ConnectJobFactory::CreateConnectJob(
    bool using_ssl,
    HostPortPair endpoint,
    const ProxyChain& proxy_chain,
    const std::optional<NetworkTrafficAnnotationTag>& proxy_annotation_tag,
    bool force_tunnel,
    PrivacyMode privacy_mode,
    const OnHostResolutionCallback& resolution_callback,
    RequestPriority request_priority,
    SocketTag socket_tag,
    const NetworkAnonymizationKey& network_anonymization_key,
    SecureDnsPolicy secure_dns_policy,
    const CommonConnectJobParams* common_connect_job_params,
    handles::NetworkHandle target_network,
    ConnectJob::Delegate* delegate) const {
  SchemelessEndpoint schemeless_endpoint{using_ssl, std::move(endpoint)};
  return CreateConnectJob(
      std::move(schemeless_endpoint), proxy_chain, proxy_annotation_tag,
      /*allowed_bad_certs=*/{}, ConnectJobFactory::AlpnMode::kDisabled,
      force_tunnel, privacy_mode, resolution_callback, request_priority,
      socket_tag, network_anonymization_key, secure_dns_policy,
      /*disable_cert_network_fetches=*/false, common_connect_job_params,
      target_network, delegate);
}

std::unique_ptr<ConnectJob> ConnectJobFactory::CreateConnectJob(
    Endpoint endpoint,
    const ProxyChain& proxy_chain,
    const std::optional<NetworkTrafficAnnotationTag>& proxy_annotation_tag,
    const std::vector<SSLConfig::CertAndStatus>& allowed_bad_certs,
    ConnectJobFactory::AlpnMode alpn_mode,
    bool force_tunnel,
    PrivacyMode privacy_mode,
    const OnHostResolutionCallback& resolution_callback,
    RequestPriority request_priority,
    SocketTag socket_tag,
    const NetworkAnonymizationKey& network_anonymization_key,
    SecureDnsPolicy secure_dns_policy,
    bool disable_cert_network_fetches,
    const CommonConnectJobParams* common_connect_job_params,
    handles::NetworkHandle target_network,
    ConnectJob::Delegate* delegate) const {
  ConnectJobParams connect_job_params = ConstructConnectJobParams(
      endpoint, proxy_chain, proxy_annotation_tag, allowed_bad_certs, alpn_mode,
      force_tunnel, privacy_mode, resolution_callback,
      network_anonymization_key, secure_dns_policy,
      disable_cert_network_fetches, common_connect_job_params,
      proxy_dns_network_anonymization_key_, target_network);

  std::unique_ptr<ConnectJob> result;
  connect_job_factoryRustBridge c(this);
  c.params = &connect_job_params; c.output = &result; c.priority = request_priority;
  c.tag = &socket_tag; c.common = common_connect_job_params; c.delegate = delegate;
  c.Run(connect_job_factoryRustBridge::Op::Create);
  return result;
}

}  // namespace net
