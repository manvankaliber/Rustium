// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/filter/gzip_header.h"

#include <stddef.h>
#include <stdint.h>

#include <algorithm>
#include <vector>

#include "base/check_op.h"
#include "base/compiler_specific.h"
#include "base/containers/span.h"
#include "base/containers/span_rust.h"
#include "net/filter/gzip_header_ffi.rs.h"
#include "net/filter/gzip_header_test_support.h"

namespace {
void Compare(base::span<const uint8_t> bytes,
             size_t chunk_size,
             uint8_t control) {
  net::GZipHeader actual;
  net::gzip_header_internal::test::LegacyHeader expected;
  net::gzip_header_internal::State state;
  auto step = [&](base::span<const uint8_t> input) {
    size_t a = 0xcafe;
    size_t b = a;
    size_t c = a;
    const auto status = expected.ReadMore(input, b);
    CHECK_EQ(static_cast<int>(actual.ReadMore(input, a)), status);
    CHECK_EQ(net::gzip_header_internal::read_more(base::SpanToRustSlice(input),
                                                  state, c),
             status);
    CHECK_EQ(a, b);
    CHECK_EQ(c, b);
    CHECK_EQ(state.phase, expected.phase());
    CHECK_EQ(state.flags, expected.flags());
    CHECK_EQ(state.extra_length, expected.extra_length());
  };
  CHECK_EQ(net::GZipHeader::HasGZipHeader(bytes),
           net::gzip_header_internal::test::LegacyHeader::HasGZipHeader(bytes));
  step({});
  for (size_t pos = 0; pos < bytes.size();) {
    if ((control & 0x80) && (pos % 31 == 17)) {
      actual.Reset();
      expected.Reset();
      state = {};
    }
    const size_t count = std::min(chunk_size, bytes.size() - pos);
    step(bytes.subspan(pos, count));
    step({});
    pos += count;
  }
  step({});
}
}  // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  // SAFETY: libFuzzer supplies a live buffer of exactly size bytes.
  const auto input = UNSAFE_BUFFERS(base::span(data, size));
  const uint8_t control = input.empty() ? 0 : input[0];
  Compare(input, 1 + control % 17, control);
  // Also explore beyond the magic prefix, where fully random inputs rarely
  // reach optional fields. Mutations still control flags and all later bytes.
  std::vector<uint8_t> header = {0x1f, 0x8b, 8, control, 0, 0, 0, 0, 0, 255};
  header.insert(header.end(), input.begin(), input.end());
  Compare(header, 1 + control % 7, control);
  return 0;
}
