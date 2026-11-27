#include "net/socket/connection_jobs_upper_rust_original_helpers.h"
// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/socket/socket_pool_additional_capacity_rust_original.h"

#include <limits>

#include "base/notimplemented.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "net/base/features.h"
#include "net/socket/client_socket_pool_rust_original.h"
#include "net/socket/connect_job_factory_rust_original.h"
#include "net/socket/socket_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/fuzztest/src/fuzztest/fuzztest.h"

namespace net {

namespace {

static_assert(static_cast<int>(SocketPoolExpandability::kUncapped) ==
              static_cast<int>(SocketPoolExpandabilityRustOriginal::kUncapped));
static_assert(static_cast<int>(SocketPoolExpandability::kCapped) ==
              static_cast<int>(SocketPoolExpandabilityRustOriginal::kCapped));

TEST(SocketPoolAdditionalCapacityTestRustOriginal, CreateWithDisabledFeature) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      features::kTcpSocketPoolLimitRandomization);
  EXPECT_EQ(SocketPoolAdditionalCapacityRustOriginal::Create(0),
            SocketPoolAdditionalCapacityRustOriginal::CreateEmpty());
}

TEST(SocketPoolAdditionalCapacityTestRustOriginal, CreateWithEnabledFeature) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      features::kTcpSocketPoolLimitRandomization,
      {
          {
              "TcpSocketPoolLimitRandomizationBase",
              "0.1",
          },
          {
              "TcpSocketPoolLimitRandomizationMinimum",
              "0.3",
          },
          {
              "TcpSocketPoolLimitRandomizationNoise",
              "0.4",
          },
      });
  EXPECT_EQ(SocketPoolAdditionalCapacityRustOriginal::Create(2),
            SocketPoolAdditionalCapacityRustOriginal::CreateForTest(0.1, 2, 0.3, 0.4));
}

TEST(SocketPoolAdditionalCapacityTestRustOriginal, CreateForTest) {
  EXPECT_EQ(std::string(
                SocketPoolAdditionalCapacityRustOriginal::CreateForTest(0.1, 2, 0.3, 0.4)),
            "SocketPoolAdditionalCapacity(base:1.000000e-01,capacity:2,minimum:"
            "3.000000e-01,noise:4.000000e-01)");
}

TEST(SocketPoolAdditionalCapacityTestRustOriginal, CreateDefault) {
  EXPECT_EQ(std::string(SocketPoolAdditionalCapacityRustOriginal::Create(256)),
            "SocketPoolAdditionalCapacity(base:1.000000e-06,capacity:256,"
            "minimum:1.000000e-02,noise:2.000000e-01)");
}

TEST(SocketPoolAdditionalCapacityTestRustOriginal, InvalidCreation) {
  const SocketPoolAdditionalCapacityRustOriginal empty_pool =
      SocketPoolAdditionalCapacityRustOriginal::CreateEmpty();

  // base range
  EXPECT_EQ(SocketPoolAdditionalCapacityRustOriginal::CreateForTest(-0.1, 2, 0.3, 0.4),
            empty_pool);
  EXPECT_EQ(SocketPoolAdditionalCapacityRustOriginal::CreateForTest(1.1, 2, 0.3, 0.4),
            empty_pool);
  EXPECT_EQ(SocketPoolAdditionalCapacityRustOriginal::CreateForTest(
                std::numeric_limits<double>::quiet_NaN(), 2, 0.3, 0.4),
            empty_pool);

  // capacity range
  EXPECT_EQ(SocketPoolAdditionalCapacityRustOriginal::CreateForTest(0.1, 2000, 0.3, 0.4),
            empty_pool);

  // minimum range
  EXPECT_EQ(SocketPoolAdditionalCapacityRustOriginal::CreateForTest(0.1, 2, -0.3, 0.4),
            empty_pool);
  EXPECT_EQ(SocketPoolAdditionalCapacityRustOriginal::CreateForTest(0.1, 2, 1.3, 0.4),
            empty_pool);
  EXPECT_EQ(SocketPoolAdditionalCapacityRustOriginal::CreateForTest(
                0.1, 2, std::numeric_limits<double>::quiet_NaN(), 0.4),
            empty_pool);

  // noise range
  EXPECT_EQ(SocketPoolAdditionalCapacityRustOriginal::CreateForTest(0.1, 2, 0.3, -0.4),
            empty_pool);
  EXPECT_EQ(SocketPoolAdditionalCapacityRustOriginal::CreateForTest(0.1, 2, 0.3, 1.4),
            empty_pool);
  EXPECT_EQ(SocketPoolAdditionalCapacityRustOriginal::CreateForTest(
                0.1, 2, 0.3, std::numeric_limits<double>::quiet_NaN()),
            empty_pool);
}

