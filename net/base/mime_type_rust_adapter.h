// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_BASE_MIME_TYPE_RUST_ADAPTER_H_
#define NET_BASE_MIME_TYPE_RUST_ADAPTER_H_

#include <cstdint>
#include <string>
#include <string_view>

#include "base/compiler_specific.h"
#include "base/containers/span_rust.h"
#include "net/base/mime_type_ffi.rs.h"

namespace net::mime_type_internal {

// CXX borrows only for the call. Plans hold checked offsets into input or owned
// transformed scratch bytes. Stage C++ strings before aliased caller writes.
inline rust::Slice<const uint8_t> Bytes(std::string_view input) {
  return base::SpanToRustSlice(base::as_byte_span(input));
}

inline std::string_view View(std::string_view input,
                             const rust::Vec<uint8_t>& scratch,
                             const Span& span) {
  if (span.scratch) {
    // SAFETY: Rust owns these initialized scratch bytes until the plan is
    // dropped. Empty CXX vectors may use a fabricated pointer; handle it first.
    input = scratch.empty() ? std::string_view()
                            : UNSAFE_BUFFERS(std::string_view(
                                  reinterpret_cast<const char*>(scratch.data()),
                                  scratch.size()));
  }
  return input.substr(span.start, span.end - span.start);
}

}  // namespace net::mime_type_internal

#endif  // NET_BASE_MIME_TYPE_RUST_ADAPTER_H_
