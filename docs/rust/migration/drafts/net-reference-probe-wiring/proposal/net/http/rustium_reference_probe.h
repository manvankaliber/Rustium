// Qualification-only bridge; counters are process-wide and monotonic.
#ifndef NET_HTTP_RUSTIUM_REFERENCE_PROBE_H_
#define NET_HTTP_RUSTIUM_REFERENCE_PROBE_H_
#include <stdint.h>
#include "net/base/net_export.h"
namespace net::rustium_reference_probe {
NET_EXPORT uint64_t AuthCount();
NET_EXPORT uint64_t AuthCacheCount();
NET_EXPORT uint64_t RequestHeadersCount();
NET_EXPORT uint64_t NetErrorCount();
NET_EXPORT uint64_t NetLogCount();
}
#endif
