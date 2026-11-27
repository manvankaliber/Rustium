// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license in LICENSE.
#include "net/dns/host_resolver_cache_rust_test_support.h"

#include <cstdint>
#include <type_traits>

#include "net/base/schemeful_site.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace net::host_resolver_cache_rust_test {
namespace {

template <class Api>
class HostResolverCachePolicyTest : public testing::Test {};
using Implementations = testing::Types<Production, Original>;
TYPED_TEST_SUITE(HostResolverCachePolicyTest, Implementations);

TYPED_TEST(HostResolverCachePolicyTest, ClockOrderIncludesMisses) {
  using Api = TypeParam;
  Clocks clocks;
  const typename Api::Cache cache(8, clocks.wall, clocks.ticks);
  EXPECT_EQ(Lookup(cache), nullptr);
  EXPECT_EQ(clocks.wall.events, (std::vector<char>{'t', 'w'}));
  clocks.wall.events.clear();
  EXPECT_FALSE(LookupStale(cache));
  EXPECT_EQ(clocks.wall.events, (std::vector<char>{'t', 'w'}));
}

TYPED_TEST(HostResolverCachePolicyTest, ExpirationExactlyNowIsActive) {
  using Api = TypeParam;
  Clocks clocks;
  typename Api::Cache cache(8, clocks.wall, clocks.ticks);
  Set<Api>(cache, Data<Api>(clocks, "host.test", "zero", base::TimeDelta()));
  ASSERT_NE(Lookup(cache), nullptr);
  auto at_zero = LookupStale(cache);
  ASSERT_TRUE(at_zero);
  EXPECT_FALSE(at_zero->IsStale());
  EXPECT_FALSE(at_zero->expired_by);
  clocks.ticks.now += base::Microseconds(1);
  EXPECT_EQ(Lookup(cache), nullptr);
  auto after = LookupStale(cache);
  ASSERT_TRUE(after);
  EXPECT_EQ(after->expired_by, base::Microseconds(1));
}

TYPED_TEST(HostResolverCachePolicyTest, TickExpiryWinsAndWallOnlyFallsBack) {
  using Api = TypeParam;
  Clocks clocks;
  typename Api::Cache cache(8, clocks.wall, clocks.ticks);
  Set<Api>(cache, Data<Api>(clocks, "host.test", "tick", base::Seconds(20),
                           base::Seconds(-20)));
  EXPECT_EQ(Id(Lookup(cache)), "tick");
  Set<Api>(cache, Data<Api>(clocks, "wall.test", "wall", base::Seconds(20),
                           base::Seconds(-20), DnsQueryType::A, false));
  EXPECT_EQ(Lookup(cache, "wall.test"), nullptr);
  auto stale = LookupStale(cache, "wall.test");
  ASSERT_TRUE(stale);
  EXPECT_EQ(stale->expired_by, base::Seconds(20));
}

TYPED_TEST(HostResolverCachePolicyTest, ClockCallbackChangesGenerationLive) {
  using Api = TypeParam;
  Clocks clocks;
  typename Api::Cache cache(8, clocks.wall, clocks.ticks);
  Set<Api>(cache, Data<Api>(clocks));
  clocks.ticks.hook = [&] { cache.MakeAllResultsStale(); };
  EXPECT_EQ(Lookup(cache), nullptr);
  auto stale = LookupStale(cache);
  ASSERT_TRUE(stale);
  EXPECT_TRUE(stale->stale_by_generation);
  EXPECT_FALSE(stale->expired_by);
}

TYPED_TEST(HostResolverCachePolicyTest, CandidatesCapturedBeforeClockInsert) {
  using Api = TypeParam;
  Clocks clocks;
  typename Api::Cache cache(8, clocks.wall, clocks.ticks);
  Set<Api>(cache, Data<Api>(clocks));
  clocks.ticks.hook = [&] {
    Set<Api>(cache, Data<Api>(clocks, "host.test", "inserted", base::Seconds(30),
                             base::Seconds(30), DnsQueryType::AAAA), true);
  };
  EXPECT_EQ(Id(Lookup(cache)), "first");
  EXPECT_EQ(Id(Lookup(cache)), "inserted");
}

TYPED_TEST(HostResolverCachePolicyTest, CloneAndMovesPreserveIndependentOwnership) {
  using Api = TypeParam;
  static_assert(!std::is_copy_constructible_v<typename Api::Cache>);
  static_assert(std::is_move_constructible_v<typename Api::Cache>);
  Clocks clocks;
  typename Api::Cache source(8, clocks.wall, clocks.ticks);
  auto input = Data<Api>(clocks);
  auto clone = input->Clone();
  input->AsData().set_strings({"mutated"});
  Set<Api>(source, std::move(input));
  typename Api::Cache moved(std::move(source));
  typename Api::Cache destination(1, clocks.wall, clocks.ticks);
  destination = std::move(moved);
  EXPECT_EQ(Id(Lookup(destination)), "mutated");
  EXPECT_EQ(Id(clone.get()), "first");
  typename Api::Cache independent(8, clocks.wall, clocks.ticks);
  EXPECT_TRUE(independent.RestoreFromValue(destination.Serialize()));
  Set<Api>(destination, Data<Api>(clocks, "host.test", "replacement"));
  EXPECT_EQ(Id(&LookupStale(independent)->result.get()), "mutated");
}

TYPED_TEST(HostResolverCachePolicyTest, KeyCanonicalizationPartitionsAndWildcards) {
  using Api = TypeParam;
  Clocks clocks;
  typename Api::Cache cache(8, clocks.wall, clocks.ticks);
  const auto key = NetworkAnonymizationKey::CreateSameSite(
      SchemefulSite(GURL("https://partition.test")));
  Set<Api>(cache, Data<Api>(clocks, "HoSt.TeSt", "partitioned"), false,
           HostResolverSource::DNS, 42, key);
  EXPECT_EQ(Lookup(cache, "host.test"), nullptr);
  EXPECT_EQ(Id(Lookup(cache, "HOST.TEST", DnsQueryType::A, false,
                      HostResolverSource::DNS, 42, key)), "partitioned");
  Set<Api>(cache, Data<Api>(clocks, "host.test", "wildcard", base::Seconds(30),
                           base::Seconds(30), DnsQueryType::UNSPECIFIED), false,
           HostResolverSource::SYSTEM);
  EXPECT_EQ(Id(Lookup(cache, "host.test", DnsQueryType::AAAA, false,
                      HostResolverSource::SYSTEM)), "wildcard");
}

TYPED_TEST(HostResolverCachePolicyTest, SecurityPreferenceAndExplicitFalse) {
  using Api = TypeParam;
  Clocks clocks;
  typename Api::Cache cache(8, clocks.wall, clocks.ticks);
  Set<Api>(cache, Data<Api>(clocks, "host.test", "secure"), true);
  Set<Api>(cache, Data<Api>(clocks, "host.test", "new-insecure"), false);
  EXPECT_EQ(Id(Lookup(cache)), "secure");
  EXPECT_EQ(Id(Lookup(cache, "host.test", DnsQueryType::A, false)), "new-insecure");
  EXPECT_EQ(Id(Lookup(cache, "host.test", DnsQueryType::A, true)), "secure");
}

TYPED_TEST(HostResolverCachePolicyTest, RestoreCommitsPrefixOnMalformedTail) {
  using Api = TypeParam;
  Clocks clocks;
  typename Api::Cache donor(8, clocks.wall, clocks.ticks);
  Set<Api>(donor, Data<Api>(clocks));
  base::Value serialized = donor.Serialize();
  serialized.GetList().Append(17);
  typename Api::Cache restored(8, clocks.wall, clocks.ticks);
  EXPECT_FALSE(restored.RestoreFromValue(serialized));
  EXPECT_EQ(restored.size_for_testing(), 1u);
  EXPECT_EQ(Lookup(restored), nullptr);
  auto result = LookupStale(restored);
  ASSERT_TRUE(result);
  EXPECT_TRUE(result->stale_by_generation);
  EXPECT_FALSE(result->expired_by);
}

TYPED_TEST(HostResolverCachePolicyTest, RestoreAtCapacityIgnoresMalformedTail) {
  using Api = TypeParam;
  Clocks clocks;
  typename Api::Cache donor(8, clocks.wall, clocks.ticks);
  Set<Api>(donor, Data<Api>(clocks));
  base::Value serialized = donor.Serialize();
  serialized.GetList().Append(17);
  typename Api::Cache restored(1, clocks.wall, clocks.ticks);
  EXPECT_TRUE(restored.RestoreFromValue(serialized));
  EXPECT_EQ(restored.size_for_testing(), 1u);
  base::ListValue malformed;
  malformed.Append(17);
  EXPECT_TRUE(restored.RestoreFromValue(base::Value(std::move(malformed))));
}

TYPED_TEST(HostResolverCachePolicyTest, UnsupportedKeySkippedBeforeOtherValidation) {
  using Api = TypeParam;
  Clocks clocks;
  typename Api::Cache donor(8, clocks.wall, clocks.ticks);
  Set<Api>(donor, Data<Api>(clocks));
  base::Value serialized = donor.Serialize();
  base::DictValue unknown;
  unknown.Set("network_anonymization_key", 17);
  base::ListValue input;
  input.Append(std::move(unknown));
  input.Append(serialized.GetList().front().Clone());
  typename Api::Cache restored(8, clocks.wall, clocks.ticks);
  EXPECT_TRUE(restored.RestoreFromValue(base::Value(std::move(input))));
  EXPECT_EQ(restored.size_for_testing(), 1u);
}

TYPED_TEST(HostResolverCachePolicyTest, HostValueHelperAndExistingEntryWin) {
  using Api = TypeParam;
  Clocks clocks;
  typename Api::Cache donor(8, clocks.wall, clocks.ticks);
  Set<Api>(donor, Data<Api>(clocks, "host.test", "donor", base::Seconds(30),
                           base::Seconds(30), DnsQueryType::A, true, true));
  typename Api::Cache restored(8, clocks.wall, clocks.ticks);
  EXPECT_TRUE(restored.RestoreFromValue(donor.Serialize()));
  auto result = LookupStale(restored);
  ASSERT_TRUE(result);
  EXPECT_EQ(result->result.get().AsData().hosts().front().host(), "srv-target.test");
  EXPECT_EQ(result->result.get().AsData().hosts().front().port(), 443u);
  Set<Api>(restored, Data<Api>(clocks, "host.test", "existing"));
  EXPECT_TRUE(restored.RestoreFromValue(donor.Serialize()));
  EXPECT_EQ(Id(Lookup(restored)), "existing");
}

TYPED_TEST(HostResolverCachePolicyTest, EvictionTieUsesMapOrderAndRemovesAllStale) {
  using Api = TypeParam;
  Clocks clocks;
  typename Api::Cache cache(2, clocks.wall, clocks.ticks);
  Set<Api>(cache, Data<Api>(clocks, "z.test", "z"));
  Set<Api>(cache, Data<Api>(clocks, "a.test", "a"));
  Set<Api>(cache, Data<Api>(clocks, "m.test", "m"));
  EXPECT_EQ(Lookup(cache, "a.test"), nullptr);
  EXPECT_NE(Lookup(cache, "z.test"), nullptr);
  cache.MakeAllResultsStale();
  Set<Api>(cache, Data<Api>(clocks, "fresh.test", "fresh"));
  EXPECT_EQ(cache.size_for_testing(), 1u);
  EXPECT_NE(Lookup(cache, "fresh.test"), nullptr);
}

template <class Api>
class HookResult final : public Api::Result {
 public:
  using Result = typename Api::Result;
  HookResult(const Clocks& clocks, std::string name, std::function<void()> serialize,
             std::function<void()> destroy = {})
      : Result(std::move(name), DnsQueryType::A, clocks.ticks.now + base::Seconds(30),
               clocks.wall.now + base::Seconds(30), Result::Type::kError,
               Result::Source::kDns),
        serialize_(std::move(serialize)), destroy_(std::move(destroy)) {}
  ~HookResult() override {
    if (destroy_) {
      destroy_();
    }
  }
  std::unique_ptr<Result> Clone() const override {
    return std::make_unique<typename Api::Error>(
        this->domain_name(), this->query_type(), this->expiration(),
        this->timed_expiration(), this->source(), ERR_NAME_NOT_RESOLVED);
  }
  base::Value ToValue() const override {
    std::function<void()> hook = std::exchange(serialize_, {});
    if (hook) {
      hook();
    }
    return Clone()->ToValue();
  }
 private:
  mutable std::function<void()> serialize_;
  std::function<void()> destroy_;
};

TYPED_TEST(HostResolverCachePolicyTest, SerializationVisitsDefinedLaterInsertion) {
  using Api = TypeParam;
  Clocks clocks;
  typename Api::Cache cache(8, clocks.wall, clocks.ticks);
  Set<Api>(cache, std::make_unique<HookResult<Api>>(
      clocks, "a.test", [&] { Set<Api>(cache, Data<Api>(clocks, "z.test", "later")); }));
  base::Value serialized = cache.Serialize();
  EXPECT_EQ(serialized.GetList().size(), 2u);
  EXPECT_EQ(cache.size_for_testing(), 2u);
}

TYPED_TEST(HostResolverCachePolicyTest, ReplacementDestructorCanInsertOtherKey) {
  using Api = TypeParam;
  Clocks clocks;
  typename Api::Cache cache(8, clocks.wall, clocks.ticks);
  Set<Api>(cache, std::make_unique<HookResult<Api>>(
      clocks, "host.test", std::function<void()>(),
      [&] { Set<Api>(cache, Data<Api>(clocks, "other.test", "during-destruction")); }));
  Set<Api>(cache, Data<Api>(clocks, "host.test", "replacement"));
  EXPECT_EQ(cache.size_for_testing(), 2u);
  EXPECT_EQ(Id(Lookup(cache, "other.test")), "during-destruction");
  EXPECT_EQ(Id(Lookup(cache)), "replacement");
}

TEST(HostResolverCachePairedTest, OrdinaryBoundedStateSequence) {
  Clocks clocks;
  Production::Cache production(16, clocks.wall, clocks.ticks);
  Original::Cache original(16, clocks.wall, clocks.ticks);
  uint64_t state = 0x9e3779b97f4a7c15ULL;
  for (unsigned step = 0; step < 512; ++step) {
    state ^= state << 13;
    state ^= state >> 7;
    state ^= state << 17;
    const std::string name = "host" + std::to_string(state % 12) + ".test";
    const auto type = (state & 16) ? DnsQueryType::A : DnsQueryType::AAAA;
    if ((state % 11) == 0) {
      production.MakeAllResultsStale();
      original.MakeAllResultsStale();
    } else if ((state % 7) == 0) {
      clocks.ticks.now += base::Seconds(1);
      clocks.wall.now -= base::Seconds(1);
    } else {
      const auto ttl = base::Seconds(static_cast<int>(state % 9) - 2);
      const auto id = std::to_string(step);
      Set<Production>(production, Data<Production>(clocks, name, id, ttl, ttl, type), state & 1);
      Set<Original>(original, Data<Original>(clocks, name, id, ttl, ttl, type), state & 1);
    }
    SCOPED_TRACE(step);
    EXPECT_EQ(Observe(production, name), Observe(original, name));
  }
}

}  // namespace
}  // namespace net::host_resolver_cache_rust_test
