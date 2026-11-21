// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/socket/native_owner_adapter.h"

#include <stddef.h>

#include <memory>
#include <vector>

#include "testing/gtest/include/gtest/gtest.h"

namespace net {
namespace {
struct MockRequest { int priority; };

TEST(RetainedRequestOwnersTest, ReplyLayoutMatchesRust) {
  EXPECT_EQ(sizeof(RustRequestOrderReply), rust_request_queue_reply_layout(0));
  EXPECT_EQ(alignof(RustRequestOrderReply), rust_request_queue_reply_layout(1));
  EXPECT_EQ(offsetof(RustRequestOrderReply, token), rust_request_queue_reply_layout(2));
  EXPECT_EQ(offsetof(RustRequestOrderReply, rank), rust_request_queue_reply_layout(3));
  EXPECT_EQ(offsetof(RustRequestOrderReply, donor), rust_request_queue_reply_layout(4));
  EXPECT_EQ(offsetof(RustRequestOrderReply, action), rust_request_queue_reply_layout(5));
}

TEST(RetainedRequestOwnersTest, DetachedOwnerInvisibleAndSameNodeReturns) {
  RetainedRequestOwners<MockRequest> owners;
  auto first = owners.Insert(std::make_unique<MockRequest>(MockRequest{2}),
                             2, true, 0, 0);
  auto second = owners.Insert(std::make_unique<MockRequest>(MockRequest{2}),
                              2, true, 0, 0);
  EXPECT_EQ((std::vector<uint64_t>{first.token, second.token}), owners.Snapshot());
  auto no_change = owners.DetachForPriority(first.token, 2);
  EXPECT_TRUE(no_change.empty());
  auto node = owners.DetachForPriority(first.token, 4);
  ASSERT_FALSE(node.empty());
  EXPECT_EQ(nullptr, owners.Lookup(first.token));
  EXPECT_EQ((std::vector<uint64_t>{second.token}), owners.Snapshot());
  // Simulates native release/reassignment callback visibility while detached.
  EXPECT_EQ(second.token, owners.At(0));
  node.mapped()->priority = 4;
  auto result = owners.Finish(std::move(node), 0, 0);
  EXPECT_EQ(first.token, result.token);
  ASSERT_NE(nullptr, owners.Lookup(first.token));
  EXPECT_EQ(4, owners.Lookup(first.token)->priority);
  EXPECT_EQ((std::vector<uint64_t>{first.token, second.token}), owners.Snapshot());
}
}  // namespace
}  // namespace net
