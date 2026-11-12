// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_HTTP_HTTP_RANGE_FFI_H_
#define NET_HTTP_HTTP_RANGE_FFI_H_

#include <stddef.h>
#include <stdint.h>

#include <type_traits>

#include "base/compiler_specific.h"
#include "base/containers/span.h"
#include "base/memory/raw_ptr_exclusion.h"

namespace net::http_range_internal {

// Explicit C ABI records; no standard-library private layout is shared.
struct ParsedRange {
  int64_t first;
  int64_t last;
  int64_t suffix;
};
struct RangesBuffer {
  // The C ABI requires an unwrapped pointer owned by the Rust allocator.
  RAW_PTR_EXCLUSION ParsedRange* data;
  size_t len;
  size_t capacity;
};
static_assert(sizeof(ParsedRange) == 3 * sizeof(int64_t));
static_assert(alignof(ParsedRange) == alignof(int64_t));
static_assert(offsetof(ParsedRange, last) == sizeof(int64_t));
static_assert(offsetof(ParsedRange, suffix) == 2 * sizeof(int64_t));
static_assert(std::is_trivially_copyable_v<RangesBuffer>);
static_assert(sizeof(RangesBuffer) == 3 * sizeof(size_t));
static_assert(offsetof(RangesBuffer, len) == sizeof(size_t));
static_assert(offsetof(RangesBuffer, capacity) == 2 * sizeof(size_t));

inline constexpr uint8_t kListSyntaxValid = 1;
inline constexpr uint8_t kListHasEntries = 2;

// Input is live for each synchronous call; null data is allowed for zero
// length. List writes its owned metadata only when kListHasEntries is set,
// including successfully parsed prefixes on later syntax failure. Every owned
// buffer must be returned once, unchanged, to the Rust destructor below.
extern "C" uint8_t rustium_http_range_list(const uint8_t*,
                                           size_t,
                                           RangesBuffer*) noexcept;
extern "C" void rustium_http_range_list_drop(RangesBuffer) noexcept;

// Fetch writes only on success. Content initializes every output to -1 before
// reading input, supports equal scalar pointers and input/output byte overlap,
// and scopes each input borrow between its sequential scalar writes.
extern "C" bool rustium_http_range_fetch(const uint8_t*,
                                         size_t,
                                         bool allow_whitespace,
                                         ParsedRange*) noexcept;
extern "C" bool rustium_http_range_content(const uint8_t*,
                                           size_t,
                                           int64_t* first,
                                           int64_t* last,
                                           int64_t* length) noexcept;

// Allocation-free ownership guard for a nonempty Rust-produced buffer.
class ScopedRangeList {
 public:
  explicit ScopedRangeList(RangesBuffer buffer) : buffer_(buffer) {}
  ScopedRangeList(const ScopedRangeList&) = delete;
  ScopedRangeList& operator=(const ScopedRangeList&) = delete;
  ~ScopedRangeList() { rustium_http_range_list_drop(buffer_); }

  base::span<const ParsedRange> values() const LIFETIME_BOUND {
    // SAFETY: The Rust constructor transfers len initialized entries in this
    // live allocation. This guard retains their ownership until destruction.
    return UNSAFE_BUFFERS(base::span(buffer_.data, buffer_.len));
  }

 private:
  RangesBuffer buffer_;
};

}  // namespace net::http_range_internal
#endif  // NET_HTTP_HTTP_RANGE_FFI_H_
