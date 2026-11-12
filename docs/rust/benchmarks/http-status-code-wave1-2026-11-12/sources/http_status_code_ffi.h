#ifndef NET_HTTP_HTTP_STATUS_CODE_FFI_H_
#define NET_HTTP_HTTP_STATUS_CODE_FFI_H_
#include <cstddef>
#include <cstdint>
#include <type_traits>
// Static immutable Rust bytes, no terminator required; null/zero means unknown.
// No ownership transfer. Caller defaults never cross this ABI.
struct RustiumHttpStatusReason { const unsigned char* data; size_t len; };
static_assert(std::is_standard_layout_v<RustiumHttpStatusReason>);
static_assert(sizeof(RustiumHttpStatusReason) == 2 * sizeof(size_t));
static_assert(alignof(RustiumHttpStatusReason) == alignof(size_t));
static_assert(offsetof(RustiumHttpStatusReason, len) == sizeof(size_t));
extern "C" RustiumHttpStatusReason rustium_http_status_reason(int32_t code);
extern "C" bool rustium_http_status_known(int32_t code);
#endif
