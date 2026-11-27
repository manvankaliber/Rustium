// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/parse_number.h"

#include "base/check.h"
#include "net/base/parse_number_native_ffi.h"

namespace net {
static_assert(sizeof(ParseIntFormat) == sizeof(int32_t));
static_assert(static_cast<int32_t>(ParseIntFormat::NON_NEGATIVE) == 0);
static_assert(static_cast<int32_t>(ParseIntFormat::OPTIONALLY_NEGATIVE) == 1);
static_assert(static_cast<int32_t>(ParseIntFormat::STRICT_NON_NEGATIVE) == 2);
static_assert(
    static_cast<int32_t>(ParseIntFormat::STRICT_OPTIONALLY_NEGATIVE) == 3);
static_assert(sizeof(ParseIntError) == 4);
static_assert(alignof(ParseIntError) == 4);
static_assert(static_cast<int32_t>(ParseIntError::FAILED_OVERFLOW) == 0);
static_assert(static_cast<int32_t>(ParseIntError::FAILED_UNDERFLOW) == 1);
static_assert(static_cast<int32_t>(ParseIntError::FAILED_PARSE) == 2);
bool ParseInt32(std::string_view input,
                ParseIntFormat format,
                int32_t* output,
                ParseIntError* error) {
  return parse_number_native_internal::rustium_parse_number_native_i32(
      reinterpret_cast<const uint8_t*>(input.data()), input.size(),
      static_cast<int32_t>(format), output, error);
}
bool ParseInt64(std::string_view input,
                ParseIntFormat format,
                int64_t* output,
                ParseIntError* error) {
  return parse_number_native_internal::rustium_parse_number_native_i64(
      reinterpret_cast<const uint8_t*>(input.data()), input.size(),
      static_cast<int32_t>(format), output, error);
}
bool ParseUint32(std::string_view input,
                 ParseIntFormat format,
                 uint32_t* output,
                 ParseIntError* error) {
  CHECK(format == ParseIntFormat::NON_NEGATIVE ||
        format == ParseIntFormat::STRICT_NON_NEGATIVE);
  return parse_number_native_internal::rustium_parse_number_native_u32(
      reinterpret_cast<const uint8_t*>(input.data()), input.size(),
      static_cast<int32_t>(format), output, error);
}
bool ParseUint64(std::string_view input,
                 ParseIntFormat format,
                 uint64_t* output,
                 ParseIntError* error) {
  CHECK(format == ParseIntFormat::NON_NEGATIVE ||
        format == ParseIntFormat::STRICT_NON_NEGATIVE);
  return parse_number_native_internal::rustium_parse_number_native_u64(
      reinterpret_cast<const uint8_t*>(input.data()), input.size(),
      static_cast<int32_t>(format), output, error);
}
}  // namespace net
