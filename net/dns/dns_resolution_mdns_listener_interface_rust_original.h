// Copyright 2026 The Chromium Authors
// BSD-style license; see LICENSE.
// Exact original abstract listener interface with original HPP value type.
#ifndef NET_DNS_DNS_RESOLUTION_MDNS_LISTENER_INTERFACE_RUST_ORIGINAL_H_
#define NET_DNS_DNS_RESOLUTION_MDNS_LISTENER_INTERFACE_RUST_ORIGINAL_H_
#include <string>
#include <vector>
#include "base/component_export.h"
#include "net/base/host_port_rust_reference.h"
#include "net/base/ip_endpoint.h"
#include "net/dns/public/dns_query_type.h"
#include "net/dns/public/mdns_listener_update_type.h"
namespace net::dns_resolution_original {
using HostPortPair=::net::HostPortRustReference;
class COMPONENT_EXPORT(DNS_RESOLUTION_ORIGINAL) MdnsListenerOriginal {
   public:
    // Delegate type for result update notifications from MdnsListener. All
    // methods have a |result_type| field to allow a single delegate to be
    // passed to multiple MdnsListeners and be used to listen for updates for
    // multiple types for the same host.
    class Delegate {
     public:
      virtual ~Delegate() = default;

      virtual void OnAddressResult(MdnsListenerUpdateType update_type,
                                   DnsQueryType result_type,
                                   IPEndPoint address) = 0;
      virtual void OnTextResult(MdnsListenerUpdateType update_type,
                                DnsQueryType result_type,
                                std::vector<std::string> text_records) = 0;
      virtual void OnHostnameResult(MdnsListenerUpdateType update_type,
                                    DnsQueryType result_type,
                                    HostPortPair host) = 0;

      // For results which may be valid MDNS but are not handled/parsed by
      // HostResolver, e.g. pointers to the root domain.
      virtual void OnUnhandledResult(MdnsListenerUpdateType update_type,
                                     DnsQueryType result_type) = 0;
    };

    // Destruction cancels the listening operation.
    virtual ~MdnsListenerOriginal() = default;

    // Begins the listening operation, invoking |delegate| whenever results are
    // updated. |delegate| will no longer be called once the listening operation
    // is cancelled (via destruction of |this|).
    virtual int Start(Delegate* delegate) = 0;
  };
}
#endif