TEST(SocketPoolAdditionalCapacityTestRustOriginal, NextExpandabilityBeforeAllocation) {
  // We use a base and noise of 0.0 with a minimum of 0.5 to ensure every roll
  // is a 50/50 shot so that we don't need to run the test millions of times
  // for flakes to be noticeable. The capacity of 2 is needed to test the logic.
  SocketPoolAdditionalCapacityRustOriginal pool_capacity =
      SocketPoolAdditionalCapacityRustOriginal::CreateForTest(0.0, 2, 0.5, 0.0);

  // Test out of bounds cases
  EXPECT_EQ(SocketPoolExpandabilityRustOriginal::kCapped,
            pool_capacity.NextExpandabilityBeforeAllocation(
                SocketPoolExpandabilityRustOriginal::kUncapped, 5, 2));
  EXPECT_EQ(SocketPoolExpandabilityRustOriginal::kCapped,
            pool_capacity.NextExpandabilityBeforeAllocation(
                SocketPoolExpandabilityRustOriginal::kCapped, 5, 2));

  // Below soft cap we are always uncapped
  EXPECT_EQ(SocketPoolExpandabilityRustOriginal::kUncapped,
            pool_capacity.NextExpandabilityBeforeAllocation(
                SocketPoolExpandabilityRustOriginal::kUncapped, 0, 2));
  EXPECT_EQ(SocketPoolExpandabilityRustOriginal::kUncapped,
            pool_capacity.NextExpandabilityBeforeAllocation(
                SocketPoolExpandabilityRustOriginal::kCapped, 0, 2));
  EXPECT_EQ(SocketPoolExpandabilityRustOriginal::kUncapped,
            pool_capacity.NextExpandabilityBeforeAllocation(
                SocketPoolExpandabilityRustOriginal::kUncapped, 1, 2));
  EXPECT_EQ(SocketPoolExpandabilityRustOriginal::kUncapped,
            pool_capacity.NextExpandabilityBeforeAllocation(
                SocketPoolExpandabilityRustOriginal::kCapped, 1, 2));

  // At hard cap we are always capped
  EXPECT_EQ(SocketPoolExpandabilityRustOriginal::kCapped,
            pool_capacity.NextExpandabilityBeforeAllocation(
                SocketPoolExpandabilityRustOriginal::kCapped, 4, 2));
  EXPECT_EQ(SocketPoolExpandabilityRustOriginal::kCapped,
            pool_capacity.NextExpandabilityBeforeAllocation(
                SocketPoolExpandabilityRustOriginal::kUncapped, 4, 2));

  // If capped at or above soft cap we always stay that way
  EXPECT_EQ(SocketPoolExpandabilityRustOriginal::kCapped,
            pool_capacity.NextExpandabilityBeforeAllocation(
                SocketPoolExpandabilityRustOriginal::kCapped, 2, 2));
  EXPECT_EQ(SocketPoolExpandabilityRustOriginal::kCapped,
            pool_capacity.NextExpandabilityBeforeAllocation(
                SocketPoolExpandabilityRustOriginal::kCapped, 3, 2));

  // When uncapped between soft and hard cap, we should be able to see some
  // distribution of each option. The probability inputs here should make it
  // a coin toss, but to prevent this from being flakey we run it 1000 times.
  bool did_see_uncapped = false;
  bool did_see_capped = false;
  for (size_t i = 0; i < 1000; ++i) {
    if (SocketPoolExpandabilityRustOriginal::kUncapped ==
        pool_capacity.NextExpandabilityBeforeAllocation(
            SocketPoolExpandabilityRustOriginal::kUncapped, 3, 2)) {
      did_see_uncapped = true;
    } else {
      did_see_capped = true;
    }
  }
  EXPECT_TRUE(did_see_uncapped);
  EXPECT_TRUE(did_see_capped);
}

