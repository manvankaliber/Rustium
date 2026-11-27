// Differential test of the public API against the frozen C++ implementation.
#include <cstdint>
#include <limits>

#include "base/time/tick_clock.h"
#include "base/values.h"
#include "net/base/backoff_entry.h"
#include "net/base/backoff_entry_serializer.h"
#include "net/base/backoff_rust_original_entry.h"
#include "net/base/backoff_rust_original_serializer.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
uint64_t random_state = 0;
uint64_t random_draws = 0;
uint64_t Next(uint64_t& x) {
  x ^= x << 13;
  x ^= x >> 7;
  x ^= x << 17;
  return x;
}
class Clock final : public base::TickClock {
 public:
  base::TimeTicks NowTicks() const override {
    ++calls;
    return base::TimeTicks() + base::Microseconds(now);
  }
  int64_t now = 0;
  mutable int calls = 0;
};
}  // namespace
double backoff_rust_test_random() {
  ++random_draws;
  return static_cast<double>(Next(random_state) >> 11) *
         (1.0 / 9007199254740992.0);
}

TEST(BackoffRustDifferential, MillionStateAndSerializerInputs) {
  using Original = backoff_rust_original::BackoffEntry;
  net::BackoffEntry::Policy p{1, 100, 2.0, 0.25, 100000, 1000, false};
  Original::Policy q{1, 100, 2.0, 0.25, 100000, 1000, false};
  Clock a, b;
  net::BackoffEntry actual(&p, &a);
  Original reference(&q, &b);
  uint64_t gen = 0x65292f19abc1278dULL;
  for (int i = 0; i < 1000000; ++i) {
    const uint64_t x = Next(gen);
    a.now = b.now = static_cast<int64_t>(x % 1000000000);
    a.calls = b.calls = 0;
    const uint64_t seed = Next(gen) | 1;
    auto run = [&](bool rust) {
      random_state = seed;
      random_draws = 0;
      switch ((x >> 8) % 8) {
        case 0:
        case 1:
        case 2:
          if (rust) {
            actual.InformOfRequest(false);
          } else {
            reference.InformOfRequest(false);
          }
          break;
        case 3:
          if (rust) {
            actual.InformOfRequest(true);
          } else {
            reference.InformOfRequest(true);
          }
          break;
        case 4:
          if (rust) {
            actual.Reset();
          } else {
            reference.Reset();
          }
          break;
        case 5: {
          auto t =
              base::TimeTicks() +
              base::Microseconds(static_cast<int64_t>((x >> 16) % 1000000000));
          if (rust) {
            actual.SetCustomReleaseTime(t);
          } else {
            reference.SetCustomReleaseTime(t);
          }
          break;
        }
        default:
          break;
      }
      return random_draws;
    };
    auto da = run(true);
    auto db = run(false);
    ASSERT_EQ(da, db) << i;
    ASSERT_EQ(a.calls, b.calls) << i;
    ASSERT_EQ(actual.failure_count(), reference.failure_count()) << i;
    ASSERT_EQ(actual.GetReleaseTime(), reference.GetReleaseTime()) << i;
    ASSERT_EQ(actual.ShouldRejectRequest(), reference.ShouldRejectRequest())
        << i;
    ASSERT_EQ(actual.GetTimeUntilRelease(), reference.GetTimeUntilRelease())
        << i;
    ASSERT_EQ(actual.CanDiscard(), reference.CanDiscard()) << i;
    ASSERT_EQ(a.calls, b.calls) << i;
    auto wall = base::Time::FromInternalValue(
        static_cast<int64_t>(Next(gen) % 1000000000));
    auto actual_list =
        net::BackoffEntrySerializer::SerializeToList(actual, wall);
    auto ref_list =
        backoff_rust_original::BackoffEntrySerializer::SerializeToList(
            reference, wall);
    ASSERT_EQ(actual_list, ref_list) << i;
    // Mutate serialized fields, including malformed types and both versions.
    base::ListValue list;
    switch ((x >> 24) % 8) {
      case 0:
        list.Append(3);
        list.Append(1);
        list.Append("0");
        list.Append("0");
        break;
      case 1:
        list.Append(1);
        list.Append(static_cast<int>(x % 5));
        list.Append(0.125);
        list.Append("0");
        break;
      case 2:
        list.Append(2);
        list.Append(-1);
        list.Append("0");
        list.Append("0");
        break;
      case 3:
        list.Append(2);
        list.Append(1);
        list.Append("bogus");
        list.Append("0");
        break;
      case 4:
        list.Append(2);
        list.Append(1);
        list.Append("0");
        list.Append("9223372036854775807");
        break;
      case 5:
        list.Append(2);
        list.Append(2);
        list.Append("-1000");
        list.Append("0");
        break;
      case 6:
        list.Append(2);
        list.Append(1);
        list.Append("0");
        list.Append(4);
        break;
      default:
        list.Append(2);
        list.Append(static_cast<int>(x % 5));
        list.Append("10000");
        list.Append("250000000");
        break;
    }
    a.calls = b.calls = 0;
    random_state = seed;
    random_draws = 0;
    auto decoded =
        net::BackoffEntrySerializer::DeserializeFromList(list, &p, &a, wall);
    da = random_draws;
    random_state = seed;
    random_draws = 0;
    auto original =
        backoff_rust_original::BackoffEntrySerializer::DeserializeFromList(
            list, &q, &b, wall);
    db = random_draws;
    ASSERT_EQ(da, db) << i;
    ASSERT_EQ(a.calls, b.calls) << i;
    ASSERT_EQ(bool(decoded), bool(original)) << i;
    if (decoded) {
      ASSERT_EQ(decoded->failure_count(), original->failure_count()) << i;
      ASSERT_EQ(decoded->GetReleaseTime(), original->GetReleaseTime()) << i;
    }
    // Mutate the same policy storage while both entries remain alive.
    if ((i % 97) == 0) {
      p.num_errors_to_ignore = q.num_errors_to_ignore =
          static_cast<int>((x >> 32) % 4);
      p.initial_delay_ms = q.initial_delay_ms =
          static_cast<int64_t>((x >> 36) % 1000);
      p.always_use_initial_delay = q.always_use_initial_delay = (x & 1);
      const double factors[] = {0.5, 1.0, 2.0,
                                std::numeric_limits<double>::infinity(),
                                std::numeric_limits<double>::quiet_NaN()};
      p.multiply_factor = q.multiply_factor = factors[(x >> 40) % 5];
    }
  }
}

