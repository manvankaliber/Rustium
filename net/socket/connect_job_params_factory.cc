// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/raw_ref.h"
#include "base/memory/raw_ptr.h"
#include "net/socket/connect_job_params_factory.h"
#include "net/socket/connect_job_params_factory_rust_ffi.h"
#include "base/notreached.h"

#include <optional>
#include <variant>
#include <vector>

#include "base/check.h"
#include "base/containers/flat_set.h"
#include "base/feature_list.h"
#include "base/memory/scoped_refptr.h"
#include "base/strings/string_util.h"
#include "net/base/features.h"
#include "net/base/host_port_pair.h"
#include "net/base/network_anonymization_key.h"
#include "net/base/privacy_mode.h"
#include "net/base/proxy_chain.h"
#include "net/base/proxy_server.h"
#include "net/base/request_priority.h"
#include "net/dns/public/secure_dns_policy.h"
#include "net/http/http_proxy_connect_job.h"
#include "net/socket/connect_job_params.h"
#include "net/socket/next_proto.h"
#include "net/socket/socket_tag.h"
#include "net/socket/socks_connect_job.h"
#include "net/socket/ssl_connect_job.h"
#include "net/socket/transport_connect_job.h"
#include "net/ssl/ssl_config.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "url/gurl.h"
#include "url/scheme_host_port.h"

namespace net {

namespace {

// Populates `ssl_config's` ALPN-related fields. Namely, `alpn_protos`,
// `application_settings`, `renego_allowed_default`, and
// `renego_allowed_for_protos`.
//
// In the case of `AlpnMode::kDisabled`, clears all of the fields.
//
// In the case of `AlpnMode::kHttp11Only`, sets `alpn_protos` to only allow
// HTTP/1.1 negotiation.
//
// In the case of `AlpnMode::kHttpAll`, copies `alpn_protos` from
// `common_connect_job_params`, and gives `HttpServerProperties` a chance to
// force use of HTTP/1.1 only.
//
// If `alpn_mode` is not `AlpnMode::kDisabled`, then `server` must be a
// `SchemeHostPort`, as it makes no sense to negotiate ALPN when the scheme
// isn't known.
HostPortPair ToHostPortPair(const ConnectJobFactory::Endpoint& endpoint) {
  if (std::holds_alternative<url::SchemeHostPort>(endpoint)) {
    return HostPortPair::FromSchemeHostPort(
        std::get<url::SchemeHostPort>(endpoint));
  }

  DCHECK(
      std::holds_alternative<ConnectJobFactory::SchemelessEndpoint>(endpoint));
  return std::get<ConnectJobFactory::SchemelessEndpoint>(endpoint)
      .host_port_pair;
}

ConnectJobParams MakeSSLSocketParams(
    ConnectJobParams params,
    const HostPortPair& host_and_port,
    const SSLConfig& ssl_config,
    const NetworkAnonymizationKey& network_anonymization_key) {
  return ConnectJobParams(base::MakeRefCounted<SSLSocketParams>(
      std::move(params), host_and_port, ssl_config, network_anonymization_key));
}

}  // namespace


struct ParamsRustBridge {
 static_assert(static_cast<int>(ConnectJobFactory::AlpnMode::kDisabled)==0);
 static_assert(static_cast<int>(ConnectJobFactory::AlpnMode::kHttp11Only)==1);
 static_assert(static_cast<int>(ConnectJobFactory::AlpnMode::kHttpAll)==2);

