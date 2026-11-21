// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_SOCKET_TRANSPORT_CLIENT_SOCKET_POOL_SCHEDULING_ORIGINAL_H_
#define NET_SOCKET_TRANSPORT_CLIENT_SOCKET_POOL_SCHEDULING_ORIGINAL_H_

#include <cstddef>
#include <optional>
#include <vector>

namespace transport_oracle {

struct StalledGroup {
  bool eligible;
  int top_priority;
};

// Transcription of the frozen native selection. The input is already in
// GroupId map order with eligibility evaluated by the native pool.
inline std::optional<size_t> FindTopStalledGroup(
    const std::vector<StalledGroup>& groups) {
  std::optional<size_t> top;
  for (size_t i = 0; i < groups.size(); ++i) {
    if (!groups[i].eligible) {
      continue;
    }
    if (!top || groups[i].top_priority > groups[*top].top_priority) {
      top = i;
    }
  }
  return top;
}

}  // namespace transport_oracle

#endif  // NET_SOCKET_TRANSPORT_CLIENT_SOCKET_POOL_SCHEDULING_ORIGINAL_H_
