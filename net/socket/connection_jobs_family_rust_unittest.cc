// Source-only ordinary family integration tests; not compiled or executed.
#include "net/socket/connection_jobs_family_rust_test_support.h"

#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "base/test/bind.h"
#include "net/dns/mock_host_resolver.h"
#include "testing/gtest/include/gtest/gtest.h"
namespace net::connection_jobs_family_test {
static_assert(sizeof(Production::Base)==sizeof(Original::Base));
static_assert(alignof(Production::Base)==alignof(Original::Base));
static_assert(sizeof(Production::Transport)==sizeof(Original::Transport));
static_assert(alignof(Production::Transport)==alignof(Original::Transport));
static_assert(sizeof(Production::Tcp)==sizeof(Original::Tcp));
static_assert(alignof(Production::Tcp)==alignof(Original::Tcp));

TEST(ConnectionJobsFamilyRustTest, WholeLifecycleMatchesIndependentOriginal) {
 base::test::TaskEnvironment tasks;
 for(size_t kind=0;kind<std::size(kCases);++kind) {
  SCOPED_TRACE(kCases[kind]);
  base::test::ScopedFeatureList features;
  if(kind>=6&&kind<12) features.InitAndEnableFeature(features::kHappyEyeballsV2);
  else features.InitAndDisableFeature(features::kHappyEyeballsV2);
  Fixture<Original> original;Fixture<Production> rust;
  EXPECT_EQ(::net::connection_jobs_family_test::Run(original,kind,3),::net::connection_jobs_family_test::Run(rust,kind,3));
 }
}

namespace {

enum class ResolverCase {
  kSuccess,
  kTimeout,
  kCancelled,
  kCancelledAfterQueue,
};

struct ResolverTrace {
  int connect_result = ERR_UNEXPECTED;
  int completion_result = ERR_IO_PENDING;
  int host_callbacks = 0;
  int socket_allocations = 0;
  bool pending_after_connect = false;
  bool pending_after_finish = false;
  bool callback_before_completion = false;
  bool socket_present = false;
  IPEndPoint peer;
  ResolveErrorInfo error_info;
  ConnectionAttempts attempts;
  std::vector<HostResolverEndpointResult> callback_results;
  base::flat_set<std::string> callback_aliases;
  base::flat_set<std::string> socket_aliases;

