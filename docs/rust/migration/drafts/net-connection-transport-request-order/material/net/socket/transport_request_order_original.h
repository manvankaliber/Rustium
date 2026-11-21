// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Independent native-order model transcribed from the frozen original queue,
// Group::InsertUnboundRequest, Group::SetPriority and preconnect accounting.
#ifndef NET_SOCKET_TRANSPORT_REQUEST_ORDER_ORIGINAL_H_
#define NET_SOCKET_TRANSPORT_REQUEST_ORDER_ORIGINAL_H_

#include <stddef.h>

#include <algorithm>
#include <cassert>
#include <vector>

namespace transport_order_original {
struct Request {
  int id;
  int priority;
  bool respect_limits;
};

class Model {
 public:
  void Insert(Request request) {
    assert(request.respect_limits || request.priority == 5);
    auto it = requests_.begin();
    while (it != requests_.end() && it->priority > request.priority)
      ++it;
    if (request.respect_limits) {
      while (it != requests_.end() && it->priority == request.priority)
        ++it;
    }
    requests_.insert(it, request);
  }

  void SetPriority(int id, int priority) {
    auto it = std::find_if(requests_.begin(), requests_.end(),
                           [id](const Request& item) { return item.id == id; });
    assert(it != requests_.end());
    if (it->priority == priority)
      return;
    Request request = *it;
    requests_.erase(it);
    assert(request.respect_limits);
    request.priority = priority;
    Insert(request);
  }

  void Erase(int id) {
    auto it = std::find_if(requests_.begin(), requests_.end(),
                           [id](const Request& item) { return item.id == id; });
    assert(it != requests_.end());
    requests_.erase(it);
  }
  void AddJob(bool preconnect) {
    if (preconnect)
      ++never_claimed_;
    ++jobs_;
  }
  bool ClaimPreconnect() {
    if (!never_claimed_)
      return false;
    --never_claimed_;
    return true;
  }
  void RemoveJob() {
    assert(jobs_);
    --jobs_;
    if (jobs_ < never_claimed_)
      never_claimed_ = jobs_;
  }
  void RemoveAllJobs() { jobs_ = never_claimed_ = 0; }
  const std::vector<Request>& requests() const { return requests_; }
  size_t never_claimed() const { return never_claimed_; }

 private:
  std::vector<Request> requests_;
  size_t jobs_ = 0;
  size_t never_claimed_ = 0;
};
}  // namespace transport_order_original

#endif  // NET_SOCKET_TRANSPORT_REQUEST_ORDER_ORIGINAL_H_
