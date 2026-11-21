// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_SOCKET_RETAINED_REQUEST_NATIVE_OWNER_ADAPTER_H_
#define NET_SOCKET_RETAINED_REQUEST_NATIVE_OWNER_ADAPTER_H_

#include <stddef.h>
#include <stdint.h>

#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/check_op.h"
#include "net/socket/retained_queue_ffi.h"

namespace net {

// The owner map is an identity index, never a second scheduling order. No
// callback is invoked inside a method or while a Rust reference is borrowed.
// Group and its requests/jobs survive and are not mutated by same-Group
// reentry while AssignJob propagates priority, matching the native contract.
template <typename Request>
class RetainedRequestOwners {
 public:
  using Token = uint64_t;
  using Owners = std::map<Token, std::unique_ptr<Request>>;
  using Detached = typename Owners::node_type;

  RetainedRequestOwners() : queue_(rust_request_queue_new()) { CHECK(queue_); }
  RetainedRequestOwners(const RetainedRequestOwners&) = delete;
  RetainedRequestOwners& operator=(const RetainedRequestOwners&) = delete;
  ~RetainedRequestOwners() = default;

  // Rust publishes before map insertion, with no callback between them.
  // Ordinary allocation failure is fail-stop as for the original list.
  RustRequestOrderReply Insert(std::unique_ptr<Request> request,
                               uint8_t priority,
                               bool respect,
                               size_t assigned,
                               size_t unassigned) {
    RustRequestOrderReply reply{};
    CHECK_EQ(0, rust_request_queue_create(queue_.get(), priority, respect, assigned,
                                          unassigned, &reply));
    CHECK(reply.token);
    CHECK(owners_.emplace(reply.token, std::move(request)).second);
    return reply;
  }

  // A no-op returns an empty node and leaves both structures untouched. A
  // changed request is absent from both order and owner lookup on return.
  // No callback runs between detach and extract. Extract allocates nothing.
  Detached DetachForPriority(Token token, uint8_t priority) {
    uint32_t detached = 0;
    CHECK_EQ(0, rust_request_queue_detach(queue_.get(), token, priority, &detached));
    CHECK_LE(detached, 1u);
    if (!detached)
      return {};
    Detached node = owners_.extract(token);
    CHECK(!node.empty());
    return node;
  }

  // The caller retains the node while performing native release/reassignment,
  // applies the original post-removal ignore-limits DCHECK, and changes the
  // native Request priority before calling Finish. Both Rust relink and node
  // reinsertion into this same map use the retained allocations. No callback
  // runs until both the order and ownership lookup are re-established.
  RustRequestOrderReply Finish(Detached node, size_t assigned, size_t unassigned) {
    CHECK(!node.empty());
    const Token token = node.key();
    RustRequestOrderReply reply{};
    CHECK_EQ(0, rust_request_queue_finish(queue_.get(), token, assigned, unassigned,
                                          &reply));
    CHECK_EQ(reply.token, token);
    CHECK(owners_.insert(std::move(node)).inserted);
    return reply;
  }

  // Call before native ReleaseJob and the original backup timer stop check.
  std::unique_ptr<Request> Remove(Token token) {
    Detached node = owners_.extract(token);
    CHECK(!node.empty());
    CHECK_EQ(0, rust_request_queue_remove(queue_.get(), token, 0));
    return std::move(node.mapped());
  }

  bool empty() const { return owners_.empty(); }
  size_t size() const { return owners_.size(); }

  template <typename Predicate>
  Token FindIf(Predicate predicate) const {
    for (const auto& [token, request] : owners_) {
      if (predicate(*request))
        return token;
    }
    return 0;
  }

  uint8_t Priority(Token token) const {
    uint8_t priority = 0;
    CHECK_EQ(0, rust_request_queue_priority(queue_.get(), token, &priority));
    return priority;
  }

  Request* Lookup(Token token) const {
    auto it = owners_.find(token);
    return it == owners_.end() ? nullptr : it->second.get();
  }

  Token At(size_t rank) const {
    Token token = 0;
    CHECK_EQ(0, rust_request_queue_at(queue_.get(), rank, &token));
    return token;
  }

  // One batch crossing, after the length query. No native callback is allowed
  // between length and snapshot under the owning sequence's normal contract.
  std::vector<Token> Snapshot() const {
    size_t count = 0;
    CHECK_EQ(0, rust_request_queue_len(queue_.get(), &count));
    // Value-initialization supplies initialized, aligned, nonaliased u64
    // storage for the Rust ABI's entire capacity, even before it writes.
    std::vector<Token> result(count, 0);
    size_t written = 0;
    CHECK_EQ(0, rust_request_queue_snapshot(queue_.get(), result.data(), count,
                                            &written));
    CHECK_EQ(count, written);
    return result;
  }

  // Snapshot only the requested ordered prefix in one Rust call. Native
  // storage is value-initialized for the full declared u64 capacity.
  std::vector<Token> SnapshotPrefix(size_t count) const {
    CHECK_LE(count, owners_.size());
    std::vector<Token> result(count, 0);
    size_t written = 0;
    CHECK_EQ(0, rust_request_queue_snapshot_prefix(queue_.get(), result.data(),
                                                   count, &written));
    CHECK_EQ(count, written);
    return result;
  }

  RustRequestOrderReply Hole(Token token, size_t assigned,
                              size_t unassigned) const {
    RustRequestOrderReply reply{};
    CHECK_EQ(0, rust_request_queue_hole(queue_.get(), token, assigned, unassigned,
                                        &reply));
    return reply;
  }

  Token Release(size_t assigned, size_t unassigned) const {
    Token token = 0;
    CHECK_EQ(0, rust_request_queue_release(queue_.get(), assigned, unassigned, &token));
    return token;
  }

 private:
  struct QueueDeleter {
    void operator()(RustRequestOrderQueue* queue) const {
      rust_request_queue_free(queue);
    }
  };
  // Reverse destruction order keeps the Rust queue alive while the native
  // ownership map is destroyed. Actual Group teardown still requires review.
  std::unique_ptr<RustRequestOrderQueue, QueueDeleter> queue_;
  Owners owners_;
};

}  // namespace net

#endif  // NET_SOCKET_RETAINED_REQUEST_NATIVE_OWNER_ADAPTER_H_
