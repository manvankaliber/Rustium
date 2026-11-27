// Source-only ordinary family integration tests; not compiled or executed.
#include "net/socket/connection_jobs_family_rust_test_support.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "base/test/bind.h"
#include "net/dns/mock_host_resolver.h"
#include "net/http/http_stream_pool_test_util.h"
#include "net/socket/socket_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

#if defined(RUSTIUM_HOST_PORT_REFERENCE_PROBE)
#include <stdint.h>

#include "base/dcheck_is_on.h"
#include "net/http/rustium_reference_probe.h"
#endif

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

#if defined(RUSTIUM_HOST_PORT_REFERENCE_PROBE)
struct HostPortCalls {
  uint64_t empty;
  uint64_t format;
};

HostPortCalls ReadHostPortCalls() {
  return {rustium_reference_probe::HostPortEmptyCount(),
          rustium_reference_probe::HostPortFormatCount()};
}

HostPortCalls HostPortCallsSince(HostPortCalls before) {
  const auto after = ReadHostPortCalls();
  return {after.empty - before.empty, after.format - before.format};
}

class ResolverCaseProvider : public MockHostResolver {
 public:
  using MockHostResolver::CreateRequest;

  std::unique_ptr<ResolveHostRequest> CreateRequest(
      const HostPortPair& host,
      const NetworkAnonymizationKey& key,
      handles::NetworkHandle target_network,
      const NetLogWithSource& net_log,
      const std::optional<ResolveHostParameters>& parameters) override {
    const auto before = ReadHostPortCalls();
    auto request = MockHostResolver::CreateRequest(
        host, key, target_network, net_log, parameters);
    creation_calls = HostPortCallsSince(before);
    return request;
  }

  HostPortCalls creation_calls = {};
};
#else
using ResolverCaseProvider = MockHostResolver;
#endif

HostResolverEndpointResult ResolverEndpoint(bool use_host_port_pair) {
  HostResolverEndpointResult endpoint;
  endpoint.ip_endpoints = {Ip("2001:db8::1", 8443), Ip("192.0.2.1", 8443)};
  if (!use_host_port_pair) {
    endpoint.metadata.supported_protocol_alpns = {"h2"};
  }
  return endpoint;
}

template <class Traits, class CallbackResult>
ResolverTrace RunResolverCase(ResolverCase test_case,
                              bool use_host_port_pair,
                              base::test::TaskEnvironment& tasks) {
  // Both jobs consume the same scripted provider behavior. The mock
  // shares DNS name validation and transport primitives with production.
#if defined(RUSTIUM_HOST_PORT_REFERENCE_PROBE)
  tasks.RunUntilIdle();
  const auto before = ReadHostPortCalls();
#endif
  ResolverCaseProvider resolver;
  resolver.set_ondemand_mode(true);
  if (test_case == ResolverCase::kTimeout) {
    resolver.rules()->AddSimulatedTimeoutFailure("family.example");
  } else {
    resolver.rules()->AddRule(
        "family.example",
        MockHostResolverBase::RuleResolver::RuleResult(
            {ResolverEndpoint(use_host_port_pair)},
            {"alias.example", "family.example"}));
  }

  const MockTransportClientSocketFactory::Rule rule(
      MockTransportClientSocketFactory::Type::kSynchronous,
      std::vector{Ip("2001:db8::1", 8443)});
  Fixture<Traits> fixture;
  fixture.common.host_resolver = &resolver;
  fixture.socket_factory.SetRules(base::span_from_ref(rule));

  ResolverTrace trace;
  Delegate<Traits> delegate;
  using Endpoint = typename Traits::Params::Endpoint;
  Endpoint destination =
      use_host_port_pair
          ? Endpoint(HostPortPair("family.example", 443))
          : Endpoint(url::SchemeHostPort("https", "family.example", 443));
  base::flat_set<std::string> alpns;
  if (!use_host_port_pair) {
    alpns = {"h2", "http/1.1"};
  }
  auto params = base::MakeRefCounted<typename Traits::Params>(
      std::move(destination), NetworkAnonymizationKey(),
      SecureDnsPolicy::kAllow,
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
      std::move(alpns));
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
#if defined(RUSTIUM_HOST_PORT_REFERENCE_PROBE)
  job.reset();
  tasks.RunUntilIdle();
  if (use_host_port_pair) {
    if (DCHECK_IS_ON()) {
      EXPECT_GT(resolver.creation_calls.empty, 0u);
    } else {
      EXPECT_EQ(resolver.creation_calls.empty, 0u);
    }
    EXPECT_EQ(resolver.creation_calls.format, 0u);

    // The uncached mock matches rules against the raw hostname. The
    // HostPortPair checks during request creation belong to the provider.
    const auto total = HostPortCallsSince(before);
    EXPECT_EQ(total.empty, resolver.creation_calls.empty);
    EXPECT_EQ(total.format, resolver.creation_calls.format);
  }
#endif
  return trace;
}

