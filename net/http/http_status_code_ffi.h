#ifndef NET_HTTP_HTTP_STATUS_CODE_FFI_H_
#define NET_HTTP_HTTP_STATUS_CODE_FFI_H_
#include <cstddef>
#include <cstdint>
// Static immutable byte record: one length byte followed by that many phrase
// bytes and a trailing NUL. Null means unknown. No ownership transfer, Rust
// object layout or incoming default view crosses this ABI.
extern "C" const uint8_t* rustium_http_status_reason_record(int32_t code);
// A recognized nonnegative status code, or -1 for an unknown code. Returning
// the value avoids retaining a caller input register across the foreign call.
extern "C" int32_t rustium_http_status_parse(int32_t code);
#endif
