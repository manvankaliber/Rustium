// Copyright 2009 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/http/http_range_original_byte_range.h"

#include <algorithm>

#include "base/check.h"
#include "base/format_macros.h"
#include "base/strings/stringprintf.h"

namespace {

const int64_t kPositionNotSpecified = -1;

}  // namespace

namespace net::http_range_promote_internal::test {

RangeOriginalByteRange::RangeOriginalByteRange()
    : first_byte_position_(kPositionNotSpecified),
      last_byte_position_(kPositionNotSpecified),
      suffix_length_(kPositionNotSpecified) {}

// static
RangeOriginalByteRange RangeOriginalByteRange::Bounded(
    int64_t first_byte_position,
    int64_t last_byte_position) {
  RangeOriginalByteRange range;
  range.set_first_byte_position(first_byte_position);
  range.set_last_byte_position(last_byte_position);
  return range;
}

// static
RangeOriginalByteRange RangeOriginalByteRange::RightUnbounded(
    int64_t first_byte_position) {
  RangeOriginalByteRange range;
  range.set_first_byte_position(first_byte_position);
  return range;
}

// static
RangeOriginalByteRange RangeOriginalByteRange::Suffix(int64_t suffix_length) {
  RangeOriginalByteRange range;
  range.set_suffix_length(suffix_length);
  return range;
}

bool RangeOriginalByteRange::IsSuffixByteRange() const {
  return suffix_length_ != kPositionNotSpecified;
}

bool RangeOriginalByteRange::HasFirstBytePosition() const {
  return first_byte_position_ != kPositionNotSpecified;
}

bool RangeOriginalByteRange::HasLastBytePosition() const {
  return last_byte_position_ != kPositionNotSpecified;
}

bool RangeOriginalByteRange::IsValid() const {
  if (suffix_length_ > 0)
    return true;
  return (first_byte_position_ >= 0 &&
          (last_byte_position_ == kPositionNotSpecified ||
           last_byte_position_ >= first_byte_position_));
}

std::string RangeOriginalByteRange::GetHeaderValue() const {
  DCHECK(IsValid());

  if (IsSuffixByteRange())
    return base::StringPrintf("bytes=-%" PRId64, suffix_length());

  DCHECK(HasFirstBytePosition());

  if (!HasLastBytePosition())
    return base::StringPrintf("bytes=%" PRId64 "-", first_byte_position());

  return base::StringPrintf("bytes=%" PRId64 "-%" PRId64, first_byte_position(),
                            last_byte_position());
}

bool RangeOriginalByteRange::ComputeBounds(int64_t size) {
  if (size < 0)
    return false;
  if (has_computed_bounds_)
    return false;
  has_computed_bounds_ = true;

  // Empty values.
  if (!HasFirstBytePosition() && !HasLastBytePosition() &&
      !IsSuffixByteRange()) {
    first_byte_position_ = 0;
    last_byte_position_ = size - 1;
    return true;
  }
  if (!IsValid())
    return false;
  if (IsSuffixByteRange()) {
    first_byte_position_ = size - std::min(size, suffix_length_);
    last_byte_position_ = size - 1;
    return true;
  }
  if (first_byte_position_ < size) {
    if (HasLastBytePosition())
      last_byte_position_ = std::min(size - 1, last_byte_position_);
    else
      last_byte_position_ = size - 1;
    return true;
  }
  return false;
}

}  // namespace net::http_range_promote_internal::test
