// Independent transcription of the original C++ decisions, frozen before the
// Rust proposal. See source-manifest.json and frozen/ for exact original bytes.
// This model does not simulate Chromium callbacks, auth providers or timers.
#ifndef NET_SOCKET_TRANSPORT_CLIENT_SOCKET_POOL_SCHEDULING_ORIGINAL_H_
#define NET_SOCKET_TRANSPORT_CLIENT_SOCKET_POOL_SCHEDULING_ORIGINAL_H_
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <deque>
#include <map>
#include <optional>
#include <vector>

namespace transport_oracle {
struct Request { int id; int priority; bool respect_limits; int job = -1; };
struct Job { int priority; };

// Descending priority; equal priorities in queue order. In Chromium the storage
// is PriorityQueue<unique_ptr<Request>> and a job is owned by the Group.
class Group {
 public:
  void Insert(Request request) {
    assert(request.job == -1);
    assert(request.respect_limits || request.priority == 5);
    auto it = requests_.begin();
    while (it != requests_.end() && it->priority > request.priority) ++it;
    if (request.respect_limits)
      while (it != requests_.end() && it->priority == request.priority) ++it;
    auto pos = static_cast<size_t>(it - requests_.begin());
    requests_.insert(it, request);
    TryToAssignJobToRequest(pos);
  }
  void AddJob(int id, int priority) {
    assert(!jobs_.count(id));
    jobs_.emplace(id, Job{priority});
    TryToAssignUnassignedJob(id);
  }
  Request Remove(int id) {
    auto it = std::find_if(requests_.begin(), requests_.end(),
                           [id](const Request& r) { return r.id == id; });
    assert(it != requests_.end());
    Request removed = *it;
    requests_.erase(it);
    if (removed.job != -1) {
      int job = removed.job;
      removed.job = -1;
      TryToAssignUnassignedJob(job);
    }
    return removed;
  }
  void SetPriority(int id, int priority) {
    auto it = std::find_if(requests_.begin(), requests_.end(),
                           [id](const Request& r) { return r.id == id; });
    assert(it != requests_.end());
    if (it->priority == priority) return;
    Request request = Remove(id);
    assert(request.respect_limits);
    request.priority = priority;
    Insert(request);
  }
  const std::vector<Request>& requests() const { return requests_; }
  const std::deque<int>& unassigned() const { return unassigned_; }
  int job_priority(int id) const { return jobs_.at(id).priority; }
 private:
  void Assign(size_t index, int job) {
    assert(requests_[index].job == -1);
    requests_[index].job = job;
    jobs_.at(job).priority = requests_[index].priority;
  }
  void TryToAssignUnassignedJob(int job) {
    unassigned_.push_back(job);
    size_t i = 0;
    while (i < requests_.size() && requests_[i].job != -1) ++i;
    if (i != requests_.size()) {
      Assign(i, unassigned_.back());
      unassigned_.pop_back();
    }
  }
  void TryToAssignJobToRequest(size_t pos) {
    if (!unassigned_.empty()) {
      Assign(pos, unassigned_.front());
      unassigned_.pop_front();
      return;
    }
    size_t next = pos + 1;
    if (next == requests_.size() || requests_[next].job == -1) return;
    size_t cur = next;
    while (cur + 1 < requests_.size() && requests_[cur + 1].job != -1) ++cur;
    int job = requests_[cur].job;
    requests_[cur].job = -1;
    Assign(pos, job);
  }
  std::vector<Request> requests_;
  std::deque<int> unassigned_;
  std::map<int, Job> jobs_;
};

// Input is already in the native GroupId map order. Eligibility is computed
// by native has_unbound_requests() and CanUseAdditionalSocketSlot().
struct StalledGroup { bool eligible; int top_priority; };
inline std::optional<size_t> FindTopStalledGroup(
    const std::vector<StalledGroup>& groups) {
  std::optional<size_t> top;
  for (size_t i = 0; i < groups.size(); ++i) {
    if (!groups[i].eligible) continue;
    if (!top || groups[i].top_priority > groups[*top].top_priority) top = i;
  }
  return top;
}
}  // namespace transport_oracle
#endif
