// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Independent original PriorityQueue/Group transcription. Job IDs are mock
// native values. The priority event simulates the observable ChangePriority
// call; it does not claim a real resolver completes synchronously.
#include <algorithm>
#include <cassert>
#include <deque>
#include <iostream>
#include <map>
#include <string>
#include <vector>

struct Request {
  int id;
  int priority;
  bool respect_limits;
  int job = 0;
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
    std::string action = "none";
    if (!unassigned_.empty()) {
      action = "front";
      Assign(rank, unassigned_.front());
      unassigned_.pop_front();
    } else {
      size_t next = rank + 1;
      if (next < requests_.size() && requests_[next].job) {
        size_t current = next;
        while (current + 1 < requests_.size() && requests_[current + 1].job)
          ++current;
        action = "donor=" + std::to_string(requests_[current].id);
        int job = requests_[current].job;
        requests_[current].job = 0;
        Assign(rank, job);
      }
    }
    std::cout << "insert " << request.id << ' ' << action << ' ';
    Print();
  }
  void Remove(int id) {
    auto it = Find(id);
    assert(it != requests_.end());
    int job = it->job;
    requests_.erase(it);
    std::string action = "none";
    if (job) {
      unassigned_.push_back(job);
      size_t first = 0;
      while (first < requests_.size() && requests_[first].job)
        ++first;
      if (first < requests_.size()) {
        action = "target=" + std::to_string(requests_[first].id);
        Assign(first, unassigned_.back());
        unassigned_.pop_back();
      }
    }
    std::cout << "release " << id << ' ' << action << ' ';
    Print();
  }
  void RemoveAssignedJob(int id) {
    auto it = Find(id);
    assert(it != requests_.end() && it->job);
    const size_t hole = it - requests_.begin();
    it->job = 0;  // Job is still native-owned until reassignment finishes.
    std::string action = "none";
    if (!unassigned_.empty()) {
      action = "front";
      Assign(hole, unassigned_.front());
      unassigned_.pop_front();
    } else {
      size_t next = hole + 1;
      if (next < requests_.size() && requests_[next].job) {
        size_t current = next;
        while (current + 1 < requests_.size() && requests_[current + 1].job)
          ++current;
        action = "donor=" + std::to_string(requests_[current].id);
        int job = requests_[current].job;
        requests_[current].job = 0;
        Assign(hole, job);
      }
    }
    std::cout << "job-remove " << id << ' ' << action << ' ';
    Print();
  }
  void SamePriority(int id, int priority) {
    auto it = Find(id);
    assert(it != requests_.end() && it->priority == priority);
    std::cout << "same " << id << ' ';
    Print();
  }
 private:
  std::vector<Request>::iterator Find(int id) {
    return std::find_if(requests_.begin(), requests_.end(),
                        [id](const Request& item) { return item.id == id; });
  }
  void Assign(size_t rank, int job) {
    requests_[rank].job = job;
    if (job_priorities_[job] != requests_[rank].priority) {
      job_priorities_[job] = requests_[rank].priority;
      std::cout << "priority job=" << job << " value=" << job_priorities_[job]
                << " visible ";
      Print();
    }
  }
  void Print() const {
    std::cout << '[';
    for (size_t i = 0; i < requests_.size(); ++i) {
      if (i) std::cout << ", ";
      std::cout << requests_[i].id;
    }
    std::cout << "]\n";
  }
  std::deque<int> unassigned_ = {101, 102};
  std::map<int, int> job_priorities_ = {{101, 0}, {102, 0}};
  std::vector<Request> requests_;
};

int main() {
  Original queue;
  queue.Insert({1, 2, true});
  queue.Insert({2, 2, true});
  queue.Insert({3, 2, true});
  queue.Insert({4, 4, true});
  queue.Remove(4);       // Native changed-priority removal and release.
  queue.Insert({4, 2, true});
  queue.SamePriority(1, 2);
  queue.Remove(1);
  queue.Insert({5, 5, false});
  queue.Insert({6, 5, false});
  queue.RemoveAssignedJob(6);
  queue.SamePriority(6, 5);
}