TEST(SocketPoolAdditionalCapacityTestRustOriginal, NextExpandabilityAfterRelease) {
  // We use a base and noise of 0.0 with a minimum of 0.5 to ensure every roll
  // is a 50/50 shot so that we don't need to run the test millions of times
  // for flakes to be noticeable. The capacity of 2 is needed to test the logic.
  SocketPoolAdditionalCapacityRustOriginal pool_capacity =
      SocketPoolAdditionalCapacityRustOriginal::CreateForTest(0.0, 2, 0.5, 0.0);

  // Test out of bounds cases
  EXPECT_EQ(SocketPoolExpandabilityRustOriginal::kCapped,
            pool_capacity.NextExpandabilityAfterRelease(
                SocketPoolExpandabilityRustOriginal::kUncapped, 5, 2));
  EXPECT_EQ(SocketPoolExpandabilityRustOriginal::kCapped,
            pool_capacity.NextExpandabilityAfterRelease(
                SocketPoolExpandabilityRustOriginal::kCapped, 5, 2));

  // Below soft cap we are always uncapped
  EXPECT_EQ(SocketPoolExpandabilityRustOriginal::kUncapped,
            pool_capacity.NextExpandabilityAfterRelease(
                SocketPoolExpandabilityRustOriginal::kUncapped, 0, 2));
  EXPECT_EQ(SocketPoolExpandabilityRustOriginal::kUncapped,
            pool_capacity.NextExpandabilityAfterRelease(
                SocketPoolExpandabilityRustOriginal::kCapped, 0, 2));
  EXPECT_EQ(SocketPoolExpandabilityRustOriginal::kUncapped,
            pool_capacity.NextExpandabilityAfterRelease(
                SocketPoolExpandabilityRustOriginal::kUncapped, 1, 2));
  EXPECT_EQ(SocketPoolExpandabilityRustOriginal::kUncapped,
            pool_capacity.NextExpandabilityAfterRelease(
                SocketPoolExpandabilityRustOriginal::kCapped, 1, 2));

  // At hard cap we are always capped
  EXPECT_EQ(SocketPoolExpandabilityRustOriginal::kCapped,
            pool_capacity.NextExpandabilityAfterRelease(
                SocketPoolExpandabilityRustOriginal::kCapped, 4, 2));
  EXPECT_EQ(SocketPoolExpandabilityRustOriginal::kCapped,
            pool_capacity.NextExpandabilityAfterRelease(
                SocketPoolExpandabilityRustOriginal::kUncapped, 4, 2));

  // If uncapped at or above soft cap we always stay that way
  EXPECT_EQ(SocketPoolExpandabilityRustOriginal::kUncapped,
            pool_capacity.NextExpandabilityAfterRelease(
                SocketPoolExpandabilityRustOriginal::kUncapped, 2, 2));
  EXPECT_EQ(SocketPoolExpandabilityRustOriginal::kUncapped,
            pool_capacity.NextExpandabilityAfterRelease(
                SocketPoolExpandabilityRustOriginal::kUncapped, 3, 2));

  // When capped between soft and hard cap, we should be able to see some
  // distribution of each option. The probability inputs here should make it
  // a coin toss, but to prevent this from being flakey we run it 1000 times.
  bool did_see_uncapped = false;
  bool did_see_capped = false;
  for (size_t i = 0; i < 1000; ++i) {
    if (SocketPoolExpandabilityRustOriginal::kUncapped ==
        pool_capacity.NextExpandabilityAfterRelease(
            SocketPoolExpandabilityRustOriginal::kCapped, 3, 2)) {
      did_see_uncapped = true;
    } else {
      did_see_capped = true;
    }
  }
  EXPECT_TRUE(did_see_uncapped);
  EXPECT_TRUE(did_see_capped);
}

