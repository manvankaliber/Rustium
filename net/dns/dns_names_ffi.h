// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_DNS_DNS_NAMES_FFI_H_
#define NET_DNS_DNS_NAMES_FFI_H_

#include <cstddef>
#include <cstdint>
#include <string>

#include "base/containers/span_reader.h"
#include "base/memory/raw_ref.h"
#include "third_party/rust/cxx/v1/cxx.h"

namespace net::dns_names_internal {
struct LabelChunk;
struct CanonicalName {
  STACK_ALLOCATED();

 public:
  const raw_ref<std::string> output;
};
// Input ranges are borrowed for the call, never retained or modified. Empty
// ranges may have null pointers. Only DomainNameLess calls this private ABI.
extern "C" bool net_dns_names_less(const uint8_t* lhs,
                                   size_t lhs_len,
                                   const uint8_t* rhs,
                                   size_t rhs_len);
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
bool host_canonicalize(rust::Slice<const uint8_t> input, CanonicalName& output);
void canonical_use_original(rust::Slice<const uint8_t> input,
                            CanonicalName& output);
}  // namespace net::dns_names_internal

#endif  // NET_DNS_DNS_NAMES_FFI_H_
