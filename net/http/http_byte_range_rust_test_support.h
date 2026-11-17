// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_HTTP_HTTP_BYTE_RANGE_RUST_TEST_SUPPORT_H_
#define NET_HTTP_HTTP_BYTE_RANGE_RUST_TEST_SUPPORT_H_
#include <array>
#include <bit>
#include <cstring>
#include <limits>
#include <type_traits>

#include "base/check.h"
#include "base/check_op.h"
#include "net/http/http_byte_range.h"
#include "net/http/http_byte_range_rust_reference.h"
namespace net::http_byte_range_rust_test {
using Original = http_byte_range_rust_reference::OriginalHttpByteRange;
static_assert(sizeof(HttpByteRange) == sizeof(Original));
static_assert(sizeof(HttpByteRange) == 32 && alignof(HttpByteRange) == 8);
static_assert(alignof(HttpByteRange) == alignof(Original));
static_assert(std::is_standard_layout_v<HttpByteRange>);
static_assert(std::is_trivially_copyable_v<HttpByteRange>);
static_assert(std::is_trivially_destructible_v<HttpByteRange>);
// Offset 24 is verified with compiler record-layout artifacts for both classes.
// Reading an initialized member's representation via unsigned char is allowed;
// padding is never examined and no private object is reinterpreted as a mirror.
template <class T>
bool Computed(const T& range) {
  unsigned char flag;
  UNSAFE_BUFFERS(std::memcpy(
      &flag, reinterpret_cast<const unsigned char*>(&range) + 24, 1));
  CHECK_LE(flag, 1);
  return flag != 0;
}
inline bool UndefinedCompute(const Original& r, int64_t size) {
  return size >= 0 && !Computed(r) && r.IsValid() && r.suffix_length() < -1 &&
         size > std::numeric_limits<int64_t>::max() + r.suffix_length();
}
inline void Equal(const HttpByteRange& a, const Original& b) {
  CHECK_EQ(a.first_byte_position(), b.first_byte_position());
  CHECK_EQ(a.last_byte_position(), b.last_byte_position());
  CHECK_EQ(a.suffix_length(), b.suffix_length());
  CHECK_EQ(Computed(a), Computed(b));
  CHECK_EQ(a.IsSuffixByteRange(), b.IsSuffixByteRange());
  CHECK_EQ(a.HasFirstBytePosition(), b.HasFirstBytePosition());
  CHECK_EQ(a.HasLastBytePosition(), b.HasLastBytePosition());
  CHECK_EQ(a.IsValid(), b.IsValid());
  if (b.IsValid()) {
    CHECK_EQ(a.GetHeaderValue(), b.GetHeaderValue());
  }
}
inline uint64_t Next(uint64_t& state) {
  state ^= state << 13;
  state ^= state >> 7;
  state ^= state << 17;
  return state;
}
inline int64_t Value(uint64_t& state) {
  constexpr std::array<int64_t, 13> edges = {
      INT64_MIN, INT64_MIN + 1, -100,          -2,        -1,  0, 1, 2,
      10,        100,           INT64_MAX - 1, INT64_MAX, 4096};
  const auto bits = Next(state);
  return bits & 1 ? edges[(bits >> 1) % edges.size()]
                  : std::bit_cast<int64_t>(bits);
}
struct Counts {
  uint64_t states = 0, operations = 0, undefined_compute_excluded = 0;
};
inline Counts Exercise(uint64_t seed, uint64_t states) {
  Counts counts;
  for (uint64_t i = 0; i < states; ++i) {
    const auto first = Value(seed), last = Value(seed), suffix = Value(seed);
    HttpByteRange a;
    Original b;
    switch (i % 4) {
      case 1:
        a = HttpByteRange::Bounded(first, last);
        b = Original::Bounded(first, last);
        break;
      case 2:
        a = HttpByteRange::RightUnbounded(first);
        b = Original::RightUnbounded(first);
        break;
      case 3:
        a = HttpByteRange::Suffix(suffix);
        b = Original::Suffix(suffix);
        break;
      default:
        break;
    }
    Equal(a, b);
    for (unsigned step = 0; step < 16; ++step) {
      const auto value = Value(seed);
      switch (Next(seed) % 8) {
        case 0:
          a.set_first_byte_position(value);
          b.set_first_byte_position(value);
          break;
        case 1:
          a.set_last_byte_position(value);
          b.set_last_byte_position(value);
          break;
        case 2:
          a.set_suffix_length(value);
          b.set_suffix_length(value);
          break;
        case 3: {
          auto copy_a = a;
          auto copy_b = b;
          Equal(copy_a, copy_b);
          a = copy_a;
          b = copy_b;
          break;
        }
        case 4: {
          auto& alias_a = a;
          auto& alias_b = b;
          a = alias_a;
          b = alias_b;
          break;
        }
        default:
          if (UndefinedCompute(b, value)) {
            ++counts.undefined_compute_excluded;
          } else {
            CHECK_EQ(a.ComputeBounds(value), b.ComputeBounds(value));
          }
      }
      Equal(a, b);
      ++counts.operations;
    }
    ++counts.states;
  }
  return counts;
}
}  // namespace net::http_byte_range_rust_test
#endif