TEST(SocketPoolAdditionalCapacityTestRustOriginal, EmptyPool) {
  const SocketPoolAdditionalCapacityRustOriginal empty_pool =
      SocketPoolAdditionalCapacityRustOriginal::CreateEmpty();

  // No sockets in use
  EXPECT_EQ(SocketPoolExpandabilityRustOriginal::kUncapped,
            empty_pool.NextExpandabilityBeforeAllocation(
                SocketPoolExpandabilityRustOriginal::kUncapped, 0, 256));
  EXPECT_EQ(SocketPoolExpandabilityRustOriginal::kUncapped,
            empty_pool.NextExpandabilityAfterRelease(
                SocketPoolExpandabilityRustOriginal::kUncapped, 0, 256));
  EXPECT_EQ(SocketPoolExpandabilityRustOriginal::kUncapped,
            empty_pool.NextExpandabilityBeforeAllocation(
                SocketPoolExpandabilityRustOriginal::kCapped, 0, 256));
  EXPECT_EQ(SocketPoolExpandabilityRustOriginal::kUncapped,
            empty_pool.NextExpandabilityAfterRelease(
                SocketPoolExpandabilityRustOriginal::kCapped, 0, 256));

  // 50% of soft cap in use
  EXPECT_EQ(SocketPoolExpandabilityRustOriginal::kUncapped,
            empty_pool.NextExpandabilityBeforeAllocation(
                SocketPoolExpandabilityRustOriginal::kUncapped, 128, 256));
  EXPECT_EQ(SocketPoolExpandabilityRustOriginal::kUncapped,
            empty_pool.NextExpandabilityAfterRelease(
                SocketPoolExpandabilityRustOriginal::kUncapped, 128, 256));
  EXPECT_EQ(SocketPoolExpandabilityRustOriginal::kUncapped,
            empty_pool.NextExpandabilityBeforeAllocation(
                SocketPoolExpandabilityRustOriginal::kCapped, 128, 256));
  EXPECT_EQ(SocketPoolExpandabilityRustOriginal::kUncapped,
            empty_pool.NextExpandabilityAfterRelease(
                SocketPoolExpandabilityRustOriginal::kCapped, 128, 256));

  // 100% of soft cap in use
  EXPECT_EQ(SocketPoolExpandabilityRustOriginal::kCapped,
            empty_pool.NextExpandabilityBeforeAllocation(
                SocketPoolExpandabilityRustOriginal::kUncapped, 256, 256));
  EXPECT_EQ(SocketPoolExpandabilityRustOriginal::kCapped,
            empty_pool.NextExpandabilityAfterRelease(
                SocketPoolExpandabilityRustOriginal::kUncapped, 256, 256));
  EXPECT_EQ(SocketPoolExpandabilityRustOriginal::kCapped,
            empty_pool.NextExpandabilityBeforeAllocation(
                SocketPoolExpandabilityRustOriginal::kCapped, 256, 256));
  EXPECT_EQ(SocketPoolExpandabilityRustOriginal::kCapped,
            empty_pool.NextExpandabilityAfterRelease(
                SocketPoolExpandabilityRustOriginal::kCapped, 256, 256));
}

TEST(SocketPoolAdditionalCapacityTestRustOriginal,
     TestDefaultDistributionForFieldTrialConfig) {
  SocketPoolAdditionalCapacityRustOriginal pool = SocketPoolAdditionalCapacityRustOriginal::Create(256);

  // In order to do that we need an easy way to measure distributions.
  // Since we are applying noise, we run a ten thousand variants.
  auto percentage_transition_for_allocation_and_release =
      [&](size_t sockets_in_use) -> std::tuple<double, double> {
    size_t transition_allocation_count = 0;
    size_t transition_release_count = 0;
    for (size_t i = 0; i < 10000; ++i) {
      if (SocketPoolExpandabilityRustOriginal::kCapped ==
          pool.NextExpandabilityBeforeAllocation(
              SocketPoolExpandabilityRustOriginal::kUncapped, sockets_in_use, 256)) {
        ++transition_allocation_count;
      }
      if (SocketPoolExpandabilityRustOriginal::kUncapped ==
          pool.NextExpandabilityAfterRelease(SocketPoolExpandabilityRustOriginal::kCapped,
                                             sockets_in_use, 256)) {
        ++transition_release_count;
      }
    }
    return {transition_allocation_count / 10000.0,
            transition_release_count / 10000.0};
  };

  // We want to validate the distribution at three points: 5%, 50%, and 95%.
  auto fifth_percentile = percentage_transition_for_allocation_and_release(268);
  auto fiftieth_percentile =
      percentage_transition_for_allocation_and_release(384);
  auto ninetyfifth_percentile =
      percentage_transition_for_allocation_and_release(500);

  // When allocating sockets and uncapped:
  // We expect a ~1% transition rate if 5% of additional sockets are in use.
  EXPECT_GT(std::get<0>(fifth_percentile), 0.00);
  EXPECT_LT(std::get<0>(fifth_percentile), 0.025);
  // We expect a ~1% transition rate if 50% of additional sockets are in use.
  EXPECT_GT(std::get<0>(fiftieth_percentile), 0.00);
  EXPECT_LT(std::get<0>(fiftieth_percentile), 0.025);
  // We expect a ~50% transition rate if 95% of additional sockets are in use.
  EXPECT_GT(std::get<0>(ninetyfifth_percentile), 0.35);
  EXPECT_LT(std::get<0>(ninetyfifth_percentile), 0.65);

  // When releasing sockets and capped:
  // We expect a ~50% transition rate if 5% of additional sockets are in use.
  EXPECT_GT(std::get<1>(fifth_percentile), 0.35);
  EXPECT_LT(std::get<1>(fifth_percentile), 0.65);
  // We expect a ~1% transition rate if 50% of additional sockets are in use.
  EXPECT_GT(std::get<1>(fiftieth_percentile), 0.00);
  EXPECT_LT(std::get<1>(fiftieth_percentile), 0.025);
  // We expect a ~1% transition rate if 95% of additional sockets are in use.
  EXPECT_GT(std::get<1>(ninetyfifth_percentile), 0.00);
  EXPECT_LT(std::get<1>(ninetyfifth_percentile), 0.025);
}

