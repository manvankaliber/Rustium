// Copyright 2026 The Chromium Authors
// BSD-style license: see LICENSE.
#include "net/spdy/http2_stream_family_rust_test_support.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {
static_assert(sizeof(SpdyStream) == sizeof(SpdyStreamOriginal));
static_assert(alignof(SpdyStream) == alignof(SpdyStreamOriginal));
TEST(Http2StreamFamilyRustTest, IndependentPublicControlPaths) {
  const http2_family_test::Inputs inputs;
  for (size_t kind = 0; kind != 5; ++kind) {
    EXPECT_EQ(http2_family_test::RunCase<SpdyStreamOriginal>(kind, inputs, true),
              http2_family_test::RunCase<SpdyStream>(kind, inputs, true));
  }
}
TEST(Http2StreamFamilyRustTest, WindowOverflowHasNoCommit) {
  const http2_family_test::Inputs inputs;
  auto original = http2_family_test::Create<SpdyStreamOriginal>(
      inputs, std::numeric_limits<int32_t>::max());
  auto rust = http2_family_test::Create<SpdyStream>(
      inputs, std::numeric_limits<int32_t>::max());
  EXPECT_FALSE(original->AdjustSendWindowSize(1));
  EXPECT_FALSE(rust->AdjustSendWindowSize(1));
  EXPECT_EQ(original->send_window_size(), std::numeric_limits<int32_t>::max());
  EXPECT_EQ(rust->send_window_size(), original->send_window_size());
  original->OnClose(OK);
  rust->OnClose(OK);
}
}  // namespace net
