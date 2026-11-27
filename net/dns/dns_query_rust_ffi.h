// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_DNS_DNS_QUERY_RUST_FFI_H_
#define NET_DNS_DNS_QUERY_RUST_FFI_H_

#include <cstddef>
#include <cstdint>

#include "third_party/rust/cxx/v1/cxx.h"

namespace net::dns_query_rust {
// Stack-scoped object/service seam. Rust selects all operations; these
// callbacks only dispatch unchanged OptRecordRdata methods. Nothing escapes.
struct OptContext;
bool opt_clone_or_create(OptContext& context, bool clone_input);
bool opt_contains(const OptContext& context, uint16_t code);
void opt_add_ede(OptContext& context);
void opt_add_padding(OptContext& context, uint16_t len);
size_t opt_len(const OptContext& context);
size_t opt_count(const OptContext& context);
}  // namespace net::dns_query_rust

#endif  // NET_DNS_DNS_QUERY_RUST_FFI_H_
