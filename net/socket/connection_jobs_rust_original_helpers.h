// Reference-only frozen policy over public native storage/views.
#ifndef NET_SOCKET_CONNECTION_JOBS_RUST_ORIGINAL_HELPERS_H_
#define NET_SOCKET_CONNECTION_JOBS_RUST_ORIGINAL_HELPERS_H_
#include "net/base/host_port_pair.h"
#include "net/base/proxy_chain.h"
#include "net/base/proxy_server.h"
#include "url/scheme_host_port.h"
#include "base/check_op.h"
#include "net/base/connection_jobs_proxy_server_rust_original_access.h"
namespace net::connection_jobs_rust_original_helpers {
// Token-preserved from HostPortRustReference::FromSchemeHostPort, except the
// resulting public native value type. Original source is hash-bound separately.
inline HostPortPair FromSchemeHostPort(const url::SchemeHostPort& scheme_host_port) {
 DCHECK(scheme_host_port.IsValid());
 std::string_view host=scheme_host_port.host();
 if(host.size()>=2&&host.front()=='['&&host.back()==']') {
  host=host.substr(1,host.size()-2);
 }
 return HostPortPair(host,scheme_host_port.port());
}
inline bool IsHttp(const ProxyServer& p){return p.scheme()==ProxyServer::SCHEME_HTTP;}
inline bool IsHttps(const ProxyServer& p){return p.scheme()==ProxyServer::SCHEME_HTTPS;}
inline bool IsQuic(const ProxyServer& p){return p.scheme()==ProxyServer::SCHEME_QUIC;}
inline bool IsSocks(const ProxyServer& p){
 return p.scheme()==ProxyServer::SCHEME_SOCKS4||p.scheme()==ProxyServer::SCHEME_SOCKS5;
}
inline bool IsHttpLike(const ProxyServer& p){return IsHttp(p)||IsHttps(p)||IsQuic(p);}
inline bool IsSecureHttpLike(const ProxyServer& p){return IsHttps(p)||IsQuic(p);}
inline bool GetAllowed(const ProxyChain& chain){
 const auto& servers=chain.proxy_servers_if_valid();
 return servers&&servers->size()==1&&(IsHttp(servers->front())||IsHttps(servers->front()));
}
inline const HostPortPair& HostPort(const ProxyServer& server){
 return ConnectionJobsProxyServerRustOriginalAccess::HostPort(server);
}
}
#endif