  bool operator==(const ResolverTrace&) const = default;
};

HostResolverEndpointResult ResolverEndpoint() {
  HostResolverEndpointResult endpoint;
  endpoint.ip_endpoints = {Ip("2001:db8::1", 8443), Ip("192.0.2.1", 8443)};
  endpoint.metadata.supported_protocol_alpns = {"h2"};
  return endpoint;
}

template <class Traits, class CallbackResult>
ResolverTrace RunResolverCase(ResolverCase test_case,
                              base::test::TaskEnvironment& tasks) {
  // Both jobs consume the same scripted provider behavior. The mock
  // shares DNS name validation and transport primitives with production.
  MockHostResolver resolver;
  resolver.set_ondemand_mode(true);
  if (test_case == ResolverCase::kTimeout) {
    resolver.rules()->AddSimulatedTimeoutFailure("family.example");
  } else {
    resolver.rules()->AddRule(
        "family.example",
        MockHostResolverBase::RuleResolver::RuleResult(
            {ResolverEndpoint()}, {"alias.example", "family.example"}));
  }

  const MockTransportClientSocketFactory::Rule rule(
      MockTransportClientSocketFactory::Type::kSynchronous,
      std::vector{Ip("2001:db8::1", 8443)});
  Fixture<Traits> fixture;
  fixture.common.host_resolver = &resolver;
  fixture.socket_factory.SetRules(base::span_from_ref(rule));

  ResolverTrace trace;
  Delegate<Traits> delegate;
  auto params = base::MakeRefCounted<typename Traits::Params>(
      url::SchemeHostPort("https", "family.example", 443),
      NetworkAnonymizationKey(), SecureDnsPolicy::kAllow,
      handles::kInvalidNetworkHandle,
      base::BindLambdaForTesting(
          [&](const HostPortPair& host,
              const HostResolverEndpointsOrServiceEndpoints& results,
              const base::flat_set<std::string>& aliases) {
            EXPECT_EQ(host.host(), "family.example");
            EXPECT_EQ(host.port(), 443);
            ++trace.host_callbacks;
            trace.callback_before_completion = delegate.result == ERR_IO_PENDING;
            const auto* endpoints =
                std::get_if<base::span<const HostResolverEndpointResult>>(
                    &results);
            EXPECT_NE(endpoints, nullptr);
            if (endpoints) {
              trace.callback_results.assign(endpoints->begin(), endpoints->end());
            }
            trace.callback_aliases = aliases;
            return CallbackResult::kContinue;
          }),
      base::flat_set<std::string>{"h2", "http/1.1"});
  auto job = std::make_unique<typename Traits::Transport>(
      DEFAULT_PRIORITY, SocketTag(), &fixture.common, std::move(params),
      &delegate, nullptr);
  trace.connect_result = job->Connect();
  trace.pending_after_connect = resolver.has_pending_requests();

  if (test_case != ResolverCase::kCancelled) {
    resolver.ResolveAllPending();
  }
  if (test_case == ResolverCase::kCancelled ||
      test_case == ResolverCase::kCancelledAfterQueue) {
    job.reset();
  }
  tasks.RunUntilIdle();
  trace.completion_result = delegate.result;
  trace.pending_after_finish = resolver.has_pending_requests();
  trace.socket_allocations = fixture.socket_factory.allocation_count();

  if (job) {
    trace.error_info = job->GetResolveErrorInfo();
    trace.attempts = job->GetConnectionAttempts();
    auto socket = job->PassSocket();
    trace.socket_present = socket != nullptr;
    if (socket) {
      EXPECT_EQ(socket->GetPeerAddress(&trace.peer), OK);
      trace.socket_aliases = socket->GetDnsAliases();
    }
  }
  return trace;
}

}  // namespace

TEST(ConnectionJobsFamilyRustTest, ResolverRequestMatchesIndependentOriginal) {
  base::test::TaskEnvironment tasks;
  base::test::ScopedFeatureList features;
  features.InitAndDisableFeature(features::kHappyEyeballsV2);
  for (ResolverCase test_case :
       {ResolverCase::kSuccess, ResolverCase::kTimeout,
        ResolverCase::kCancelled, ResolverCase::kCancelledAfterQueue}) {
    SCOPED_TRACE(static_cast<int>(test_case));
    const auto original = RunResolverCase<
        Original, ConnectJobRustOriginalHostResolutionCallbackResult>(test_case,
                                                                       tasks);
    const auto production =
        RunResolverCase<Production, OnHostResolutionCallbackResult>(test_case,
                                                                    tasks);
    EXPECT_EQ(production, original);
    EXPECT_EQ(production.connect_result, ERR_IO_PENDING);
    EXPECT_TRUE(production.pending_after_connect);
    EXPECT_FALSE(production.pending_after_finish);

    switch (test_case) {
      case ResolverCase::kSuccess:
        EXPECT_EQ(production.completion_result, OK);
        EXPECT_EQ(production.error_info.error, OK);
        EXPECT_EQ(production.host_callbacks, 1);
        EXPECT_TRUE(production.callback_before_completion);
        EXPECT_EQ(production.callback_results, std::vector{ResolverEndpoint()});
        EXPECT_EQ(production.callback_aliases,
                  (base::flat_set<std::string>{"alias.example", "family.example"}));
        EXPECT_EQ(production.socket_allocations, 1);
        EXPECT_TRUE(production.socket_present);
        EXPECT_EQ(production.peer, Ip("2001:db8::1", 8443));
        EXPECT_EQ(production.socket_aliases,
                  (base::flat_set<std::string>{"alias.example", "family.example"}));
        EXPECT_TRUE(production.attempts.empty());
        break;
      case ResolverCase::kTimeout:
        EXPECT_EQ(production.completion_result, ERR_NAME_NOT_RESOLVED);
        EXPECT_EQ(production.error_info.error, ERR_DNS_TIMED_OUT);
        EXPECT_EQ(production.host_callbacks, 0);
        EXPECT_EQ(production.socket_allocations, 0);
        EXPECT_FALSE(production.socket_present);
        EXPECT_EQ(production.attempts,
                  ConnectionAttempts({{IPEndPoint(), ERR_NAME_NOT_RESOLVED}}));
        break;
      case ResolverCase::kCancelled:
      case ResolverCase::kCancelledAfterQueue:
        EXPECT_EQ(production.completion_result, ERR_IO_PENDING);
        EXPECT_EQ(production.host_callbacks, 0);
        EXPECT_EQ(production.socket_allocations, 0);
        EXPECT_FALSE(production.socket_present);
        break;
    }
  }
}

}  // namespace net::connection_jobs_family_test
