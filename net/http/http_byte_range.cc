// Copyright 2009 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/http/http_byte_range.h"

#include "base/check.h"
#include "net/http/http_byte_range_rust_ffi.h"
namespace net {
HttpByteRange::HttpByteRange()
    : HttpByteRange(http_byte_range_rust_construct(0, 0, 0)) {}
HttpByteRange::HttpByteRange(HttpByteRangeRustState state)
    : first_byte_position_(state.first),
      last_byte_position_(state.last),
      suffix_length_(state.suffix),
      has_computed_bounds_(state.computed) {}
HttpByteRange HttpByteRange::Bounded(int64_t first, int64_t last) {
  return HttpByteRange(http_byte_range_rust_construct(1, first, last));
}
HttpByteRange HttpByteRange::RightUnbounded(int64_t first) {
  return HttpByteRange(http_byte_range_rust_construct(2, first, 0));
}
HttpByteRange HttpByteRange::Suffix(int64_t suffix) {
  return HttpByteRange(http_byte_range_rust_construct(3, suffix, 0));
}
bool HttpByteRange::IsSuffixByteRange() const {
  return http_byte_range_rust_is_suffix(suffix_length_);
}
bool HttpByteRange::HasFirstBytePosition() const {
  return http_byte_range_rust_has_first(first_byte_position_);
}
bool HttpByteRange::HasLastBytePosition() const {
  return http_byte_range_rust_has_last(last_byte_position_);
}
bool HttpByteRange::IsValid() const {
  return http_byte_range_rust_is_valid(first_byte_position_,
                                       last_byte_position_, suffix_length_);
}
std::string HttpByteRange::GetHeaderValue() const {
  DCHECK(IsValid());
  if (!IsSuffixByteRange()) {
    DCHECK(HasFirstBytePosition());
  }
  const auto header = http_byte_range_rust_header(
      first_byte_position_, last_byte_position_, suffix_length_);
  // SAFETY: the Rust value contains 48 initialized bytes and len <= 47.
  return UNSAFE_BUFFERS(std::string(header.bytes, header.len));
}
bool HttpByteRange::ComputeBounds(int64_t size) {
  const auto result =
      http_byte_range_rust_compute({first_byte_position_, last_byte_position_,
                                    suffix_length_, has_computed_bounds_},
                                   size);
  first_byte_position_ = result.state.first;
  last_byte_position_ = result.state.last;
  suffix_length_ = result.state.suffix;
  has_computed_bounds_ = result.state.computed;
  return result.success;
}
}  // namespace net
