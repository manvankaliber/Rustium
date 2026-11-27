// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Independent original-loop oracle; see source-identity.json for source pins.
#ifndef NET_SOCKET_TRANSPORT_ASSIGNMENT_ORIGINAL_ACTIONS_H_
#define NET_SOCKET_TRANSPORT_ASSIGNMENT_ORIGINAL_ACTIONS_H_

#include <stddef.h>

#include <limits>
#include <vector>

namespace transport_assignment_original {
inline constexpr size_t kNone = std::numeric_limits<size_t>::max();
inline constexpr size_t kFront = kNone - 1;

// The hole represents the just-inserted request or the request whose job was
// released by RemoveUnboundJob. Other entries are in original queue order.
inline size_t ChooseHole(const std::vector<bool>& jobs, size_t hole,
                         size_t unassigned_jobs) {
  if (unassigned_jobs)
    return kFront;
  size_t next = hole + 1;
  if (next == jobs.size() || !jobs[next])
    return kNone;
  size_t current = next;
  while (current + 1 < jobs.size() && jobs[current + 1])
    ++current;
  return current;
}
inline size_t ChooseRelease(const std::vector<bool>& jobs) {
  size_t i = 0;
  while (i < jobs.size() && jobs[i])
    ++i;
  return i == jobs.size() ? kNone : i;
}
}  // namespace transport_assignment_original
#endif  // NET_SOCKET_TRANSPORT_ASSIGNMENT_ORIGINAL_ACTIONS_H_