TEST(BackoffRustDifferential, ClockCanMutatePolicyDuringRead) {
  using Original = backoff_rust_original::BackoffEntry;
  net::BackoffEntry::Policy p{0, 1000, 2.0, 0.0, 10000, 1000, false};
  Original::Policy q{0, 1000, 2.0, 0.0, 10000, 1000, false};
  class MutatingClock final : public base::TickClock {
   public:
    MutatingClock(int64_t* maximum, int64_t* lifetime)
        : maximum_(maximum), lifetime_(lifetime) {}
    base::TimeTicks NowTicks() const override {
      *maximum_ = 5;
      *lifetime_ = 10;
      return base::TimeTicks() + base::Milliseconds(100);
    }

   private:
    raw_ptr<int64_t> maximum_;
    raw_ptr<int64_t> lifetime_;
  };
  MutatingClock a(&p.maximum_backoff_ms, &p.entry_lifetime_ms);
  MutatingClock b(&q.maximum_backoff_ms, &q.entry_lifetime_ms);
  net::BackoffEntry actual(&p, &a);
  Original reference(&q, &b);
  random_state = 42;
  actual.InformOfRequest(false);
  random_state = 42;
  reference.InformOfRequest(false);
  EXPECT_EQ(actual.GetReleaseTime(), reference.GetReleaseTime());
  EXPECT_EQ(actual.GetReleaseTime(),
            base::TimeTicks() + base::Milliseconds(105));
  p.maximum_backoff_ms = q.maximum_backoff_ms = 10000;
  p.entry_lifetime_ms = q.entry_lifetime_ms = 1000;
  EXPECT_EQ(actual.CanDiscard(), reference.CanDiscard());
}

