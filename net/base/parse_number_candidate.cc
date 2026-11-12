// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Test-only CXX/POD candidate. Production entry points remain in parse_number.cc
// after measured regressions in tiny public calls prevented acceptance.
#include "net/base/parse_number.h"

#include "base/check.h"
#include "base/strings/string_view_rust.h"
#include "net/base/parse_number_ffi.rs.h"

namespace net {
namespace {

template <typename Result, typename T>
bool Finish(const Result& result, T* output, ParseIntError* optional_error) {
  if (result.status == 0) {
    *output = result.value;
    return true;
  }
  if (optional_error) {
    // The bridge uses zero for success, then the public error enum plus one.
    *optional_error = static_cast<ParseIntError>(result.status - 1);
  }
  return false;
}

}  // namespace

bool ParseInt32(std::string_view input,
                ParseIntFormat format,
                int32_t* output,
                ParseIntError* optional_error) {
  return Finish(parse_number_internal::parse_i32(
                    base::StringViewToRustSlice(input),
                    static_cast<int32_t>(format), optional_error != nullptr),
                output, optional_error);
}

bool ParseInt64(std::string_view input,
                ParseIntFormat format,
                int64_t* output,
                ParseIntError* optional_error) {
  return Finish(parse_number_internal::parse_i64(
                    base::StringViewToRustSlice(input),
                    static_cast<int32_t>(format), optional_error != nullptr),
                output, optional_error);
}

bool ParseUint32(std::string_view input,
                 ParseIntFormat format,
                 uint32_t* output,
                 ParseIntError* optional_error) {
  CHECK(format == ParseIntFormat::NON_NEGATIVE ||
        format == ParseIntFormat::STRICT_NON_NEGATIVE);
  return Finish(parse_number_internal::parse_u32(
                    base::StringViewToRustSlice(input),
                    static_cast<int32_t>(format), optional_error != nullptr),
                output, optional_error);
}

bool ParseUint64(std::string_view input,
                 ParseIntFormat format,
                 uint64_t* output,
                 ParseIntError* optional_error) {
  CHECK(format == ParseIntFormat::NON_NEGATIVE ||
        format == ParseIntFormat::STRICT_NON_NEGATIVE);
  return Finish(parse_number_internal::parse_u64(
                    base::StringViewToRustSlice(input),
                    static_cast<int32_t>(format), optional_error != nullptr),
                output, optional_error);
}

}  // namespace net

