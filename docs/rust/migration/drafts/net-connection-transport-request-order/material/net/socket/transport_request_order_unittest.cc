// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/socket/transport_request_order_ffi.h"

#include <stddef.h>
#include <stdint.h>

#include <algorithm>
#include <limits>
#include <vector>

#include "net/socket/transport_request_order_original.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {
namespace {

using transport_order_original::Model;
using transport_order_original::Request;

class ProjectedOrder {
 public:
  void Insert(Request request) {
    size_t higher = 0;
    size_t same = 0;
    for (const Request& item : requests_) {
      higher += item.priority > request.priority;
      same += item.priority == request.priority;
    }
    size_t plan = TransportRequestInsertPlan(request.respect_limits ? 1 : 0,
                                             higher, same);
    ASSERT_NE(plan, std::numeric_limits<size_t>::max());
    ASSERT_LE(plan >> 1, requests_.size());
    EXPECT_EQ((plan & 1u) != 0, !request.respect_limits);
    requests_.insert(requests_.begin() + (plan >> 1), request);
  }
  void SetPriority(int id, int priority) {
    auto it = std::find_if(requests_.begin(), requests_.end(),
                           [id](const Request& item) { return item.id == id; });
    ASSERT_NE(it, requests_.end());
    const int32_t action = TransportRequestReprioritize(it->priority, priority);
    ASSERT_TRUE(action == 0 || action == 1);
    if (action == 0)
      return;
    Request item = *it;
    requests_.erase(it);
    item.priority = priority;
    Insert(item);
  }
  const std::vector<Request>& requests() const { return requests_; }

 private:
  std::vector<Request> requests_;
};

void ExpectSame(const Model& reference, const ProjectedOrder& projected) {
  ASSERT_EQ(reference.requests().size(), projected.requests().size());
  for (size_t i = 0; i < reference.requests().size(); ++i) {
    EXPECT_EQ(reference.requests()[i].id, projected.requests()[i].id);
    EXPECT_EQ(reference.requests()[i].priority,
              projected.requests()[i].priority);
    EXPECT_EQ(reference.requests()[i].respect_limits,
              projected.requests()[i].respect_limits);
  }
}

TEST(TransportRequestOrderPolicyTest, InsertAndReprioritizeSequences) {
  for (int seed = 0; seed < 48; ++seed) {
    Model reference;
    ProjectedOrder projected;
    for (int id = 0; id < 12; ++id) {
      const bool respect = (id + seed) % 5 != 0;
      Request request{id, respect ? (id * 3 + seed) % 6 : 5, respect};
      reference.Insert(request);
      projected.Insert(request);
      ExpectSame(reference, projected);
    }
    for (int id = 0; id < 12; ++id) {
      auto it = std::find_if(reference.requests().begin(),
                             reference.requests().end(),
                             [id](const Request& item) { return item.id == id; });
      ASSERT_NE(it, reference.requests().end());
      const int current = it->priority;
      reference.SetPriority(id, current);
      projected.SetPriority(id, current);
      ExpectSame(reference, projected);
      if (!it->respect_limits)
        continue;
      const int next = (current + seed + 1) % 6;
      reference.SetPriority(id, next);
      projected.SetPriority(id, next);
      ExpectSame(reference, projected);
    }
  }
}

TEST(TransportRequestOrderPolicyTest, EncodesNativeFrontAndBackRanks) {
  EXPECT_EQ(7u, TransportRequestInsertPlan(0, 3, 2));
  EXPECT_EQ(10u, TransportRequestInsertPlan(1, 3, 2));
  EXPECT_EQ(10u, TransportRequestInsertPlan(2, 3, 2));
  EXPECT_EQ(std::numeric_limits<size_t>::max(),
            TransportRequestInsertPlan(1, std::numeric_limits<size_t>::max(),
                                       1));
  EXPECT_EQ(0, TransportRequestReprioritize(3, 3));
  EXPECT_EQ(1, TransportRequestReprioritize(3, 4));
}

}  // namespace
}  // namespace net
