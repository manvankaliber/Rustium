// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/backoff_entry.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "base/check_op.h"
#include "base/memory/stack_allocated.h"
#include "base/numerics/clamped_math.h"
#include "base/numerics/safe_math.h"
#include "base/rand_util.h"
#include "base/time/tick_clock.h"
#include "net/base/backoff_rust_ffi.h"

namespace {
struct BackoffRustContext {
  STACK_ALLOCATED();

 public:
  BackoffRustContext(const net::BackoffEntry& e,
                     const net::BackoffEntry::Policy& p,
                     int* count = nullptr)
      : entry(e), policy(p), failures(count) {}
  const net::BackoffEntry& entry;
  const net::BackoffEntry::Policy& policy;
  int* failures;
};
}  // namespace
extern "C" int64_t backoff_rust_service_now(const void* ctx) {
  return static_cast<const BackoffRustContext*>(ctx)
      ->entry.GetTimeTicksNow()
      .ToInternalValue();
}
#if defined(BACKOFF_RUST_DETERMINISTIC_TEST)
extern double backoff_rust_test_random();
#endif
extern "C" void backoff_rust_service_publish(const void* ctx,
                                             int32_t failures) {
  *static_cast<const BackoffRustContext*>(ctx)->failures = failures;
}
extern "C" BackoffRustEntry backoff_rust_service_entry(const void* ctx) {
  const auto& entry = static_cast<const BackoffRustContext*>(ctx)->entry;
  return {entry.failure_count(), entry.GetReleaseTime().ToInternalValue()};
}
extern "C" int64_t backoff_rust_service_maximum(const void* ctx) {
  return static_cast<const BackoffRustContext*>(ctx)->policy.maximum_backoff_ms;
}
extern "C" int64_t backoff_rust_service_lifetime(const void* ctx) {
  return static_cast<const BackoffRustContext*>(ctx)->policy.entry_lifetime_ms;
}
extern "C" double backoff_rust_service_random() {
#if defined(BACKOFF_RUST_DETERMINISTIC_TEST)
  return backoff_rust_test_random();
#else
  return base::RandDouble();
#endif
}
extern "C" double backoff_rust_service_pow(double base, int32_t exponent) {
  return std::pow(base, exponent);
}
extern "C" int64_t backoff_rust_service_add(int64_t tick, int64_t delta) {
  return ((base::TimeTicks() + base::Microseconds(tick)) +
          base::Microseconds(delta))
      .ToInternalValue();
}
extern "C" int64_t backoff_rust_service_subtract(int64_t a, int64_t b) {
  return ((base::TimeTicks() + base::Microseconds(a)) -
          (base::TimeTicks() + base::Microseconds(b)))
      .InMicroseconds();
}
extern "C" int64_t backoff_rust_service_milliseconds(int64_t a) {
  return base::Milliseconds(a).InMicroseconds();
}
extern "C" int64_t backoff_rust_service_in_milliseconds(int64_t a) {
  return base::Microseconds(a).InMilliseconds();
}
extern "C" int64_t backoff_rust_service_checked_delay(double value) {
  base::CheckedNumeric<int64_t> result = value;
  result *= base::Time::kMicrosecondsPerMillisecond;
  return result.ValueOrDefault(std::numeric_limits<int64_t>::max());
}
namespace {
BackoffRustPolicy RustPolicy(const net::BackoffEntry::Policy& p) {
  return {p.num_errors_to_ignore,
          p.initial_delay_ms,
          p.multiply_factor,
          p.jitter_factor,
          p.maximum_backoff_ms,
          p.entry_lifetime_ms,
          static_cast<uint8_t>(p.always_use_initial_delay)};
}
}  // namespace

namespace net {

BackoffEntry::BackoffEntry(const BackoffEntry::Policy* policy)
    : BackoffEntry(policy, nullptr) {}

BackoffEntry::BackoffEntry(const BackoffEntry::Policy* policy,
                           const base::TickClock* clock)
    : policy_(policy), clock_(clock) {
  DCHECK(policy_);
  Reset();
}

BackoffEntry::~BackoffEntry() {
  // TODO(joi): Enable this once our clients (e.g. URLRequestThrottlerManager)
  // always destroy from the I/O thread.
  // DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
}

void BackoffEntry::InformOfRequest(bool succeeded) {
  const BackoffRustContext context{*this, *policy_, &failure_count_};
  auto state = backoff_rust_inform(
      {failure_count_, exponential_backoff_release_time_.ToInternalValue()},
      RustPolicy(*policy_), succeeded, &context);
  failure_count_ = state.failures;
  exponential_backoff_release_time_ =
      base::TimeTicks() + base::Microseconds(state.release);
}

bool BackoffEntry::ShouldRejectRequest() const {
  const BackoffRustContext context{*this, *policy_};
  return backoff_rust_reject(
      {failure_count_, exponential_backoff_release_time_.ToInternalValue()},
      &context);
}

base::TimeDelta BackoffEntry::GetTimeUntilRelease() const {
  const BackoffRustContext context{*this, *policy_};
  return base::Microseconds(backoff_rust_until(
      {failure_count_, exponential_backoff_release_time_.ToInternalValue()},
      &context));
}

base::TimeTicks BackoffEntry::GetReleaseTime() const {
  return exponential_backoff_release_time_;
}

void BackoffEntry::SetCustomReleaseTime(const base::TimeTicks& release_time) {
  const auto state = backoff_rust_custom(
      {failure_count_, exponential_backoff_release_time_.ToInternalValue()},
      release_time.ToInternalValue());
  failure_count_ = state.failures;
  exponential_backoff_release_time_ =
      base::TimeTicks() + base::Microseconds(state.release);
}

bool BackoffEntry::CanDiscard() const {
  const BackoffRustContext context{*this, *policy_};
  return backoff_rust_discard(
      {failure_count_, exponential_backoff_release_time_.ToInternalValue()},
      RustPolicy(*policy_), &context);
}

void BackoffEntry::Reset() {
  const auto state = backoff_rust_reset();
  failure_count_ = state.failures;
  exponential_backoff_release_time_ =
      base::TimeTicks() + base::Microseconds(state.release);
}

base::TimeTicks BackoffEntry::GetTimeTicksNow() const {
  return clock_ ? clock_->NowTicks() : base::TimeTicks::Now();
}

base::TimeTicks BackoffEntry::BackoffDurationToReleaseTime(
    base::TimeDelta backoff_duration) const {
  const BackoffRustContext context{*this, *policy_};
  return base::TimeTicks() +
         base::Microseconds(backoff_rust_release_for_duration(
             backoff_duration.InMicroseconds(), RustPolicy(*policy_),
             &context));
}

}  // namespace net
