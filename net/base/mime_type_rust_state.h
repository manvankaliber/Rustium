// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_BASE_MIME_TYPE_RUST_STATE_H_
#define NET_BASE_MIME_TYPE_RUST_STATE_H_

#include <string>

#include "base/containers/span_rust.h"
#include "base/memory/raw_ptr_exclusion.h"

namespace net::mime_type_internal {

// A synchronous stack-only view of caller storage. Construction does not read
// either output: invalid inputs may legally supply null/unused output pointers.
// Getters are invoked lazily by Rust, and no writes occur until Rust returns.
class ContentTypeState {
 public:
  ContentTypeState(const std::string* mime, const bool* had)
      : mime_(mime), had_(had) {}
  rust::Slice<const uint8_t> current_mime() const {
    return base::SpanToRustSlice(base::as_byte_span(std::string_view(*mime_)));
  }
  bool had_charset() const { return *had_; }

 private:
  // Synchronous non-owning adapter pointers, never stored beyond the FFI call.
  // Preserve the public API's existing pointer lifetime/diagnostic contract.
  RAW_PTR_EXCLUSION const std::string* const mime_;
  RAW_PTR_EXCLUSION const bool* const had_;
};

}  // namespace net::mime_type_internal

#endif  // NET_BASE_MIME_TYPE_RUST_STATE_H_