 raw_ref<const ConnectJobFactory::Endpoint> endpoint;
 raw_ref<const ProxyChain> chain;
 raw_ref<const std::optional<NetworkTrafficAnnotationTag>> annotation;
 raw_ref<const std::vector<SSLConfig::CertAndStatus>> allowed_bad_certs;
 ConnectJobFactory::AlpnMode alpn_mode;
 bool force_tunnel;
 PrivacyMode privacy;
 raw_ref<const OnHostResolutionCallback> callback;
 raw_ref<const NetworkAnonymizationKey> endpoint_nak;
 SecureDnsPolicy secure_dns;
 bool disable_fetch;
 raw_ptr<const CommonConnectJobParams> common;
 raw_ref<const NetworkAnonymizationKey> proxy_nak;
 handles::NetworkHandle network;
 struct NativeFrame {
   HostPortPair host;
   SSLConfig config;
   ConnectJobParams params;
   raw_ptr<const NetworkAnonymizationKey> nak=nullptr;
   url::SchemeHostPort alpn_endpoint;
   size_t index=0;
 };
 // Values only; safe Rust selects recursion/indexing, C++ owns native types.
 std::vector<NativeFrame> frames;
 NetworkAnonymizationKey empty_nak;
 std::optional<HostPortPair> next_host;
 std::optional<TransportSocketParams::Endpoint> transport_endpoint;
 base::flat_set<std::string> protocols;
 ConnectJobParams output;
 NativeFrame& Frame(){return frames.back();}
 const ProxyServer& Proxy(){return chain->GetProxyServer(Frame().index);}
 static bool Service(void* context,uint32_t module,uint32_t action,
                     int64_t argument,int64_t* output){
  auto& c=*static_cast<ParamsRustBridge*>(context);*output=0;CHECK_EQ(module,11u);
  using Action=connect_job_params_factory_rust::Action;
  switch(static_cast<Action>(action)){
      case Action::CheckValid: { DCHECK(c.chain.get().IsValid()); break; }
      case Action::PushRoot: { c.frames.emplace_back(); c.Frame().nak=&c.endpoint_nak.get(); break; }
      case Action::DestinationScheme: { *output=std::holds_alternative<url::SchemeHostPort>(c.endpoint.get()); break; }
      case Action::SchemeCrypto: { *output=GURL::SchemeIsCryptographic(base::ToLowerASCII(std::get<url::SchemeHostPort>(c.endpoint.get()).scheme())); break; }
      case Action::SchemelessSsl: { DCHECK(std::holds_alternative<ConnectJobFactory::SchemelessEndpoint>(c.endpoint.get())); *output=std::get<ConnectJobFactory::SchemelessEndpoint>(c.endpoint.get()).using_ssl; break; }
      case Action::EndpointAlpnMode: { *output=static_cast<int>(c.alpn_mode); break; }
      case Action::AllowedCerts: { c.Frame().config.allowed_bad_certs=c.allowed_bad_certs.get(); break; }
      case Action::Privacy: { c.Frame().config.privacy_mode=c.privacy; break; }
      case Action::SetDestinationAlpn: { DCHECK(std::holds_alternative<url::SchemeHostPort>(c.endpoint.get())); c.Frame().alpn_endpoint=std::get<url::SchemeHostPort>(c.endpoint.get()); break; }
      case Action::ClearAlpn: { c.Frame().config.alpn_protos={}; c.Frame().config.application_settings={}; c.Frame().config.renego_allowed_default=false; break; }
      case Action::AlpnHttp11: { c.Frame().config.alpn_protos={NextProto::kProtoHTTP11}; break; }
      case Action::AlpnAll: { c.Frame().config.alpn_protos=*c.common->alpn_protos; break; }
      case Action::AlpnApplications: { c.Frame().config.application_settings=*c.common->application_settings; break; }
      case Action::CheckAll: { DCHECK_EQ(argument,static_cast<int>(ConnectJobFactory::AlpnMode::kHttpAll)); break; }
      case Action::HasProperties: { *output=c.common->http_server_properties!=nullptr; break; }
      case Action::MaybeForceHttp11: { c.common->http_server_properties->MaybeForceHTTP11(c.Frame().alpn_endpoint,*c.Frame().nak,&c.Frame().config); break; }
      case Action::RenegoDefault: { c.Frame().config.renego_allowed_default=argument!=0; break; }
      case Action::RenegoHttp11: { c.Frame().config.renego_allowed_for_protos={NextProto::kProtoHTTP11}; break; }
      case Action::DestinationCertFetch: { c.Frame().config.disable_cert_verification_network_fetches=c.disable_fetch; break; }
      case Action::EarlyData: { c.Frame().config.early_data_enabled=*c.common->enable_early_data; break; }
      case Action::DestinationUsage: { c.Frame().config.proxy_chain=c.chain.get(); c.Frame().config.proxy_chain_index=c.chain.get().length(); c.Frame().config.session_usage=SessionUsage::kDestination; break; }
      case Action::Direct: { *output=c.chain.get().is_direct(); break; }
      case Action::ForceTunnel: { *output=c.force_tunnel; break; }
      case Action::GetToProxyAllowed: { *output=c.chain.get().is_get_to_proxy_allowed(); break; }
      case Action::ChainLength: { *output=c.chain.get().length(); break; }
      case Action::EndpointPair: { c.next_host=ToHostPortPair(c.endpoint.get()); break; }
      case Action::SchemeTransport: { c.transport_endpoint=std::get<url::SchemeHostPort>(c.endpoint.get()); break; }
      case Action::PairTransport: { c.transport_endpoint=std::get<ConnectJobFactory::SchemelessEndpoint>(c.endpoint.get()).host_port_pair; break; }
      case Action::BeginProtocols: { c.protocols.clear(); break; }
      case Action::ProtocolCount: { *output=c.Frame().config.alpn_protos.size(); break; }
      case Action::InsertProtocol: { c.protocols.insert(NextProtoToString(c.Frame().config.alpn_protos[static_cast<size_t>(argument)])); break; }
      case Action::BuildDirect: { c.Frame().params=ConnectJobParams(base::MakeRefCounted<TransportSocketParams>(*c.transport_endpoint,c.endpoint_nak.get(),c.secure_dns,c.network,c.callback.get(),std::move(c.protocols))); break; }
      case Action::PushProxy: { c.frames.emplace_back(); c.Frame().index=static_cast<size_t>(argument); c.Frame().host=std::move(*c.next_host); break; }
      case Action::EmptyNak: { c.Frame().nak=&c.empty_nak; break; }
      case Action::EndpointNak: { c.Frame().nak=&c.endpoint_nak.get(); break; }
      case Action::ProxySecure: { *output=c.Proxy().is_secure_http_like(); break; }
      case Action::ProxyQuic: { *output=c.Proxy().is_quic(); break; }
      case Action::ProxyHttp: { *output=c.Proxy().is_http_like(); break; }
      case Action::ProxySocks: { DCHECK(c.Proxy().is_socks()); DCHECK_EQ(c.chain.get().length(),1u); break; }
      case Action::ProxySocks5: { *output=c.Proxy().scheme()==ProxyServer::SCHEME_SOCKS5; break; }
      case Action::CheckProxyQuicAt: { CHECK(c.chain.get().GetProxyServer(static_cast<size_t>(argument)).is_quic()); break; }
      case Action::ProxyDisableFetch: { c.Frame().config.disable_cert_verification_network_fetches=true; break; }
      case Action::ProxyAlpnEndpoint: { c.Frame().alpn_endpoint=url::SchemeHostPort(url::kHttpsScheme,c.Proxy().host_port_pair().host(),c.Proxy().host_port_pair().port()); break; }
      case Action::ProxyUsage: { c.Frame().config.proxy_chain=c.chain.get(); c.Frame().config.proxy_chain_index=c.Frame().index; c.Frame().config.session_usage=SessionUsage::kProxy; break; }
      case Action::ProxyTransport: { c.Frame().params=ConnectJobParams(base::MakeRefCounted<TransportSocketParams>(c.Proxy().host_port_pair(),c.proxy_nak.get(),c.secure_dns,c.network,c.callback.get(),std::move(c.protocols))); break; }
      case Action::NextProxyHost: { c.next_host=c.Proxy().host_port_pair(); break; }
      case Action::WrapProxySsl: { c.Frame().params=MakeSSLSocketParams(std::move(c.Frame().params),c.Proxy().host_port_pair(),c.Frame().config,*c.Frame().nak); break; }
      case Action::BuildQuic: { c.Frame().params=ConnectJobParams(base::MakeRefCounted<HttpProxySocketParams>(std::move(c.Frame().config),c.Frame().host,c.chain.get(),c.Frame().index,argument!=0,*c.annotation.get(),*c.Frame().nak,c.secure_dns,c.network)); break; }
      case Action::CheckNotQuic: { CHECK(!c.Proxy().is_quic()); break; }
      case Action::BuildHttp: { c.Frame().params=ConnectJobParams(base::MakeRefCounted<HttpProxySocketParams>(std::move(c.Frame().params),c.Frame().host,c.chain.get(),c.Frame().index,argument!=0,*c.annotation.get(),*c.Frame().nak,c.secure_dns,c.network)); break; }
      case Action::BuildSocks: { c.Frame().params=ConnectJobParams(base::MakeRefCounted<SOCKSSocketParams>(std::move(c.Frame().params),argument!=0,ToHostPortPair(c.endpoint.get()),*c.Frame().nak,*c.annotation.get())); break; }
      case Action::PopProxy: { auto params=std::move(c.Frame().params); c.frames.pop_back(); c.Frame().params=std::move(params); break; }
      case Action::WrapDestinationSsl: { c.Frame().params=MakeSSLSocketParams(std::move(c.Frame().params),ToHostPortPair(c.endpoint.get()),c.Frame().config,c.endpoint_nak.get()); break; }
      case Action::Return: { c.output=std::move(c.Frame().params); break; }

   default:NOTREACHED();
  }
  return true;
 }
};
ConnectJobParams ConstructConnectJobParams(
    const ConnectJobFactory::Endpoint& endpoint,
    const ProxyChain& proxy_chain,
    const std::optional<NetworkTrafficAnnotationTag>& proxy_annotation_tag,
    const std::vector<SSLConfig::CertAndStatus>& allowed_bad_certs,
    ConnectJobFactory::AlpnMode alpn_mode,
    bool force_tunnel,
    PrivacyMode privacy_mode,
    const OnHostResolutionCallback& resolution_callback,
    const NetworkAnonymizationKey& endpoint_network_anonymization_key,
    SecureDnsPolicy secure_dns_policy,
    bool disable_cert_network_fetches,
    const CommonConnectJobParams* common_connect_job_params,
    const NetworkAnonymizationKey& proxy_dns_network_anonymization_key,
    handles::NetworkHandle target_network) {
ParamsRustBridge c{raw_ref(endpoint),raw_ref(proxy_chain),raw_ref(proxy_annotation_tag),raw_ref(allowed_bad_certs),
 alpn_mode,force_tunnel,privacy_mode,raw_ref(resolution_callback),
 raw_ref(endpoint_network_anonymization_key),secure_dns_policy,disable_cert_network_fetches,
 common_connect_job_params,raw_ref(proxy_dns_network_anonymization_key),target_network};
 connection_jobs_rust_run(11,0,0,&c,&ParamsRustBridge::Service);
 return std::move(c.output);
}

}  // namespace net