void ExpectResolverCases(bool use_host_port_pair) {
  base::test::TaskEnvironment tasks;
  base::test::ScopedFeatureList features;
  features.InitAndDisableFeature(features::kHappyEyeballsV2);
  for (ResolverCase test_case :
       {ResolverCase::kSuccess, ResolverCase::kTimeout,
        ResolverCase::kCancelled, ResolverCase::kCancelledAfterQueue}) {
    SCOPED_TRACE(static_cast<int>(test_case));
    const auto original = RunResolverCase<
        Original, ConnectJobRustOriginalHostResolutionCallbackResult>(
        test_case, use_host_port_pair, tasks);
    const auto production =
        RunResolverCase<Production, OnHostResolutionCallbackResult>(
            test_case, use_host_port_pair, tasks);
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
        EXPECT_EQ(production.callback_results,
                  std::vector{ResolverEndpoint(use_host_port_pair)});
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

}  // namespace

TEST(ConnectionJobsFamilyRustTest, ResolverRequestMatchesIndependentOriginal) {
  ExpectResolverCases(false);
}

TEST(ConnectionJobsFamilyRustTest,
     HostPortPairResolverRequestMatchesIndependentOriginal) {
  ExpectResolverCases(true);
}

#if defined(RUSTIUM_HOST_PORT_REFERENCE_PROBE)
TEST(ConnectionJobsFamilyRustTest, HostPortPairProbeControls) {
  base::test::TaskEnvironment tasks;
  HostPortPair host("family.example", 443);

  tasks.RunUntilIdle();
  auto before = ReadHostPortCalls();
  EXPECT_FALSE(host.IsEmpty());
  tasks.RunUntilIdle();
  const auto empty = HostPortCallsSince(before);
  EXPECT_EQ(empty.empty, 1u);
  EXPECT_EQ(empty.format, 0u);

  before = ReadHostPortCalls();
  EXPECT_EQ(host.HostForURL(), "family.example");
  tasks.RunUntilIdle();
  const auto format = HostPortCallsSince(before);
  EXPECT_EQ(format.empty, 0u);
  EXPECT_EQ(format.format, 1u);
}
#endif

namespace {

struct ConnectorRaceTrace {
  int initial_result = ERR_UNEXPECTED;
  bool ipv6_started_immediately = false;
  bool ipv4_started_early = false;
  bool ipv4_started_at_fallback = false;
  int completion_count = 0;
  int result = ERR_IO_PENDING;
  bool loser_detached = false;
  IPEndPoint peer;
  ServiceEndpoint resolved_endpoint;
  ConnectionAttempts attempts;

  bool operator==(const ConnectorRaceTrace&) const = default;
};

template <class Traits>
struct ConnectorRaceDelegate final : Traits::Base::Delegate {
  int completion_count = 0;
  int result = ERR_IO_PENDING;

  void OnConnectJobComplete(int value, typename Traits::Base*) override {
    ++completion_count;
    result = value;
  }

  void OnNeedsProxyAuth(const HttpResponseInfo&,
                       HttpAuthController*,
                       base::OnceClosure,
                       typename Traits::Base*) override {
    ADD_FAILURE() << "Unexpected proxy auth for a direct TCP connection";
  }
};

template <class Traits>
ConnectorRaceTrace RunConnectorRace(bool ipv4_wins,
                                    base::test::TaskEnvironment& tasks) {
  const IPEndPoint ipv6 = Ip("2001:db8::1", 80);
  const IPEndPoint ipv4 = Ip("192.0.2.1", 80);
  ServiceEndpoint endpoint;
  endpoint.ipv6_endpoints = {ipv6};
  endpoint.ipv4_endpoints = {ipv4};
  FakeServiceEndpointResolver resolver;
  resolver.ConfigureDefaultResolution()
      .add_endpoint(endpoint)
      .CompleteStartSynchronously(OK);

  MockConnectCompleter ipv6_connect;
  MockConnectCompleter ipv4_connect;
  SequencedSocketData ipv6_data;
  SequencedSocketData ipv4_data;
  MockConnect ipv6_completion(&ipv6_connect);
  MockConnect ipv4_completion(&ipv4_connect);
  ipv6_completion.peer_addr = ipv6;
  ipv4_completion.peer_addr = ipv4;
  ipv6_data.set_connect_data(ipv6_completion);
  ipv4_data.set_connect_data(ipv4_completion);
  ipv6_data.set_expected_addresses(AddressList(ipv6));
  ipv4_data.set_expected_addresses(AddressList(ipv4));

  MockClientSocketFactory sockets;
  sockets.AddSocketDataProvider(&ipv6_data);
  sockets.AddSocketDataProvider(&ipv4_data);
  Fixture<Traits> fixture;
  fixture.common.client_socket_factory = &sockets;
  fixture.common.host_resolver = &resolver;
  ConnectorRaceDelegate<Traits> delegate;
  auto job = std::make_unique<typename Traits::Tcp>(
      DEFAULT_PRIORITY, SocketTag(), &fixture.common, fixture.params,
      &delegate, nullptr);
  ConnectorRaceTrace trace;
  trace.initial_result = job->Connect();
  tasks.RunUntilIdle();
  trace.ipv6_started_immediately = ipv6_connect.is_connecting();

  // Test the pre-migration contract against mock time, including the boundary
  // immediately before the second connector is permitted to start.
  constexpr auto fallback = TcpConnectJobRustOriginal::kIPv6FallbackTime;
  tasks.FastForwardBy(fallback - base::Milliseconds(1));
  trace.ipv4_started_early = ipv4_connect.is_connecting();
  tasks.FastForwardBy(base::Milliseconds(1));
  trace.ipv4_started_at_fallback = ipv4_connect.is_connecting();
  EXPECT_TRUE(sockets.AllDataProvidersUsed());
  EXPECT_EQ(delegate.completion_count, 0);

  auto& winner = ipv4_wins ? ipv4_connect : ipv6_connect;
  auto& losing_data = ipv4_wins ? ipv6_data : ipv4_data;
  if (winner.is_connecting()) {
    winner.Complete(OK);
    tasks.RunUntilIdle();
  }

  trace.loser_detached = losing_data.socket() == nullptr;
  if (delegate.result == OK) {
    trace.resolved_endpoint = job->PassServiceEndpoint();
    EXPECT_EQ(trace.resolved_endpoint, endpoint);
    auto socket = job->PassSocket();
    EXPECT_NE(socket, nullptr);
    if (socket) {
      EXPECT_EQ(socket->GetPeerAddress(&trace.peer), OK);
    }
  }
  trace.attempts = job->GetConnectionAttempts();
  EXPECT_TRUE(trace.attempts.empty());
  job.reset();
  tasks.RunUntilIdle();
  trace.completion_count = delegate.completion_count;
  trace.result = delegate.result;
  return trace;
}

}  // namespace

TEST(ConnectionJobsFamilyRustTest, HappyEyeballsFallbackMatchesOriginal) {
  base::test::TaskEnvironment tasks(
      base::test::TaskEnvironment::TimeSource::MOCK_TIME);
  base::test::ScopedFeatureList features;
  features.InitWithFeatures({features::kHappyEyeballsV2},
                            {features::kAdjustIPv6FallbackTime,
                             features::kIPv6FallbackBasedOnRTT});
  for (bool ipv4_wins : {false, true}) {
    SCOPED_TRACE(ipv4_wins);
    const auto original = RunConnectorRace<Original>(ipv4_wins, tasks);
    const auto production = RunConnectorRace<Production>(ipv4_wins, tasks);
    EXPECT_EQ(production, original);
    EXPECT_EQ(production.initial_result, ERR_IO_PENDING);
    EXPECT_TRUE(production.ipv6_started_immediately);
    EXPECT_FALSE(production.ipv4_started_early);
    EXPECT_TRUE(production.ipv4_started_at_fallback);
    EXPECT_TRUE(production.loser_detached);
    EXPECT_EQ(production.completion_count, 1);
    EXPECT_EQ(production.result, OK);
    EXPECT_EQ(production.peer,
              ipv4_wins ? Ip("192.0.2.1", 80) : Ip("2001:db8::1", 80));
  }
}

}  // namespace net::connection_jobs_family_test
