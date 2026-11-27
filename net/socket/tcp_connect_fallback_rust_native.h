// Source-only family draft. Native time/float primitive projection.
#ifndef NET_SOCKET_TCP_CONNECT_FALLBACK_RUST_NATIVE_H_
#define NET_SOCKET_TCP_CONNECT_FALLBACK_RUST_NATIVE_H_
#include "base/memory/raw_ptr.h"
#include "net/socket/tcp_connect_job.h"
#include "net/http/http_server_properties.h"
#include "net/nqe/network_quality_estimator.h"
#include "net/base/features.h"
#include "base/feature_list.h"
#include "base/notreached.h"
#include "url/scheme_host_port.h"
namespace net {
// Native time/feature/property services for the Rust fallback selection. This
// context has no receiver and cannot mutate a const ConnectJob.
struct TcpFallbackRustBridge {
  raw_ptr<const CommonConnectJobParams> common = nullptr;
  raw_ptr<const TransportSocketParams> params = nullptr;
  base::TimeDelta delay;
  std::optional<base::TimeDelta> rtt;
  url::SchemeHostPort destination;
  raw_ptr<const ServerNetworkStats> stats = nullptr;
  static bool Service(void* context, uint32_t module, uint32_t action,
                      int64_t, int64_t* output) {
    auto& c = *static_cast<TcpFallbackRustBridge*>(context);
    *output = 0; CHECK_EQ(module, 2u);
    switch (action) {
      case 1000: c.delay = TcpConnectJob::kIPv6FallbackTime; break;
      case 1001: *output = base::FeatureList::IsEnabled(features::kAdjustIPv6FallbackTime); break;
      case 1002: c.delay = features::kIPv6FallbackTime.Get(); break;
      case 1003: *output = base::FeatureList::IsEnabled(features::kIPv6FallbackBasedOnRTT); break;
      case 1004: *output = !!c.common->http_server_properties; break;
      case 1005: *output = std::holds_alternative<url::SchemeHostPort>(c.params->destination()); break;
      case 1006: c.destination = std::get<url::SchemeHostPort>(c.params->destination()); break;
      case 1007: { const auto& pair = std::get<HostPortPair>(c.params->destination());
        c.destination = url::SchemeHostPort("https", pair.host(), pair.port()); break; }
      case 1008: c.stats = c.common->http_server_properties->GetServerNetworkStats(
        c.destination, c.params->network_anonymization_key()); *output = !!c.stats; break;
      case 1009: *output = !c.stats->srtt.is_zero(); break;
      case 1010: c.rtt = c.stats->srtt; break;
      case 1011: *output = !!c.common->network_quality_estimator; break;
      case 1012: c.rtt = c.common->network_quality_estimator->GetTransportRTT(); break;
      case 1013: *output = c.rtt.has_value(); break;
      case 1014: c.delay = *c.rtt * features::kIPv6FallbackRTTMultiplier.Get(); break;
      case 1015: *output = c.delay < features::kIPv6FallbackMin.Get(); break;
      case 1016: c.delay = features::kIPv6FallbackMin.Get(); break;
      case 1017: *output = features::kIPv6FallbackMax.Get() < c.delay; break;
      case 1018: c.delay = features::kIPv6FallbackMax.Get(); break;
      default: NOTREACHED();
    }
    return true;
  }
};

}
#endif
