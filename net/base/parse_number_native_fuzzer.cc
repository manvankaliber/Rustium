// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include <string>
#include <string_view>

#include "base/check_op.h"
#include "base/compiler_specific.h"
#include "base/containers/span.h"
#include "net/base/parse_number.h"
#include "net/base/parse_number_native_reference.h"

namespace {
template <typename T, typename Actual, typename Expected>
void CompareOne(std::string_view input,
                net::ParseIntFormat format,
                Actual actual,
                Expected expected) {
  for (bool report_error : {false, true}) {
    T a = 23614;
    T b = a;
    auto ea = static_cast<net::ParseIntError>(19);
    auto eb = ea;
    CHECK_EQ(actual(input, format, &a, report_error ? &ea : nullptr),
             expected(input, format, &b, report_error ? &eb : nullptr));
    CHECK_EQ(a, b);
    CHECK_EQ(ea, eb);
  }
}

void Compare(std::string_view input, uint8_t control) {
  namespace reference = net::parse_number_native_internal::test;
  const auto signed_format = static_cast<net::ParseIntFormat>(control & 3);
  const auto unsigned_format = static_cast<net::ParseIntFormat>(control & 2);
  CompareOne<int32_t>(input, signed_format, net::ParseInt32,
                      reference::ParseInt32);
  CompareOne<int64_t>(input, signed_format, net::ParseInt64,
                      reference::ParseInt64);
  CompareOne<uint32_t>(input, unsigned_format, net::ParseUint32,
                       reference::ParseUint32);
  CompareOne<uint64_t>(input, unsigned_format, net::ParseUint64,
                       reference::ParseUint64);
}
}  // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  // SAFETY: libFuzzer supplies a live buffer with exactly size bytes.
  const auto bytes = UNSAFE_BUFFERS(base::span(data, size));
  const uint8_t control = bytes.empty() ? 0 : bytes[0];
  // SAFETY: The input buffer remains alive for the complete synchronous call.
  Compare(UNSAFE_BUFFERS(
              std::string_view(reinterpret_cast<const char*>(data), size)),
          control);
  std::string decimal;
  decimal.reserve(bytes.size() + 1);
  if (control & 4) {
    decimal.push_back('-');
  }
  for (uint8_t byte : bytes) {
    decimal.push_back('0' + byte % 10);
  }
  Compare(decimal, control);
  return 0;
}
