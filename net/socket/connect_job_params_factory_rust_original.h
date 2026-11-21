// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_SOCKET_CONNECT_JOB_PARAMS_FACTORY_RUST_ORIGINAL_H_
#define NET_SOCKET_CONNECT_JOB_PARAMS_FACTORY_RUST_ORIGINAL_H_

#include <memory>
#include <optional>
#include <variant>
#include <vector>

#include "net/base/host_port_pair.h"
#include "net/base/network_anonymization_key.h"
#include "net/base/network_handle.h"
#include "net/base/privacy_mode.h"
#include "net/base/request_priority.h"
#include "net/dns/public/secure_dns_policy.h"
#include "net/http/http_proxy_connect_job_rust_original.h"
#include "net/socket/connect_job_rust_original.h"
#include "net/socket/connect_job_factory_rust_original.h"
#include "net/socket/connect_job_params_rust_original.h"
#include "net/socket/socks_connect_job_rust_original.h"
#include "net/socket/ssl_connect_job_rust_original.h"
#include "net/socket/transport_connect_job_rust_original.h"

namespace net {

class NetworkAnonymizationKey;
struct NetworkTrafficAnnotationTag;
class ProxyChain;
struct SSLConfig;

ConnectJobParamsRustOriginal ConstructConnectJobParamsRustOriginal(
    const ConnectJobFactoryRustOriginal::Endpoint& endpoint,
    const ProxyChain& proxy_chain,
    const std::optional<NetworkTrafficAnnotationTag>& proxy_annotation_tag,
    const std::vector<SSLConfig::CertAndStatus>& allowed_bad_certs,
    ConnectJobFactoryRustOriginal::AlpnMode alpn_mode,
    bool force_tunnel,
    PrivacyMode privacy_mode,
    const ConnectJobRustOriginalHostResolutionCallback& resolution_callback,
    const NetworkAnonymizationKey& network_anonymization_key,
    SecureDnsPolicy secure_dns_policy,
    bool disable_cert_network_fetches,
    const CommonConnectJobParamsRustOriginal* common_connect_job_params,
    const NetworkAnonymizationKey& proxy_dns_network_anonymization_key,
    handles::NetworkHandle target_network);

}  // namespace net

#endif  // NET_SOCKET_CONNECT_JOB_PARAMS_FACTORY_RUST_ORIGINAL_H_
