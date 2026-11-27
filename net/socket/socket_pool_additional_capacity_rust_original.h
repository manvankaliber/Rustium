// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_SOCKET_SOCKET_POOL_ADDITIONAL_CAPACITY_RUST_ORIGINAL_H_
#define NET_SOCKET_SOCKET_POOL_ADDITIONAL_CAPACITY_RUST_ORIGINAL_H_

#include <string>

#include "base/strings/stringprintf.h"
#include "net/base/net_export.h"

// All family reference objects link together in one test-only source_set.
#define CONNECTION_JOBS_ORIGINAL_PRIVATE_EXPORT
#define CONNECTION_JOBS_ORIGINAL_EXPORT

namespace net {

// Socket pools update their expandability before every socket allocation and
// release.
enum class SocketPoolExpandabilityRustOriginal {

  // Uncapped pools can allocate or release sockets.
  kUncapped,

  // Capped pools cannot allocate sockets, but can release them.
  kCapped
};

// This class encapsulates the logic for the additional TCP Socket Pool capacity
// allocated (and randomized) to prevent cross-site expandability tracking.
// Stored on `ClientSocketPool` subclasses and used in capacity calculations.
// See crbug.com/415691664 for more details.
class CONNECTION_JOBS_ORIGINAL_PRIVATE_EXPORT SocketPoolAdditionalCapacityRustOriginal {
 public:
  // This initializes using values from kTcpSocketPoolLimitRandomization.
  // `additional_capacity` is the maximum amount of sockets (on top of the
  // `socket_soft_cap`) allowed to be allocated. Usually `additional_capacity`
  // equals `socket_soft_cap`, but this is not enforced.
  static SocketPoolAdditionalCapacityRustOriginal Create(size_t additional_capacity);

  // This initializes an empty pool that contains no capacity.
  static SocketPoolAdditionalCapacityRustOriginal CreateEmpty();

  static SocketPoolAdditionalCapacityRustOriginal CreateForTest(double base,
                                                    size_t capacity,
                                                    double minimum,
                                                    double noise);

  // Calculates the next `SocketPoolExpandability` before the allocation of a
  // socket. `sockets_in_use` should be counted pre-allocation and
  // `socket_soft_cap` is likely being passed down from
  // `g_max_sockets_per_pool`.
  SocketPoolExpandabilityRustOriginal NextExpandabilityBeforeAllocation(
      SocketPoolExpandabilityRustOriginal current_expandability,
      size_t sockets_in_use,
      size_t socket_soft_cap) const;

  // Calculates the next `SocketPoolExpandability` after the release of a
  // socket. `sockets_in_use` should be counted post-release and
  // `socket_soft_cap` is likely being passed down from
  // `g_max_sockets_per_pool`.
  SocketPoolExpandabilityRustOriginal NextExpandabilityAfterRelease(
      SocketPoolExpandabilityRustOriginal current_expandability,
      size_t sockets_in_use,
      size_t socket_soft_cap) const;

  size_t capacity() const { return capacity_; }

  explicit operator std::string() const {
    return base::StringPrintf(
        "SocketPoolAdditionalCapacity(base:%e,capacity:%i,minimum:%e,noise:%e)",
        base_, capacity_, minimum_, noise_);
  }

  friend bool operator==(const SocketPoolAdditionalCapacityRustOriginal& lhs,
                         const SocketPoolAdditionalCapacityRustOriginal& rhs) {
    return lhs.base_ == rhs.base_ && lhs.capacity_ == rhs.capacity_ &&
           lhs.minimum_ == rhs.minimum_ && lhs.noise_ == rhs.noise_;
  }

 private:
  enum class SocketPoolAction { kAllocation, kRelease };

  static void LogExpandabilityTransition(
      SocketPoolAction action,
      SocketPoolExpandabilityRustOriginal current_expandability,
      SocketPoolExpandabilityRustOriginal next_expandability,
      size_t sockets_in_use);

  SocketPoolAdditionalCapacityRustOriginal() = default;
  SocketPoolAdditionalCapacityRustOriginal(double base,
                               size_t capacity,
                               double minimum,
                               double noise);

  // Helper for NextExpandabilityBeforeAllocation to avoid duplicate logging
  // code.
  SocketPoolExpandabilityRustOriginal NextExpandabilityBeforeAllocationImpl(
      SocketPoolExpandabilityRustOriginal current_expandability,
      size_t sockets_in_use,
      size_t socket_soft_cap) const;

  // Helper for NextExpandabilityAfterRelease to avoid duplicate logging code.
  SocketPoolExpandabilityRustOriginal NextExpandabilityAfterReleaseImpl(
      SocketPoolExpandabilityRustOriginal current_expandability,
      size_t sockets_in_use,
      size_t socket_soft_cap) const;

  // This helper function for `NextExpandabilityBefore(Allocation|Release)`
  // handles common logic. Returns a SocketPoolExpandability if the common logic
  // is controlling, and std::nullopt otherwise.
  std::optional<SocketPoolExpandabilityRustOriginal> NextExpandabilityCommonImpl(
      size_t sockets_in_use,
      size_t socket_soft_cap) const;

  // Unlike other functions in this class, this one will CHECK on invalid
  // constants and inputs. As such, all validation must be performed before we
  // get to this stage. The actual way this function rolls dice are quite
  // complex, please see the implementation for details.
  // `actions_taken` must be between 0 and `capacity_`, and is the
  // amount of `capacity_` already allocated for
  // `NextExpandabilityBeforeAllocationImpl` and the amount of `capacity_` free
  // for `NextExpandabilityAfterReleaseImpl`. This is done to ensure the
  // probability converges toward 1 correctly for each.
  bool ShouldTransitionExpandability(SocketPoolAction action,
                                     size_t actions_taken) const;

  // See the implementation of `ShouldTransitionExpandability` for how these
  // constants are used and bound in calculating the probability of a
  // expandability transition.
  double base_ = 0.0;
  size_t capacity_ = 0;
  double minimum_ = 0.0;
  double noise_ = 0.0;
};

}  // namespace net

#endif  // NET_SOCKET_SOCKET_POOL_ADDITIONAL_CAPACITY_RUST_ORIGINAL_H_
