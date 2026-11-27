// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_BASE_MIME_SNIFFER_FFI_H_
#define NET_BASE_MIME_SNIFFER_FFI_H_
#include <stddef.h>
#include <stdint.h>

#include <type_traits>
namespace net::mime_sniffer_internal {
struct RawSniff {
  const uint8_t* mime_data;
  size_t mime_size;
  uint8_t flags;  // bit0: original hint; bit1: complete/matched.
};
static_assert(sizeof(bool) == 1);
static_assert(std::is_standard_layout_v<RawSniff>);
static_assert(std::is_trivially_copyable_v<RawSniff>);
static_assert(sizeof(RawSniff) == 3 * sizeof(size_t));
static_assert(alignof(RawSniff) == alignof(size_t));
static_assert(offsetof(RawSniff, mime_size) == sizeof(size_t));
static_assert(offsetof(RawSniff, flags) == 2 * sizeof(size_t));
// Private ABI: every nonempty input is one live initialized byte range, at
// most PTRDIFF_MAX bytes, borrowed only for the call. Empty views may be null.
// The caller assigns output only after return. MIME pointers in RawSniff are
// immutable static Rust strings; they never point into any input or output.
extern "C" {
RawSniff rust_mime_sniff(const uint8_t* content,
                         size_t content_size,
                         const uint8_t* path,
                         size_t path_size,
                         const uint8_t* hint,
                         size_t hint_size,
                         uint8_t file_url,
                         uint8_t force_html,
                         uint8_t hint_alias,
                         size_t content_offset) noexcept;
RawSniff rust_mime_sniff_local(const uint8_t* content, size_t size) noexcept;
bool rust_mime_looks_binary(const uint8_t* content, size_t size) noexcept;
bool rust_mime_should_sniff(const uint8_t* mime,
                            size_t mime_size,
                            uint8_t scheme,
                            const uint8_t* options,
                            size_t options_size) noexcept;
}
}  // namespace net::mime_sniffer_internal
#endif