void ValidateRandomizedInputs(double base,
                              size_t capacity,
                              double minimum,
                              double noise,
                              bool capped,
                              size_t sockets_in_use,
                              size_t socket_soft_cap) {
  SocketPoolAdditionalCapacityRustOriginal pool =
      SocketPoolAdditionalCapacityRustOriginal::CreateForTest(base, capacity, minimum,
                                                  noise);
  // Because there's some randomization here, we want to run these a few times.
  for (size_t i = 0; i < 1000; ++i) {
    pool.NextExpandabilityBeforeAllocation(
        capped ? SocketPoolExpandabilityRustOriginal::kCapped
               : SocketPoolExpandabilityRustOriginal::kUncapped,
        sockets_in_use, socket_soft_cap);
    pool.NextExpandabilityAfterRelease(capped
                                           ? SocketPoolExpandabilityRustOriginal::kCapped
                                           : SocketPoolExpandabilityRustOriginal::kUncapped,
                                       sockets_in_use, socket_soft_cap);
  }
}
FUZZ_TEST(SocketPoolAdditionalCapacityTestRustOriginal, ValidateRandomizedInputs)
    .WithDomains(fuzztest::Arbitrary<double>(),
                 fuzztest::Arbitrary<size_t>(),
                 fuzztest::Arbitrary<double>(),
                 fuzztest::Arbitrary<double>(),
                 fuzztest::Arbitrary<bool>(),
                 fuzztest::Arbitrary<size_t>(),
                 fuzztest::Arbitrary<size_t>())
    .WithSeeds({
        {std::numeric_limits<double>::quiet_NaN(), 0,
         std::numeric_limits<double>::quiet_NaN(),
         std::numeric_limits<double>::quiet_NaN(), false, 0, 0},
        {0.0, 0, 0.0, 0.0, false, 0, 0},
        {0.3, 64, 0.1, 0.1, false, 96, 64},
        {0.6, 128, 0.2, 0.2, true, 192, 128},
        {1.0, 256, 1.0, 1.0, true, 320, 256},
        {1.0, 256, 1.0, 1.0, true, std::numeric_limits<uint32_t>::max(),
         std::numeric_limits<uint32_t>::max()},
    });

// This is mocked up so that we can model the sort of function usage we expect
// in the additions to ClientSocketPool. We won't actually be implementing or
// using the normal public interface functions of a ClientSocketPool.
class MockClientSocketPoolRustOriginal : public ClientSocketPoolRustOriginal {
 public:
  MockClientSocketPoolRustOriginal()
      : ClientSocketPoolRustOriginal(/*socket_soft_cap=*/256,
                         ProxyChain::Direct(),
                         /*is_for_websockets=*/false,
                         /*common_connect_job_params*/ nullptr,
                         /*connect_job_factory*/ nullptr) {}

