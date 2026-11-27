// Native value/I/O providers, not a policy reference or layout cast.
#ifndef NET_DNS_DNS_RESOLUTION_MANAGER_NATIVE_PROVIDERS_H_
#define NET_DNS_DNS_RESOLUTION_MANAGER_NATIVE_PROVIDERS_H_
namespace net {
class DatagramServerSocket; class IPAddress; class HttpResponseInfo; class URLRequestContext;
struct LoadTimingInternalInfo;
}
namespace net::dns_resolution_original {
using DatagramServerSocket=::net::DatagramServerSocket;
using IPAddress=::net::IPAddress;
using HttpResponseInfo=::net::HttpResponseInfo;
using URLRequestContext=::net::URLRequestContext;
using LoadTimingInternalInfo=::net::LoadTimingInternalInfo;
}
#endif
