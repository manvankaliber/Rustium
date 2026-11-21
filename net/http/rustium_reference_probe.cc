#define RUSTIUM_REFERENCE_PROBE 1
#include "net/http/rustium_reference_probe.h"

#include "net/http/http_request_headers_rust_ffi.h"
#if !defined(RUSTIUM_REFERENCE_PROBE)
#error This file belongs only in the isolated qualification build.
#endif
extern "C" uint64_t rustium_probe_auth_count();
extern "C" uint64_t rustium_probe_auth_cache_count();
extern "C" uint64_t rustium_probe_request_headers_count();
extern "C" uint64_t rustium_probe_net_error_count();
extern "C" uint64_t rustium_probe_net_log_count();
namespace net::rustium_reference_probe {
uint64_t AuthCount() { return rustium_probe_auth_count(); }
uint64_t AuthCacheCount() { return rustium_probe_auth_cache_count(); }
uint64_t RequestHeadersCount() { return rustium_probe_request_headers_count(); }
uint64_t NetErrorCount() { return rustium_probe_net_error_count(); }
uint64_t NetLogCount() { return rustium_probe_net_log_count(); }

namespace request_headers {
bool Validate(const char* data, size_t length, uint32_t kind) {
  return ::rustium_request_headers_validate(data, length, kind);
}
size_t Crlf(const char* data, size_t length) {
  return ::rustium_request_headers_crlf(data, length);
}
RequestHeadersRustParsed Parse(const char* data, size_t length) {
  return ::rustium_request_headers_parse(data, length);
}
}  // namespace request_headers
}
