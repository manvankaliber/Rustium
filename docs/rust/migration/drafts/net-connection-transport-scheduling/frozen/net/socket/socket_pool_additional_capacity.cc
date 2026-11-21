// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/raw_ptr.h"
#include "net/socket/socket_pool_additional_capacity.h"
#include <bit>
#include <cmath>
#include "net/socket/connection_jobs_upper_rust_ffi.h"
#include "base/check.h"

#include "base/feature_list.h"
#include "base/metrics/histogram_functions.h"
#include "base/rand_util.h"
#include "net/base/features.h"

namespace net {

static constexpr char kErrorHistogramName[] =
    "Net.TcpSocketPoolLimitRandomization.Error";

// These values are persisted to logs, entries should not be renumbered.
// LINT.IfChange(SocketPoolAdditionalCapacityError)
enum class SocketPoolAdditionalCapacityError {
  kInvalidBase = 0,
  kInvalidCapacity = 1,
  kInvalidMinimum = 2,
  kInvalidNoise = 3,
  kInvalidSocketSoftCap = 4,
  kInvalidSocketInUse = 5,
  kInvalidSocketAllocation = 6,
  kMaxValue = kInvalidSocketAllocation,
};
// LINT.ThenChange(//tools/metrics/histograms/metadata/net/enums.xml:SocketPoolAdditionalCapacityError)



// Typed storage/provider primitives only. Sequencing/branches live in safe Rust.
struct SocketPoolAdditionalCapacityFamilyRustBridge final
    : connection_jobs_upper_rust::Invocation {
  using Class = SocketPoolAdditionalCapacity;

  explicit SocketPoolAdditionalCapacityFamilyRustBridge(Class* job)
      : Invocation(job), mutable_owner(job), owner(job) { Register();  }
  explicit SocketPoolAdditionalCapacityFamilyRustBridge(const Class* job)
      : Invocation(job), mutable_owner(nullptr), owner(job) { Register();  }
  SocketPoolAdditionalCapacityFamilyRustBridge()
      : Invocation(this), mutable_owner(nullptr), owner(nullptr) { Register(); }
  ~SocketPoolAdditionalCapacityFamilyRustBridge() {  Unregister(); }
  Class& M() { CHECK(Live()); CHECK(mutable_owner); return *mutable_owner; }
  const Class& R() const { CHECK(Live()); CHECK(owner); return *owner; }
  int64_t Run(uint32_t operation, int64_t argument = 0) {
    return connection_jobs_upper_rust::connection_jobs_rust_run(
        12, operation, argument, this, Service);
  }
  const raw_ptr<Class> mutable_owner;
  const raw_ptr<const Class> owner;


  size_t in_use=0,soft_cap=0,capacity_input=0,actions_taken=0;
  SocketPoolExpandability current=SocketPoolExpandability::kUncapped;
  SocketPoolExpandability next=SocketPoolExpandability::kUncapped;
  int action_input=0;
  std::optional<Class> output;
  raw_ptr<const Class> other=nullptr;
  double percentage=0.0;
  double probability=0.0;
  std::string histogram;

  static bool Service(void* context, uint32_t module, uint32_t action, int64_t argument,
                      int64_t* value) {
    auto& c = *static_cast<SocketPoolAdditionalCapacityFamilyRustBridge*>(context);
    *value = 0;

    if (!c.Live() || module != 12) { return false; }
    if(action==UINT32_MAX){*value=1;return true;}
    switch (action) {
      case 0: {  // FEATURE
*value=base::FeatureList::IsEnabled(features::kTcpSocketPoolLimitRandomization);
        break;
      }
      case 1: {  // CREATE_FEATURE
c.output.emplace(Class(features::kTcpSocketPoolLimitRandomizationBase.Get(),c.capacity_input,
features::kTcpSocketPoolLimitRandomizationMinimum.Get(),features::kTcpSocketPoolLimitRandomizationNoise.Get()));
        break;
      }
      case 2: {  // CREATE_EMPTY
c.output.emplace(Class());
        break;
      }
      case 3: {  // BASE_BITS
*value=std::bit_cast<int64_t>(c.R().base_);
        break;
      }
      case 4: {  // CAPACITY
*value=static_cast<int64_t>(c.R().capacity_);
        break;
      }
      case 5: {  // MINIMUM_BITS
*value=std::bit_cast<int64_t>(c.R().minimum_);
        break;
      }
      case 6: {  // NOISE_BITS
*value=std::bit_cast<int64_t>(c.R().noise_);
        break;
      }
      case 7: {  // ERROR
base::UmaHistogramEnumeration(kErrorHistogramName,static_cast<SocketPoolAdditionalCapacityError>(argument));
        break;
      }
      case 8: {  // CLEAR_BASE
c.M().base_=0.0;
        break;
      }
      case 9: {  // CLEAR_CAPACITY
c.M().capacity_=0;
        break;
      }
      case 10: {  // CLEAR_MINIMUM
c.M().minimum_=0.0;
        break;
      }
      case 11: {  // CLEAR_NOISE
c.M().noise_=0.0;
        break;
      }
      case 12: {  // IN_USE
*value=static_cast<int64_t>(c.in_use);
        break;
      }
      case 13: {  // SOFT_CAP
*value=static_cast<int64_t>(c.soft_cap);
        break;
      }
      case 14: {  // CURRENT
*value=static_cast<int>(c.current);
        break;
      }
      case 15: {  // CHECK_TRANSITION
CHECK_GE(c.R().base_,0.0);CHECK_LE(c.R().base_,1.0);CHECK_LE(c.R().capacity_,256u);
CHECK_GE(c.R().minimum_,0.0);CHECK_LE(c.R().minimum_,1.0);CHECK_GE(c.R().noise_,0.0);CHECK_LE(c.R().noise_,1.0);
CHECK_LE(static_cast<size_t>(argument),c.R().capacity_);
        break;
      }
      case 16: {  // RANDOM_DOUBLE
*value=std::bit_cast<int64_t>(base::RandDouble());
        break;
      }
      case 17: {  // RANDOM_BOOL
*value=base::RandBool();
        break;
      }
      case 18: {  // SET_PERCENTAGE
c.percentage=std::bit_cast<double>(argument);
        break;
      }
      case 19: {  // POWER
*value=std::bit_cast<int64_t>(std::pow(c.R().base_,c.percentage));
        break;
      }
      case 20: {  // SET_PROBABILITY
c.probability=std::bit_cast<double>(argument);
        break;
      }
      case 21: {  // PERCENT_ALLOCATE
base::UmaHistogramPercentage("Net.TcpSocketPoolLimitRandomization.Probability.Allocation",static_cast<int>(c.probability*100));
        break;
      }
      case 22: {  // PERCENT_RELEASE
base::UmaHistogramPercentage("Net.TcpSocketPoolLimitRandomization.Probability.Release",static_cast<int>(c.probability*100));
        break;
      }
      case 23: {  // ACTION
*value=c.action_input;
        break;
      }
      case 24: {  // ACTIONS_TAKEN
*value=static_cast<int64_t>(c.actions_taken);
        break;
      }
      case 25: {  // STORE_NEXT
c.next=static_cast<SocketPoolExpandability>(argument);
        break;
      }
      case 26: {  // NEXT
*value=static_cast<int>(c.next);
        break;
      }
      case 27: {  // TRANSITION_HISTOGRAM
base::UmaHistogramCounts1000(c.histogram,c.in_use);
        break;
      }
      case 28: {  // HISTOGRAM_PREFIX
c.histogram="Net.TcpSocketPoolLimitRandomization.Transition.";
        break;
      }
      case 29: {  // HISTOGRAM_ALLOCATE
c.histogram+="Allocation.";
        break;
      }
      case 30: {  // HISTOGRAM_RELEASE
c.histogram+="Release.";
        break;
      }
      case 31: {  // HISTOGRAM_FROM_UNCAPPED
c.histogram+="UncappedTo";
        break;
      }
      case 32: {  // HISTOGRAM_FROM_CAPPED
c.histogram+="CappedTo";
        break;
      }
      case 33: {  // HISTOGRAM_TO_UNCAPPED
c.histogram+="Uncapped";
        break;
      }
      case 34: {  // HISTOGRAM_TO_CAPPED
c.histogram+="Capped";
        break;
      }
      case 35: {  // OTHER_BASE
*value=std::bit_cast<int64_t>(c.other->base_);
        break;
      }
      case 36: {  // OTHER_CAPACITY
*value=static_cast<int64_t>(c.other->capacity_);
        break;
      }
      case 37: {  // OTHER_MINIMUM
*value=std::bit_cast<int64_t>(c.other->minimum_);
        break;
      }
      case 38: {  // OTHER_NOISE
*value=std::bit_cast<int64_t>(c.other->noise_);
        break;
      }
      default: return false;
    }
    // Only invocation-local liveness is read after any deleting callback.
    return c.Live();
  }
};
// static
SocketPoolAdditionalCapacity SocketPoolAdditionalCapacity::Create(
    size_t additional_capacity) {
  SocketPoolAdditionalCapacityFamilyRustBridge c;c.capacity_input=additional_capacity;c.Run(0);return std::move(*c.output);
}

SocketPoolAdditionalCapacity SocketPoolAdditionalCapacity::CreateEmpty() {
  SocketPoolAdditionalCapacityFamilyRustBridge c;c.Run(1);return std::move(*c.output);
}

// static
SocketPoolAdditionalCapacity SocketPoolAdditionalCapacity::CreateForTest(
    double base,
    size_t capacity,
    double minimum,
    double noise) {
  return SocketPoolAdditionalCapacity(base, capacity, minimum, noise);
}

SocketPoolExpandability
SocketPoolAdditionalCapacity::NextExpandabilityBeforeAllocation(
    SocketPoolExpandability current_expandability,
    size_t sockets_in_use,
    size_t socket_soft_cap) const {
  SocketPoolAdditionalCapacityFamilyRustBridge c(this);c.current=current_expandability;c.in_use=sockets_in_use;c.soft_cap=socket_soft_cap;c.action_input=0;
  return static_cast<SocketPoolExpandability>(c.Run(3));
}

SocketPoolExpandability
SocketPoolAdditionalCapacity::NextExpandabilityAfterRelease(
    SocketPoolExpandability current_expandability,
    size_t sockets_in_use,
    size_t socket_soft_cap) const {
  SocketPoolAdditionalCapacityFamilyRustBridge c(this);c.current=current_expandability;c.in_use=sockets_in_use;c.soft_cap=socket_soft_cap;c.action_input=1;
  return static_cast<SocketPoolExpandability>(c.Run(4));
}

// static
void SocketPoolAdditionalCapacity::LogExpandabilityTransition(
    SocketPoolAction action,
    SocketPoolExpandability current_expandability,
    SocketPoolExpandability next_expandability,
    size_t sockets_in_use) {
  SocketPoolAdditionalCapacityFamilyRustBridge c;c.action_input=static_cast<int>(action);c.current=current_expandability;c.next=next_expandability;c.in_use=sockets_in_use;c.Run(9);
}

SocketPoolAdditionalCapacity::SocketPoolAdditionalCapacity(double base,
                                                           size_t capacity,
                                                           double minimum,
                                                           double noise)
    : base_(base), capacity_(capacity), minimum_(minimum), noise_(noise) {
  SocketPoolAdditionalCapacityFamilyRustBridge c(this);c.Run(2);
}

SocketPoolExpandability
SocketPoolAdditionalCapacity::NextExpandabilityBeforeAllocationImpl(
    SocketPoolExpandability current_expandability,
    size_t sockets_in_use,
    size_t socket_soft_cap) const {
  SocketPoolAdditionalCapacityFamilyRustBridge c(this);c.current=current_expandability;c.in_use=sockets_in_use;c.soft_cap=socket_soft_cap;c.action_input=0;
  return static_cast<SocketPoolExpandability>(c.Run(5));
}

SocketPoolExpandability
SocketPoolAdditionalCapacity::NextExpandabilityAfterReleaseImpl(
    SocketPoolExpandability current_expandability,
    size_t sockets_in_use,
    size_t socket_soft_cap) const {
  SocketPoolAdditionalCapacityFamilyRustBridge c(this);c.current=current_expandability;c.in_use=sockets_in_use;c.soft_cap=socket_soft_cap;c.action_input=1;
  return static_cast<SocketPoolExpandability>(c.Run(6));
}

std::optional<SocketPoolExpandability>
SocketPoolAdditionalCapacity::NextExpandabilityCommonImpl(
    size_t sockets_in_use,
    size_t socket_soft_cap) const {
  SocketPoolAdditionalCapacityFamilyRustBridge c(this);c.in_use=sockets_in_use;c.soft_cap=socket_soft_cap;
  const auto result=c.Run(7);if(result<0)return std::nullopt;return static_cast<SocketPoolExpandability>(result);
}

bool SocketPoolAdditionalCapacity::ShouldTransitionExpandability(
    SocketPoolAction action,
    size_t actions_taken) const {
  SocketPoolAdditionalCapacityFamilyRustBridge c(this);c.action_input=static_cast<int>(action);c.actions_taken=actions_taken;return c.Run(8)!=0;
}

bool operator==(const SocketPoolAdditionalCapacity& lhs,const SocketPoolAdditionalCapacity& rhs){
 SocketPoolAdditionalCapacityFamilyRustBridge c(&lhs);c.other=&rhs;return c.Run(10)!=0;
}

}  // namespace net
