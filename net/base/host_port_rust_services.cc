// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "base/compiler_specific.h"
#include "net/base/host_port_rust_ffi.h"
#include "net/base/host_port_rust_ffi.rs.h"
#include "url/url_canon.h"
#include "url/url_canon_ip.h"
#include "url/url_util.h"

namespace net {
namespace host_port_rust {
namespace {
std::string_view View(rust::Slice<const uint8_t> bytes) {
  // SAFETY: CXX provides initialized bytes, valid for this synchronous call.
  return bytes.empty()
             ? std::string_view()
             : UNSAFE_BUFFERS(std::string_view(
                   reinterpret_cast<const char*>(bytes.data()), bytes.size()));
}
}  // namespace
Authority authority(rust::Slice<const uint8_t> bytes) {
  auto input = View(bytes);
  url::Component auth(0, input.size()), user, password, host, port;
  url::ParseAuthority(input, auth, url::ParserMode::kSpecialURL, &user,
                      &password, &host, &port);
  return {user.len, password.len, host.begin, host.len, port.begin, port.len};
}
int32_t parse_port(rust::Slice<const uint8_t> bytes,
                   int32_t begin,
                   int32_t len) {
  return url::ParsePort(View(bytes), url::Component(begin, len));
}
bool ipv6(rust::Slice<const uint8_t> bytes) {
  unsigned char address[16];
  return url::IPv6AddressToNumber(View(bytes), address);
}
}  // namespace host_port_rust
namespace {
rust::Slice<const uint8_t> Bytes(std::string_view input) {
  // SAFETY: The input view remains live and unchanged for the synchronous call.
  return input.empty() ? rust::Slice<const uint8_t>()
                       : UNSAFE_BUFFERS(rust::Slice<const uint8_t>(
                             reinterpret_cast<const uint8_t*>(input.data()),
                             input.size()));
}
}  // namespace
bool host_port_rust::ParseHostAndPort(std::string_view input,
                                      std::string* host,
                                      int* port) {
  const auto result = parse(Bytes(input));
  if (!result.ok) {
    return false;
  }
  // No Rust borrow survives this call; copying before assignment supports input
  // views into *host exactly as the original temporary std::string did.
  std::string copied(input.substr(result.start, result.len));
  *host = std::move(copied);
  *port = result.port;
  return true;
}

}  // namespace net
