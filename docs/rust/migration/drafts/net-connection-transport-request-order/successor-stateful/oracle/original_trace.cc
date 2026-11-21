// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Independent executable transcription of original PriorityQueue ordering,
// Group::TryToAssignJobToRequest and release-to-first-unassigned loops.
// Native providers, callbacks, auth, timers and ownership are not simulated.
#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>

struct Request {
  int id;
  int priority;
  bool respect_limits;
  bool job = false;
};

class Original {
 public:
  void Insert(Request request) {
    auto position = requests_.begin();
    while (position != requests_.end() && position->priority > request.priority)
      ++position;
    if (request.respect_limits) {
      while (position != requests_.end() && position->priority == request.priority)
        ++position;
    }
    const size_t rank = position - requests_.begin();
    requests_.insert(position, request);
    std::cout << "insert " << request.id << ' ';
    if (unassigned_) {
      requests_[rank].job = true;
      --unassigned_;
      std::cout << "front ";
    } else {
      size_t next = rank + 1;
      if (next == requests_.size() || !requests_[next].job) {
        std::cout << "none ";
      } else {
        size_t current = next;
        while (current + 1 < requests_.size() && requests_[current + 1].job)
          ++current;
        std::cout << "donor=" << requests_[current].id << ' ';
        requests_[current].job = false;
        requests_[rank].job = true;
      }
    }
    Print();
  }
  void Remove(int id) {
    auto it = std::find_if(requests_.begin(), requests_.end(),
                           [id](const Request& request) { return request.id == id; });
    assert(it != requests_.end());
    const bool had_job = it->job;
    requests_.erase(it);
    std::cout << "release " << id << ' ';
    if (had_job) {
      ++unassigned_;
      size_t first = 0;
      while (first < requests_.size() && requests_[first].job)
        ++first;
      if (first < requests_.size()) {
        requests_[first].job = true;
        --unassigned_;
        std::cout << "target=" << requests_[first].id << ' ';
      } else {
        std::cout << "none ";
      }
    } else {
      std::cout << "none ";
    }
    Print();
  }
  void RemoveAssignedJob(int id) {
    auto it = std::find_if(requests_.begin(), requests_.end(),
                           [id](const Request& request) { return request.id == id; });
    assert(it != requests_.end() && it->job);
    const size_t hole = it - requests_.begin();
    it->job = false;
    std::cout << "job-remove " << id << ' ';
    if (unassigned_) {
      it->job = true;
      --unassigned_;
      std::cout << "front ";
    } else {
      size_t next = hole + 1;
      if (next == requests_.size() || !requests_[next].job) {
        std::cout << "none ";
      } else {
        size_t current = next;
        while (current + 1 < requests_.size() && requests_[current + 1].job)
          ++current;
        std::cout << "donor=" << requests_[current].id << ' ';
        requests_[current].job = false;
        requests_[hole].job = true;
      }
    }
    Print();
  }
  void SamePriority(int id, int priority) {
    auto it = std::find_if(requests_.begin(), requests_.end(),
                           [id](const Request& request) { return request.id == id; });
    assert(it != requests_.end() && it->priority == priority);
    std::cout << "same " << id << ' ';
    Print();
  }
 private:
  void Print() const {
    std::cout << '[';
    for (size_t i = 0; i < requests_.size(); ++i) {
      if (i) std::cout << ", ";
      std::cout << requests_[i].id;
    }
    std::cout << "]\n";
  }
  // Represents two existing unassigned preconnect jobs.
  size_t unassigned_ = 2;
  std::vector<Request> requests_;
};

int main() {
  Original queue;
  queue.Insert({1, 2, true});
  queue.Insert({2, 2, true});
  queue.Insert({3, 2, true});
  queue.Insert({4, 4, true});
  queue.Remove(4);        // changed priority: release/reassign before reinsert
  queue.Insert({4, 2, true});
  queue.SamePriority(1, 2);
  queue.Remove(1);
  queue.Insert({5, 5, false});
  queue.Insert({6, 5, false});
  queue.RemoveAssignedJob(6);
  queue.SamePriority(6, 5);
}
