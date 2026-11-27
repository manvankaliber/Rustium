// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_DNS_DNS_RESPONSE_RUST_FFI_H_
#define NET_DNS_DNS_RESPONSE_RUST_FFI_H_
#include <cstddef>
#include <cstdint>

#include "third_party/rust/cxx/v1/cxx.h"
namespace net::dns_response_rust {
// A live C++ ownership adapter, borrowed for one synchronous Rust call. Packet
// borrows end before any mutation of name, fields or aliased packet backing.
struct ParserContext;
struct ParseOutput;
bool aliased(const ParserContext& ctx);
void commit(ParserContext& ctx, const ParseOutput& output);
rust::Slice<const uint8_t> packet(const ParserContext& ctx);
void clear_name(ParserContext& ctx);
bool name_empty(const ParserContext& ctx);
void append_name(ParserContext& ctx, rust::Slice<const uint8_t> bytes);
void field(ParserContext& ctx, uint8_t index, uint32_t value);
void rdata(ParserContext& ctx, size_t offset, size_t len);
}  // namespace net::dns_response_rust
#endif
