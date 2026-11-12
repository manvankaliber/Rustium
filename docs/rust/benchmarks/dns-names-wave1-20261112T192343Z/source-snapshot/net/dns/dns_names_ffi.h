// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_DNS_DNS_NAMES_FFI_H_
#define NET_DNS_DNS_NAMES_FFI_H_

#include <cstdint>

#include "base/containers/span_reader.h"
#include "base/memory/raw_ref.h"
#include "third_party/rust/cxx/v1/cxx.h"

namespace net::dns_names_internal {
struct CanonicalHostResult;
struct LabelChunk;
struct NameReader {
  STACK_ALLOCATED();

 public:
  const raw_ref<base::SpanReader<const uint8_t>> reader;
};
rust::Slice<const uint8_t> reader_remaining(const NameReader& reader);
LabelChunk reader_take(NameReader& reader, size_t count);
// Callback-scoped borrows; these services never retain input or mutate it.
bool host_compliant(rust::Slice<const uint8_t> input);
bool host_localhost(rust::Slice<const uint8_t> input);
bool host_ip_literal(rust::Slice<const uint8_t> input);
bool host_url_address(rust::Slice<const uint8_t> input);
CanonicalHostResult host_canonicalize(rust::Slice<const uint8_t> input);
}  // namespace net::dns_names_internal

#endif  // NET_DNS_DNS_NAMES_FFI_H_