  SocketPoolExpandabilityRustOriginal RequestSocket() {
    UpdateExpandabilityBeforeAllocation();
    if (Expandability() == SocketPoolExpandabilityRustOriginal::kUncapped) {
      ++sockets_in_use_;
    }
    CHECK_LE(sockets_in_use_, 512);
    return Expandability();
  }

  SocketPoolExpandabilityRustOriginal ReleaseSocket() {
    --sockets_in_use_;
    UpdateExpandabilityAfterRelease();
    CHECK_GE(sockets_in_use_, 0);
    return Expandability();
  }

  size_t SocketsInUse() const override { return sockets_in_use_; }

 private:
  int RequestSocket(
      const GroupId& group_id,
      scoped_refptr<SocketParams> params,
      const std::optional<NetworkTrafficAnnotationTag>& proxy_annotation_tag,
      RequestPriority priority,
      const SocketTag& socket_tag,
      RespectLimits respect_limits,
      ClientSocketHandleRustOriginal* handle,
      CompletionOnceCallback callback,
      const ProxyAuthCallback& proxy_auth_callback,
      const NetLogWithSource& net_log) override {
    NOTIMPLEMENTED();
    return ERR_IO_PENDING;
  }
  int RequestSockets(
      const GroupId& group_id,
      scoped_refptr<SocketParams> params,
      const std::optional<NetworkTrafficAnnotationTag>& proxy_annotation_tag,
      size_t num_sockets,
      PreconnectCompletionCallback callback,
      const NetLogWithSource& net_log) override {
    NOTIMPLEMENTED();
    return ERR_IO_PENDING;
  }
  void SetPriority(const GroupId& group_id,
                   ClientSocketHandleRustOriginal* handle,
                   RequestPriority priority) override {
    NOTIMPLEMENTED();
  }
  void CancelRequest(const GroupId& group_id,
                     ClientSocketHandleRustOriginal* handle,
                     bool cancel_connect_job) override {
    NOTIMPLEMENTED();
  }
  void ReleaseSocket(const GroupId& group_id,
                     std::unique_ptr<StreamSocket> socket,
                     int64_t generation) override {
    NOTIMPLEMENTED();
  }
  void FlushWithError(int error, const char* net_log_reason_utf8) override {
    NOTIMPLEMENTED();
  }
  void CloseIdleSockets(const char* net_log_reason_utf8) override {
    NOTIMPLEMENTED();
  }
  void CloseIdleSocketsInGroup(const GroupId& group_id,
                               const char* net_log_reason_utf8) override {
    NOTIMPLEMENTED();
  }
  size_t IdleSocketCount() const override {
    NOTIMPLEMENTED();
    return 0;
  }
  size_t IdleSocketCountInGroup(const GroupId& group_id) const override {
    NOTIMPLEMENTED();
    return 0;
  }
  LoadState GetLoadState(const GroupId& group_id,
                         const ClientSocketHandleRustOriginal* handle) const override {
    NOTIMPLEMENTED();
    return LOAD_STATE_IDLE;
  }
  base::Value GetInfoAsValue(const std::string& name,
                             const std::string& type) const override {
    NOTIMPLEMENTED();
    return base::Value();
  }
  bool HasActiveSocket(const GroupId& group_id) const override {
    NOTIMPLEMENTED();
    return false;
  }
  bool IsStalled() const override {
    NOTIMPLEMENTED();
    return false;
  }
  void AddHigherLayeredPool(HigherLayeredPoolRustOriginal* higher_pool) override {
    NOTIMPLEMENTED();
  }
  void RemoveHigherLayeredPool(HigherLayeredPoolRustOriginal* higher_pool) override {
    NOTIMPLEMENTED();
  }

 private:
  size_t sockets_in_use_{0};
};

TEST(SocketPoolAdditionalCapacityTestRustOriginal,
     ValidateAdditionalCapacityForMockClientSocketPool) {
  MockClientSocketPoolRustOriginal pool;
  ValidateAdditionalCapacityForSocketPool(
      base::BindLambdaForTesting([&]() { return static_cast<SocketPoolExpandability>(pool.RequestSocket()); }),
      base::DoNothing(),
      base::BindLambdaForTesting([&]() { return static_cast<SocketPoolExpandability>(pool.ReleaseSocket()); }),
      base::BindLambdaForTesting([&]() { return pool.SocketsInUse(); }));
}

}  // namespace

}  // namespace net
