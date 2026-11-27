// Qualification-only bridge; counters are process-wide and monotonic.
#ifndef NET_HTTP_RUSTIUM_REFERENCE_PROBE_H_
#define NET_HTTP_RUSTIUM_REFERENCE_PROBE_H_
#include <stddef.h>
#include <stdint.h>
#include "net/base/net_export.h"
struct RequestHeadersRustParsed;

namespace net::rustium_reference_probe {
NET_EXPORT uint64_t AuthCount();
NET_EXPORT uint64_t AuthCacheCount();
NET_EXPORT uint64_t RequestHeadersCount();
NET_EXPORT uint64_t NetErrorCount();
NET_EXPORT uint64_t NetLogCount();
NET_EXPORT uint64_t HostPortEmptyCount();
NET_EXPORT uint64_t HostPortFormatCount();

// Direct ABI tests use the same Rust library as the component counter.
namespace request_headers {
NET_EXPORT_PRIVATE bool Validate(const char* data, size_t length, uint32_t kind);
NET_EXPORT_PRIVATE size_t Crlf(const char* data, size_t length);
NET_EXPORT_PRIVATE RequestHeadersRustParsed Parse(const char* data, size_t length);
}  // namespace request_headers
}
#endif
