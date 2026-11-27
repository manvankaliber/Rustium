// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/socket/transport_client_socket_pool_scheduling_ffi.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "net/socket/transport_client_socket_pool_scheduling_original.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {
namespace {

using transport_oracle::StalledGroup;

std::optional<size_t> RunRust(const std::vector<StalledGroup>& groups) {
  std::optional<size_t> best;
  int32_t priority = -1;
  uint64_t packed = 0;
  uint32_t count = 0;
  std::array<size_t, 16> indices;
  auto flush = [&]() {
    if (!count) {
      return;
    }
    int32_t selected = SelectStalledGroupBatch(packed, count, priority);
    EXPECT_GE(selected, -1);
    EXPECT_LT(selected, static_cast<int32_t>(count));
    if (selected >= 0 && selected < static_cast<int32_t>(count)) {
      best = indices[selected];
      priority = static_cast<int32_t>((packed >> (selected * 4)) & 0xf);
    }
    packed = 0;
    count = 0;
  };
  for (size_t i = 0; i < groups.size(); ++i) {
    if (!groups[i].eligible) {
      continue;
    }
    indices[count] = i;
    packed |= static_cast<uint64_t>(groups[i].top_priority) << (count * 4);
    if (++count == indices.size()) {
      flush();
    }
  }
  flush();
  return best;
}

TEST(TransportSchedulingPolicyTest, DifferentialOrderedGroups) {
  EXPECT_EQ(RunRust({}), transport_oracle::FindTopStalledGroup({}));
  for (size_t size = 0; size <= 40; ++size) {
    for (unsigned seed = 0; seed < 100; ++seed) {
      std::vector<StalledGroup> groups;
      for (size_t i = 0; i < size; ++i)
        groups.push_back({((i + seed) % 4) != 0,
                          static_cast<int>((i * 7 + seed / 3) % 6)});
      EXPECT_EQ(RunRust(groups), transport_oracle::FindTopStalledGroup(groups))
          << "size=" << size << " seed=" << seed;
    }
  }
}

TEST(TransportSchedulingPolicyTest, EqualPriorityKeepsEarlierBatch) {
  std::vector<StalledGroup> groups(33, {true, 5});
  EXPECT_EQ(RunRust(groups), std::optional<size_t>(0));
  groups[0].eligible = false;
  EXPECT_EQ(RunRust(groups), std::optional<size_t>(1));
}

}  // namespace
}  // namespace net
