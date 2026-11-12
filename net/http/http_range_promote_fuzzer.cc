// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>
#include <array>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

#include "base/check_op.h"
#include "base/compiler_specific.h"
#include "base/containers/span.h"
#include "net/http/http_range_promote_reference.h"
#include "net/http/http_util.h"
namespace {
namespace reference = net::http_range_promote_internal::test;
void Equal(const net::HttpByteRange& a, const net::HttpByteRange& b) {
  CHECK_EQ(a.first_byte_position(), b.first_byte_position());
  CHECK_EQ(a.last_byte_position(), b.last_byte_position());
  CHECK_EQ(a.suffix_length(), b.suffix_length());
}
void Compare(std::string_view input, uint8_t control) {
  std::vector<net::HttpByteRange> a;
  if (control & 1) {
    a.push_back(net::HttpByteRange::Bounded(12, 34));
  }
  auto b = a;
  CHECK_EQ(net::HttpUtil::ParseRangeHeader(input, &a),
           reference::ParseRangeHeader(input, &b));
  CHECK_EQ(a.size(), b.size());
  for (size_t i = 0; i < a.size(); ++i) {
    Equal(a[i], b[i]);
  }
  for (bool whitespace : {false, true}) {
    const auto x = net::HttpUtil::ParseFetchSingleRange(input, whitespace);
    const auto y = reference::ParseFetchSingleRange(input, whitespace);
    CHECK_EQ(x.has_value(), y.has_value());
    if (x) {
      Equal(*x, *y);
    }
  }
  constexpr std::array<std::array<size_t, 3>, 5> layouts = {
      {{0, 1, 2}, {0, 0, 2}, {0, 1, 0}, {0, 1, 1}, {0, 0, 0}}};
  const auto slots = layouts[control % layouts.size()];
  std::array<int64_t, 3> x = {7, 8, 9}, y = x;
  CHECK_EQ(net::HttpUtil::ParseContentRangeHeaderFor206(
               input, &x[slots[0]], &x[slots[1]], &x[slots[2]]),
           reference::ParseContentRangeHeaderFor206(
               input, &y[slots[0]], &y[slots[1]], &y[slots[2]]));
  for (size_t i = 0; i < 3; ++i) {
    CHECK_EQ(x[i], y[i]);
  }
}

void CompareOverlap(std::string_view input, uint8_t control) {
  std::array<int64_t, 32> a{}, b{};
  const size_t begin = control % 8;
  input = input.substr(0, 128);
  // SAFETY: The bounded view and offset fit in the live256-byte scalar array.
  if (!input.empty()) {
    UNSAFE_BUFFERS(std::memcpy(reinterpret_cast<char*>(a.data()) + begin,
                               input.data(), input.size()));
  }
  b = a;
  const auto av = UNSAFE_BUFFERS(std::string_view(
      reinterpret_cast<const char*>(a.data()) + begin, input.size()));
  const auto bv = UNSAFE_BUFFERS(std::string_view(
      reinterpret_cast<const char*>(b.data()) + begin, input.size()));
  const size_t slot = (control / 8) % 24;
  std::array<size_t, 3> outputs = {24, 25, 26};
  outputs[control % 3] = slot;
  switch ((control / 32) % 5) {
    case 1:
      outputs[1] = outputs[0];
      break;
    case 2:
      outputs[2] = outputs[0];
      break;
    case 3:
      outputs[2] = outputs[1];
      break;
    case 4:
      outputs[1] = outputs[2] = outputs[0];
      break;
  }
  CHECK_EQ(net::HttpUtil::ParseContentRangeHeaderFor206(
               av, &a[outputs[0]], &a[outputs[1]], &a[outputs[2]]),
           reference::ParseContentRangeHeaderFor206(
               bv, &b[outputs[0]], &b[outputs[1]], &b[outputs[2]]));
  for (size_t i = 0; i < a.size(); ++i) {
    CHECK_EQ(a[i], b[i]);
  }
}
}  // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  // SAFETY: libFuzzer supplies this initialized, live byte range.
  const auto bytes = UNSAFE_BUFFERS(base::span(data, size));
  const uint8_t control = bytes.empty() ? 0 : bytes[0];
  const auto input = UNSAFE_BUFFERS(
      std::string_view(reinterpret_cast<const char*>(data), size));
  Compare(input, control);
  CompareOverlap(input, control);
  Compare(std::string("bytes=") + std::string(input), control);
  std::string digits;
  for (uint8_t byte : bytes.first(std::min<size_t>(bytes.size(), 60))) {
    digits.push_back('0' + byte % 10);
  }
  const auto a = digits.substr(0, digits.size() / 3),
             b = digits.substr(digits.size() / 3, digits.size() / 3),
             c = digits.substr(2 * (digits.size() / 3));
  Compare("bytes=" + a + "-" + b + "," + c + "-", control);
  const auto content = "bytes " + a + "-" + b + "/" + c;
  Compare(content, control);
  CompareOverlap(content, control);
  return 0;
}
