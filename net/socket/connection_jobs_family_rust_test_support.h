// Source-only ordinary family fixture. Not compiled or executed.
#ifndef NET_SOCKET_CONNECTION_JOBS_FAMILY_RUST_TEST_SUPPORT_H_
#define NET_SOCKET_CONNECTION_JOBS_FAMILY_RUST_TEST_SUPPORT_H_
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>
#include <vector>
#include "base/check.h"
#include "base/containers/flat_set.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "net/base/features.h"
#include "net/base/ip_address.h"
#include "net/base/proxy_chain.h"
#include "net/base/proxy_server.h"
#include "net/dns/public/host_resolver_results.h"
#include "net/http/http_auth_controller.h"
#include "net/log/net_log.h"
#include "net/socket/connect_job.h"
#include "net/socket/connect_job_params_factory.h"
#include "net/socket/connect_job_params_factory_rust_original.h"
#include "net/socket/connect_job_rust_original.h"
#include "net/socket/socket_tag.h"
#include "net/socket/stream_socket.h"
#include "net/socket/tcp_connect_job.h"
#include "net/socket/tcp_connect_job_rust_original.h"
#include "net/socket/transport_client_socket_pool_test_util.h"
#include "net/socket/transport_connect_job.h"
#include "net/socket/transport_connect_job_rust_original.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
namespace net::connection_jobs_family_test {
struct Production {
 using Base=ConnectJob; using Common=CommonConnectJobParams;
 using Params=TransportSocketParams; using Transport=TransportConnectJob;
 using Tcp=TcpConnectJob; using Factory=ConnectJobFactory;
 using ResolveCallback=OnHostResolutionCallback;
 static auto ParamsFor(const Factory::Endpoint& endpoint,const ProxyChain& chain,
   const Common* common,const NextProtoVector&,const SSLConfig::ApplicationSettings&) {
  return ConstructConnectJobParams(endpoint,chain,TRAFFIC_ANNOTATION_FOR_TESTS,{},
   Factory::AlpnMode::kHttpAll,false,PRIVACY_MODE_DISABLED,ResolveCallback(),
   NetworkAnonymizationKey(),SecureDnsPolicy::kAllow,false,common,
   NetworkAnonymizationKey(),handles::kInvalidNetworkHandle);
 }
};
struct Original {
 using Base=ConnectJobRustOriginal; using Common=CommonConnectJobParamsRustOriginal;
 using Params=TransportSocketParamsRustOriginal; using Transport=TransportConnectJobRustOriginal;
 using Tcp=TcpConnectJobRustOriginal; using Factory=ConnectJobFactoryRustOriginal;
 using ResolveCallback=ConnectJobRustOriginalHostResolutionCallback;
 static auto ParamsFor(const Factory::Endpoint& endpoint,const ProxyChain& chain,
   const Common* common,const NextProtoVector&,const SSLConfig::ApplicationSettings&) {
  return ConstructConnectJobParamsRustOriginal(endpoint,chain,TRAFFIC_ANNOTATION_FOR_TESTS,{},
   Factory::AlpnMode::kHttpAll,false,PRIVACY_MODE_DISABLED,ResolveCallback(),
   NetworkAnonymizationKey(),SecureDnsPolicy::kAllow,false,common,
   NetworkAnonymizationKey(),handles::kInvalidNetworkHandle);
 }
};
inline IPEndPoint Ip(std::string_view text,uint16_t port) {
 auto address=IPAddress::FromIPLiteral(text);CHECK(address);
 return IPEndPoint(*address,port);
}
template<class Traits>
struct Delegate final:Traits::Base::Delegate {
 int result=ERR_IO_PENDING;
 void OnConnectJobComplete(int value,typename Traits::Base*) override {result=value;}
 void OnNeedsProxyAuth(const HttpResponseInfo&,HttpAuthController*,base::OnceClosure,
                      typename Traits::Base*) override {CHECK(false);}
};
template<class Traits>
struct Fixture {
 MockTransportClientSocketFactory socket_factory{NetLog::Get()};
 NextProtoVector alpns{NextProto::kProtoHTTP2,NextProto::kProtoHTTP11};
 SSLConfig::ApplicationSettings settings{{NextProto::kProtoHTTP2,{}}};
 bool early=false;
 typename Traits::Common common{
  &socket_factory,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,
  nullptr,nullptr,nullptr,nullptr,NetLog::Get(),nullptr,nullptr,
  &alpns,&settings,nullptr,&early};
 scoped_refptr<typename Traits::Params> params=base::MakeRefCounted<typename Traits::Params>(
  url::SchemeHostPort("http","family.example",80),NetworkAnonymizationKey(),
  SecureDnsPolicy::kAllow,handles::kInvalidNetworkHandle,typename Traits::ResolveCallback(),
  base::flat_set<std::string>());
 std::vector<IPEndPoint> one{Ip("192.0.2.1",80)};
 std::vector<IPEndPoint> many{Ip("192.0.2.1",80),Ip("192.0.2.2",80),Ip("192.0.2.3",80)};
 std::vector<IPEndPoint> v6{Ip("2001:db8::1",80)};
 std::vector<IPEndPoint> both{Ip("2001:db8::1",80),Ip("192.0.2.1",80)};
 ProxyChain direct=ProxyChain::Direct();
 ProxyChain http=ProxyChain(ProxyServer::SCHEME_HTTP,HostPortPair("proxy.example",8080));
 ProxyChain socks=ProxyChain(ProxyServer::SCHEME_SOCKS5,HostPortPair("proxy.example",1080));
};
// Twelve full job lifecycles plus four params-family paths. Setup/providers are
// outside clocks. Construction, attempt scheduling, socket publication, queries
// and destruction are inside. Mock network I/O is a shared explicit primitive.
inline constexpr std::array<const char*,16> kCases={
 "transport_sync_v4","transport_fail_three","transport_sync_v6",
 "transport_dual_sync","transport_dual_fail","transport_cancel_unstarted",
 "tcp_sync_v4","tcp_fail_three","tcp_sync_v6","tcp_dual_sync",
 "tcp_dual_fail","tcp_cancel_unstarted",
 "params_direct_http","params_direct_https","params_http_proxy","params_socks5"};
template<class Traits>
uint64_t Run(Fixture<Traits>& f,size_t kind,size_t iterations) {
 uint64_t checksum=0;
 const size_t local=kind%6;
 f.socket_factory.set_default_client_socket_type(
  (local==1||local==4)&&kind<12?MockTransportClientSocketFactory::Type::kFailing:
                              MockTransportClientSocketFactory::Type::kSynchronous);
 for(size_t iteration=0;iteration<iterations;++iteration) {
  if(kind>=12) {
   const bool tls=kind==13;
   const typename Traits::Factory::Endpoint endpoint=
    url::SchemeHostPort(tls?"https":"http","family.example",tls?443:80);
   const ProxyChain& chain=kind==14?f.http:kind==15?f.socks:f.direct;
   auto params=Traits::ParamsFor(endpoint,chain,&f.common,f.alpns,f.settings);
   checksum=checksum*131+params.is_transport()+2*params.is_ssl()+
            4*params.is_http_proxy()+8*params.is_socks();
   continue;
  }
  const auto& ips=local==1?f.many:local==2?f.v6:(local==3||local==4)?f.both:f.one;
  Delegate<Traits> delegate;
  int rv=ERR_IO_PENDING;
  const int before=f.socket_factory.allocation_count();
  if(kind<6) {
   HostResolverEndpointResult endpoint;endpoint.ip_endpoints=ips;
   typename Traits::Transport job(DEFAULT_PRIORITY,SocketTag(),&f.common,f.params,
     &delegate,nullptr,typename Traits::Transport::EndpointResultOverride(
       std::move(endpoint),base::flat_set<std::string>{"family.example"}));
   if(local!=5) rv=job.Connect();
   checksum=checksum*131+static_cast<uint32_t>(rv)+static_cast<int>(job.GetLoadState());
   checksum+=job.GetConnectionAttempts().size();
   if(rv==OK) {auto socket=job.PassSocket();CHECK(socket);checksum+=socket->GetDnsAliases().size();}
  } else {
   ServiceEndpoint endpoint;
   for(const auto& ip:ips) {
    if(ip.GetFamily()==ADDRESS_FAMILY_IPV6) endpoint.ipv6_endpoints.push_back(ip);
    else endpoint.ipv4_endpoints.push_back(ip);
   }
   typename Traits::Tcp job(DEFAULT_PRIORITY,SocketTag(),&f.common,f.params,
     &delegate,nullptr,typename Traits::Tcp::ServiceEndpointOverride(
       std::move(endpoint),base::flat_set<std::string>{"family.example"}));
   if(local!=5) rv=job.Connect();
   checksum=checksum*131+static_cast<uint32_t>(rv)+static_cast<int>(job.GetLoadState());
   checksum+=job.GetConnectionAttempts().size()+job.HasEstablishedConnection();
   if(rv==OK) {
    auto endpoint_out=job.PassServiceEndpoint();
    checksum+=endpoint_out.ipv4_endpoints.size()+endpoint_out.ipv6_endpoints.size();
    auto socket=job.PassSocket();CHECK(socket);checksum+=socket->GetDnsAliases().size();
   }
  }
  CHECK_EQ(delegate.result,ERR_IO_PENDING);
  CHECK_EQ(rv,local==5?ERR_IO_PENDING:(local==1||local==4)?ERR_CONNECTION_FAILED:OK);
  checksum+=f.socket_factory.allocation_count()-before;
 }
 return checksum;
}
}
#endif
