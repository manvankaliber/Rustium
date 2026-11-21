// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/socket/transport_assignment_ffi.h"
#include "net/socket/transport_assignment_original.h"
#include <cstdint>
#include <vector>
#include "testing/gtest/include/gtest/gtest.h"
namespace net {
namespace {
TEST(TransportAssignmentPolicyTest, ExhaustiveChunksMatchOriginalPrefix) {
  for (uint32_t count = 0; count <= 16; ++count) {
    for (uint32_t bits = 0; bits < (1u << count); ++bits) {
      ASSERT_EQ(transport_assignment_original::FirstWithoutJob(bits, count),
                TransportAssignmentPrefix(bits, count))
          << "count=" << count << " bits=" << bits;
    }
  }
  EXPECT_EQ(-1, TransportAssignmentPrefix(0, 17));
  EXPECT_EQ(transport_assignment_original::UseFront(false), TransportAssignmentUseFront(false));
  EXPECT_EQ(transport_assignment_original::UseFront(true), TransportAssignmentUseFront(true));
}
// Mimic batch consumption over a queue snapshot. Both release (first missing)
// and steal (immediately preceding it) use the same original prefix boundary.
TEST(TransportAssignmentPolicyTest, PrefixAcrossBatchBoundaries) {
  for (unsigned total = 0; total <= 50; ++total) {
    for (unsigned assigned = 0; assigned <= total; ++assigned) {
      std::vector<bool> jobs(total, false);
      for (unsigned i = 0; i < assigned; ++i) jobs[i] = true;
      unsigned at = 0;
      while (at < total) {
        uint32_t bits = 0;
        unsigned count = 0;
        for (; count < 16 && at + count < total; ++count)
          if (jobs[at + count]) bits |= 1u << count;
        int n = TransportAssignmentPrefix(bits, count);
        ASSERT_GE(n, 0);
        ASSERT_LE(n, static_cast<int>(count));
        at += n;
        if (n < static_cast<int>(count)) break;
      }
      EXPECT_EQ(assigned, at) << "total=" << total;
      if (at) EXPECT_TRUE(jobs[at - 1]);
      if (at < total) EXPECT_FALSE(jobs[at]);
    }
  }
}
}  // namespace
}  // namespace net