TEST(BackoffRustDifferential, ClockObservesAndUpdatesState) {
  using Original = backoff_rust_original::BackoffEntry;
  net::BackoffEntry::Policy p{0, 100, 2.0, 0.0, 10000, 1000, false};
  Original::Policy q{0, 100, 2.0, 0.0, 10000, 1000, false};
  class InspectClock final : public base::TickClock {
   public:
    base::TimeTicks NowTicks() const override {
      if (actual_) {
        observed_ = actual_->failure_count();
        actual_->SetCustomReleaseTime(base::TimeTicks() + base::Seconds(100));
      }
      if (original_) {
        observed_ = original_->failure_count();
        original_->SetCustomReleaseTime(base::TimeTicks() + base::Seconds(100));
      }
      return base::TimeTicks() + base::Seconds(1);
    }
    raw_ptr<net::BackoffEntry> actual_ = nullptr;
    raw_ptr<Original> original_ = nullptr;
    mutable int observed_ = -1;
  };
  InspectClock a, b;
  net::BackoffEntry actual(&p, &a);
  Original original(&q, &b);
  a.actual_ = &actual;
  b.original_ = &original;
  random_state = 11;
  actual.InformOfRequest(false);
  random_state = 11;
  original.InformOfRequest(false);
  EXPECT_EQ(a.observed_, 1);
  EXPECT_EQ(a.observed_, b.observed_);
  EXPECT_EQ(actual.failure_count(), original.failure_count());
  EXPECT_EQ(actual.GetReleaseTime(), original.GetReleaseTime());
}

TEST(BackoffRustDifferential, ClockChangesReleaseDuringQueries) {
  using Original = backoff_rust_original::BackoffEntry;
  net::BackoffEntry::Policy p{0, 100, 2.0, 0.0, 10000, 1000, false};
  Original::Policy q{0, 100, 2.0, 0.0, 10000, 1000, false};
  class MutateClock final : public base::TickClock {
   public:
    base::TimeTicks NowTicks() const override {
      if (actual_) {
        actual_->SetCustomReleaseTime(base::TimeTicks() + base::Seconds(100));
      }
      if (original_) {
        original_->SetCustomReleaseTime(base::TimeTicks() + base::Seconds(100));
      }
      return base::TimeTicks() + base::Seconds(1);
    }
    raw_ptr<net::BackoffEntry> actual_ = nullptr;
    raw_ptr<Original> original_ = nullptr;
  };
  MutateClock a, b;
  net::BackoffEntry actual(&p, &a);
  Original original(&q, &b);
  a.actual_ = &actual;
  b.original_ = &original;
  EXPECT_EQ(actual.ShouldRejectRequest(), original.ShouldRejectRequest());
  actual.Reset();
  original.Reset();
  EXPECT_EQ(actual.GetTimeUntilRelease(), original.GetTimeUntilRelease());
  actual.Reset();
  original.Reset();
  EXPECT_EQ(actual.CanDiscard(), original.CanDiscard());
}

TEST(BackoffRustDifferential, DefinedNumericBoundaries) {
  using Original = backoff_rust_original::BackoffEntry;
  const double values[] = {0.0,
                           -0.5,
                           1.0,
                           2.0,
                           std::numeric_limits<double>::infinity(),
                           -std::numeric_limits<double>::infinity(),
                           std::numeric_limits<double>::quiet_NaN()};
  const int64_t delays[] = {-1000, -1,   0,
                            1,     1000, std::numeric_limits<int64_t>::max()};
  for (double factor : values) {
    for (double jitter : values) {
      for (int64_t delay : delays) {
        net::BackoffEntry::Policy p{0,     delay, factor, jitter,
                                    10000, 1000,  false};
        Original::Policy q{0, delay, factor, jitter, 10000, 1000, false};
        Clock a, b;
        a.now = b.now = 1234567;
        net::BackoffEntry actual(&p, &a);
        Original original(&q, &b);
        for (int i = 0; i < 4; ++i) {
          random_state = 123 + i;
          random_draws = 0;
          actual.InformOfRequest(false);
          auto da = random_draws;
          random_state = 123 + i;
          random_draws = 0;
          original.InformOfRequest(false);
          auto db = random_draws;
          ASSERT_EQ(da, db);
          ASSERT_EQ(actual.GetReleaseTime(), original.GetReleaseTime());
          ASSERT_EQ(actual.failure_count(), original.failure_count());
          ASSERT_EQ(a.calls, b.calls);
        }
      }
    }
  }
}
