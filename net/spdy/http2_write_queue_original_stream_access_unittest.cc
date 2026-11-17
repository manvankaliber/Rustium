// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/spdy/http2_write_queue_rust_test_support.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace net::http2_write_queue_test {
namespace {
TEST(Http2WriteQueueOriginalStreamAccessTest, BothGettersObserveSameFactory) {
  auto stream = MakeStream(MEDIUM, 17);
  auto original = StreamWeakPtrFor<Http2WriteQueueOriginal>(*stream);
  auto production = StreamWeakPtrFor<SpdyWriteQueue>(*stream);
  EXPECT_EQ(original.get(), stream.get());
  EXPECT_EQ(production.get(), stream.get());
  EXPECT_EQ(original.get(), production.get());
  stream.reset();
  EXPECT_FALSE(original);
  EXPECT_FALSE(production);
}

TEST(Http2WriteQueueOriginalStreamAccessTest, NewOwnerDoesNotReuseExpiredView) {
  auto first = MakeStream(LOW, 1);
  auto expired = StreamWeakPtrFor<Http2WriteQueueOriginal>(*first);
  first.reset();
  EXPECT_FALSE(expired);
  auto second = MakeStream(LOW, 1);
  auto current = StreamWeakPtrFor<Http2WriteQueueOriginal>(*second);
  EXPECT_TRUE(current);
  EXPECT_EQ(current.get(), second.get());
  EXPECT_FALSE(expired);
}
}  // namespace
}  // namespace net::http2_write_queue_test
