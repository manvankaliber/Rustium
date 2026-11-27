// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/socket/transport_assignment_ffi.h"

#include <stddef.h>

#include <vector>

#include "net/socket/transport_assignment_original_actions.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {
namespace {

using transport_assignment_original::ChooseHole;
using transport_assignment_original::ChooseRelease;

TEST(TransportAssignmentPolicyTest, InsertedHoleMatchesOriginalLoop) {
  for (size_t count = 0; count <= 8; ++count) {
    for (size_t assigned = 0; assigned <= count; ++assigned) {
      for (size_t hole = 0; hole <= count; ++hole) {
        std::vector<bool> jobs;
        for (size_t i = 0; i < count; ++i)
          jobs.push_back(i < assigned);
        jobs.insert(jobs.begin() + hole, false);
        for (size_t unassigned = 0; unassigned <= 2; ++unassigned) {
          if (unassigned && assigned != count)
            continue;
          EXPECT_EQ(ChooseHole(jobs, hole, unassigned),
                    TransportAssignmentHole(hole, jobs.size(), assigned,
                                            unassigned));
        }
      }
    }
  }
}

TEST(TransportAssignmentPolicyTest, RemovedAssignedJobMatchesOriginalLoop) {
  for (size_t count = 1; count <= 8; ++count) {
    for (size_t old_assigned = 1; old_assigned <= count; ++old_assigned) {
      for (size_t hole = 0; hole < old_assigned; ++hole) {
        std::vector<bool> jobs(count, false);
        for (size_t i = 0; i < old_assigned; ++i)
          jobs[i] = true;
        jobs[hole] = false;
        for (size_t unassigned = 0; unassigned <= 2; ++unassigned) {
          if (unassigned && old_assigned != count)
            continue;
          EXPECT_EQ(ChooseHole(jobs, hole, unassigned),
                    TransportAssignmentHole(hole, count, old_assigned - 1,
                                            unassigned));
        }
      }
    }
  }
}

TEST(TransportAssignmentPolicyTest, AddedOrReleasedJobMatchesOriginalLoop) {
  for (size_t count = 0; count <= 8; ++count) {
    for (size_t assigned = 0; assigned <= count; ++assigned) {
      std::vector<bool> jobs(count, false);
      for (size_t i = 0; i < assigned; ++i)
        jobs[i] = true;
      EXPECT_EQ(ChooseRelease(jobs),
                TransportAssignmentRelease(count, assigned, 1));
      if (assigned == count) {
        EXPECT_EQ(ChooseRelease(jobs),
                  TransportAssignmentRelease(count, assigned, 2));
      }
    }
  }
  EXPECT_EQ(kTransportAssignmentInvalid, TransportAssignmentHole(0, 0, 0, 0));
  EXPECT_EQ(kTransportAssignmentInvalid, TransportAssignmentRelease(0, 0, 0));
}

}  // namespace
}  // namespace net
