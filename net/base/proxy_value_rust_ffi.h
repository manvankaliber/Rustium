// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_BASE_PROXY_VALUE_RUST_FFI_H_
#define NET_BASE_PROXY_VALUE_RUST_FFI_H_
#include <stdint.h>

#include <string>
#include <string_view>
#include <vector>

#include "base/compiler_specific.h"
#include "base/memory/stack_allocated.h"
#include "third_party/rust/cxx/v1/cxx.h"
namespace base {
class Pickle;
class PickleIterator;
}  // namespace base
namespace net {
class ProxyServer;
class ProxyChain;
}  // namespace net
namespace net::proxy_value_rust {
struct Authority;
struct ReadInt;
struct Server;
struct Chain;
// Stack-bound handles. Rust borrows these only for the synchronous call; they
// do not own/retain the caller's pickle, iterator, strings, or proxy values.
struct Reader {
  STACK_ALLOCATED();

 public:
  base::PickleIterator* iterator;
};
struct Writer {
  STACK_ALLOCATED();

 public:
  base::Pickle* pickle;
};
rust::Vec<uint8_t> canonical_host(rust::Slice<const uint8_t> input);
Authority parse_authority(rust::Slice<const uint8_t> input);
bool is_ipv6(rust::Slice<const uint8_t> input);
ReadInt read_int(Reader& reader);
ReadInt read_length(Reader& reader);
rust::Vec<uint8_t> read_string(Reader& reader);
void write_int(Writer& writer, int32_t value);
void write_string(Writer& writer, rust::Slice<const uint8_t> value);
void check_invalid_record(rust::Slice<const uint8_t> host, uint16_t port);
void check_host(rust::Slice<const uint8_t> host, bool require_nonempty);
void check_format_scheme(bool recognized);
inline rust::Slice<const uint8_t> Bytes(std::string_view s) {
  // SAFETY: initialized contiguous byte storage lives throughout each call;
  // the empty case never constructs a slice from a null C++ string_view.
  return s.empty() ? rust::Slice<const uint8_t>()
                   : UNSAFE_BUFFERS(rust::Slice<const uint8_t>(
                         reinterpret_cast<const uint8_t*>(s.data()), s.size()));
}
inline std::string_view View(rust::Slice<const uint8_t> s) {
  // SAFETY: CXX slices point to initialized immutable storage for this call.
  return s.empty() ? std::string_view()
                   : UNSAFE_BUFFERS(std::string_view(
                         reinterpret_cast<const char*>(s.data()), s.size()));
}
inline std::string String(const rust::Vec<uint8_t>& s) {
  // SAFETY: initialized CXX vector storage is live throughout the copy.
  return s.empty() ? std::string()
                   : UNSAFE_BUFFERS(std::string(
                         reinterpret_cast<const char*>(s.data()), s.size()));
}
rust::Vec<uint8_t> Copy(std::string_view s);
template <typename T>
rust::Slice<const T> Slice(const rust::Vec<T>& v) {
  // SAFETY: the initialized vector is immutable and live through the entire
  // synchronous Rust call, including when Snapshot creates a temporary.
  return v.empty() ? rust::Slice<const T>()
                   : UNSAFE_BUFFERS(rust::Slice<const T>(v.data(), v.size()));
}
Server Snapshot(const ProxyServer& server);
rust::Vec<Server> Snapshot(const ProxyChain& chain);
ProxyServer Restore(const Server& server);
ProxyChain Restore(const Chain& chain);
bool QuicEnabled();
bool MultiEnabled();
}  // namespace net::proxy_value_rust
#endif
