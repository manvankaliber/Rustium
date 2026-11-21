// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/raw_span.h"
#include "base/memory/raw_ptr.h"
#include "net/socket/tcp_connect_job.h"
#include "net/socket/tcp_connect_job_rust_ffi.h"
#include "net/socket/connection_jobs_upper_rust_ffi.h"
#include "net/socket/connect_job_rust_native.h"
#include "net/socket/tcp_connect_fallback_rust_native.h"

#include <algorithm>
#include <memory>
#include "base/memory/raw_ptr_exclusion.h"
#include <optional>
#include <ranges>
#include <set>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "base/check_op.h"
#include "base/containers/flat_set.h"
#include "base/containers/span.h"
#include "base/containers/to_vector.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/notreached.h"
#include "base/task/single_thread_task_runner.h"
#include "base/time/time.h"
#include "base/trace_event/trace_event.h"
#include "base/types/expected.h"
#include "net/base/features.h"
#include "net/base/host_port_pair.h"
#include "net/base/ip_endpoint.h"
#include "net/base/net_errors.h"
#include "net/base/trace_constants.h"
#include "net/dns/public/host_resolver_results.h"
#include "net/dns/public/secure_dns_policy.h"
#include "net/http/http_server_properties.h"
#include "net/log/net_log_event_type.h"
#include "net/nqe/network_quality_estimator.h"
#include "net/socket/socket_tag.h"
#include "net/socket/ssl_client_socket.h"
#include "net/socket/tcp_connect_job_connector.h"
#include "net/socket/transport_connect_job.h"
#include "net/ssl/ssl_config_service.h"
#include "url/scheme_host_port.h"
#include "url/url_constants.h"

namespace net {

namespace {

// Converts `endpoint` to a HostPortPair.
HostPortPair ToLegacyDestinationEndpoint(
    const TransportSocketParams::Endpoint& endpoint) {
  if (std::holds_alternative<url::SchemeHostPort>(endpoint)) {
    return HostPortPair::FromSchemeHostPort(
        std::get<url::SchemeHostPort>(endpoint));
  }

  DCHECK(std::holds_alternative<HostPortPair>(endpoint));
  return std::get<HostPortPair>(endpoint);
}





// Returns true if the given `endpoint` is present in the provided `results`
// list. This is used to check if an actively connecting stale endpoint is
// still valid when fresh DNS results arrive, allowing the stale connector to
// be promoted to a fresh connector.


}  // namespace


// Invocation-local observer. Invalidated at the most-derived destructor entry.
// No typed native pointer crosses the Rust ABI or survives a service call.
struct tcp_connect_jobRustBridge : connection_jobs_upper_rust::Invocation {
  using Op = tcp_connect_job_rust::Operation;
  using Action = tcp_connect_job_rust::Action;
  raw_ptr<const TcpConnectJob> owner;
  raw_ptr<TcpConnectJob> mutable_owner = nullptr;

  int64_t state_index = 0;
  // Ephemeral primitive-call observations; never exposed to Rust.
  RAW_PTR_EXCLUSION TcpConnectJob::Connector* connector = nullptr;
  RAW_PTR_EXCLUSION const TcpConnectJob::Connector* input_connector = nullptr;
  RAW_PTR_EXCLUSION const TcpConnectJob::Connector* other = nullptr;
  RAW_PTR_EXCLUSION const IPEndPoint* input_ip = nullptr;
  RAW_PTR_EXCLUSION const ServiceEndpoint* endpoint = nullptr;
  RAW_PTR_EXCLUSION const ServiceEndpoint* endpoint_output = nullptr;
  base::raw_span<const ServiceEndpoint> results;
  raw_ptr<const base::flat_set<std::string>> aliases = nullptr;
  raw_ptr<TcpConnectJob::IPEndPointInfo> ip_output = nullptr;
  std::optional<IPEndPoint> ip, saved_ip;
  bool v6 = false;
  std::optional<ConnectJobRustBridge> base;
  std::unique_ptr<StreamSocket> socket_payload;
  std::optional<base::flat_set<std::string>> aliases_payload;
  base::TimeDelta delay;
  TcpFallbackRustBridge fallback;
  TcpConnectJob::ConnectionState& State() {
    CHECK(mutable_owner);
    return state_index ? mutable_owner->stale_state_ : mutable_owner->fresh_state_;
  }
  const TcpConnectJob::ConnectionState& ReadState() const {
    return state_index ? owner->stale_state_ : owner->fresh_state_;
  }
  base::span<const IPEndPoint> Ips() const {
    return v6 ? base::span(endpoint->ipv6_endpoints) : base::span(endpoint->ipv4_endpoints);
  }

  explicit tcp_connect_jobRustBridge(TcpConnectJob* value) : Invocation(static_cast<const ConnectJob*>(value)), owner(value), mutable_owner(value) {
    Register(&Clear); base.emplace(value);
  }
  explicit tcp_connect_jobRustBridge(const TcpConnectJob* value) : Invocation(static_cast<const ConnectJob*>(value)), owner(value) {
    Register(&Clear); base.emplace(value);
  }
  ~tcp_connect_jobRustBridge() { base.reset(); Unregister(); }
  static void Clear(Invocation* value) {
    auto& c = *static_cast<tcp_connect_jobRustBridge*>(value);
    c.owner = nullptr; c.mutable_owner = nullptr;
    c.connector = nullptr; c.input_connector = nullptr; c.other = nullptr; c.input_ip = nullptr; c.endpoint = nullptr; c.endpoint_output = nullptr; c.results = {}; c.aliases = nullptr;
  }
  int64_t Run(Op operation, int64_t argument = 0) {
    return connection_jobs_rust_run(2, static_cast<uint32_t>(operation),
                                    argument, this, &Service);
  }
  static bool Service(void* context, uint32_t module, uint32_t action,
                      int64_t argument, int64_t* output) {
    auto& c = *static_cast<tcp_connect_jobRustBridge*>(context);
    *output = 0;
    if (!c.owner) return false;
    if (action == UINT32_MAX) return true;
    if (module == 0) {
      CHECK(c.base);
      return ConnectJobRustBridge::Service(&*c.base, action, argument, output);
    }
    const auto& j = *c.owner;
    // Direct safe Rust fallback helpers project only native providers here.
    if (module == 2 && action >= 1000 && action <= 1018) {
      c.fallback.common = j.common_connect_job_params();
      c.fallback.params = j.params_.get();
      return TcpFallbackRustBridge::Service(&c.fallback, module, action, argument, output);
    }
    CHECK_EQ(module, 2u);
    // Mutable access is requested only in action arms that require it. Const
    // wrappers never obtain a mutable receiver and never commit a snapshot.
    auto Mutable = [&]() -> TcpConnectJob& { CHECK(c.mutable_owner); return *c.mutable_owner; };
    switch (static_cast<Action>(action)) {
      case Action::CheckNotDone: { CHECK(!j.is_done_); break; }
      case Action::CheckDnsIncomplete: { DCHECK(!j.dns_request_complete_); break; }
      case Action::CheckPendingDifferent: { DCHECK_NE(argument, ERR_IO_PENDING); break; }
      case Action::CheckFinalEndpointEmpty: { DCHECK(!j.final_service_endpoint_); break; }
      case Action::CheckComplete: { DCHECK(j.is_done_); break; }
      case Action::CheckHasFinal: { DCHECK(j.final_service_endpoint_); break; }
      case Action::CheckCrypto: { DCHECK(argument); break; }
      case Action::SelectState: { *output = c.state_index; c.state_index = argument; break; }
      case Action::HasPrimary: { *output = !!c.ReadState().primary_connector; break; }
      case Action::HasV4: { *output = !!c.ReadState().ipv4_connector; break; }
      case Action::PrimaryDone: { *output = c.ReadState().primary_connector->is_done(); break; }
      case Action::V4Done: { *output = c.ReadState().ipv4_connector->is_done(); break; }
      case Action::PrimaryWaitingDns: { *output = c.ReadState().primary_connector->is_waiting_on_dns(); break; }
      case Action::V4WaitingDns: { *output = c.ReadState().ipv4_connector->is_waiting_on_dns(); break; }
      case Action::PrimaryWaitingEndpoint: { *output = c.ReadState().primary_connector->is_waiting_for_endpoint(); break; }
      case Action::V4WaitingEndpoint: { *output = c.ReadState().ipv4_connector->is_waiting_for_endpoint(); break; }
      case Action::PrimaryHasAddress: { *output = c.ReadState().primary_connector->current_address().has_value(); break; }
      case Action::V4HasAddress: { *output = c.ReadState().ipv4_connector->current_address().has_value(); break; }
      case Action::PrimaryLoad: { *output = c.ReadState().primary_connector->GetLoadState(); break; }
      case Action::V4Load: { *output = c.ReadState().ipv4_connector->GetLoadState(); break; }
      case Action::PrimaryV6: { *output = c.ReadState().primary_connector->is_connecting_to_ipv6(); break; }
      case Action::SelectPrimary: { c.connector = c.State().primary_connector.get(); break; }
      case Action::SelectV4: { c.connector = c.State().ipv4_connector.get(); break; }
      case Action::AdvanceConnector: { *output = c.connector->TryAdvanceState(); break; }
      case Action::HasRequest: { *output = !!j.dns_request_; break; }
      case Action::HasOverride: { *output = j.endpoint_override_.has_value(); break; }
      case Action::DisableStale: { *output = j.disable_stale_dns_; break; }
      case Action::DualRace: { NOTREACHED(); }
      case Action::OptimisticEnabled: { *output = base::FeatureList::IsEnabled(features::kOptimisticDnsForTcp); break; }
      case Action::StaleConnectorsFeature: { *output = features::kUseStaleConnectorsForOptimisticDns.Get(); break; }
      case Action::RequestStale: { *output = j.dns_request_->IsStaleWhileRefreshing(); break; }
      case Action::DnsComplete: { *output = j.dns_request_complete_; break; }
      case Action::SetDnsComplete: { Mutable().dns_request_complete_ = true; break; }
      case Action::DnsStart: { auto now = base::TimeTicks::Now(); if (c.owner) Mutable().connect_timing_.domain_lookup_start = now; break; }
      case Action::DnsEnd: { auto now = base::TimeTicks::Now(); if(c.owner) { Mutable().connect_timing_.domain_lookup_end = now; Mutable().connect_timing_.connect_start = now; } break; }
      case Action::MakeRequest: { HostResolver::ResolveHostParameters p; p.initial_priority = j.priority();
 p.secure_dns_policy = j.params_->secure_dns_policy();
 p.cache_usage = static_cast<HostResolver::ResolveHostParameters::CacheUsage>(argument);
 auto request = j.common_connect_job_params()->host_resolver->CreateServiceEndpointRequest(
 HostResolver::Host(j.params_->destination()), j.params_->network_anonymization_key(),
 j.params_->target_network(), j.net_log(), p);
 if(c.owner) Mutable().dns_request_ = std::move(request); break; }
      case Action::CacheDefault: { *output = static_cast<int>(HostResolver::ResolveHostParameters{}.cache_usage); break; }
      case Action::CacheStale: { *output = static_cast<int>(HostResolver::ResolveHostParameters::CacheUsage::STALE_ALLOWED_WHILE_REFRESHING); break; }
      case Action::StartRequest: { *output = Mutable().dns_request_->Start(c.mutable_owner.get()); break; }
      case Action::Priority: { Mutable().dns_request_->ChangeRequestPriority(static_cast<RequestPriority>(argument)); break; }
      case Action::ResolveError: { auto info = j.dns_request_->GetResolveErrorInfo(); if(c.owner) Mutable().resolve_error_info_ = info; break; }
      case Action::ClearAttempts: { Mutable().connection_attempts_.clear(); break; }
      case Action::HasCallback: { *output = !j.params_->host_resolution_callback().is_null(); break; }
      case Action::HostCallback: { *output = j.params_->host_resolution_callback().Run(
 ToLegacyDestinationEndpoint(j.params_->destination()), c.results, *c.aliases) ==
 OnHostResolutionCallbackResult::kMayBeDeletedAsync; break; }
      case Action::WaitingDeletion: { *output = j.waiting_on_possible_async_deletion_count_; break; }
      case Action::IncrementWaiting: { ++Mutable().waiting_on_possible_async_deletion_count_; break; }
      case Action::DecrementWaiting: { DCHECK_GT(j.waiting_on_possible_async_deletion_count_, 0u); --Mutable().waiting_on_possible_async_deletion_count_; break; }
      case Action::PostAdvance: { base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
 FROM_HERE, base::BindOnce(&TcpConnectJob::TryAdvanceWaitingConnectorsAsync,
 Mutable().weak_ptr_factory_.GetWeakPtr(), argument != 0)); break; }
      case Action::ResetIndex: { c.State().current_endpoint_index = 0; break; }
      case Action::SetIndex: { c.State().current_endpoint_index = static_cast<size_t>(argument); break; }
      case Action::GetIndex: { *output = c.ReadState().current_endpoint_index; break; }
      case Action::IncrementIndex: { ++c.State().current_endpoint_index; break; }
      case Action::MakeStalePrimary: { Mutable().stale_state_.primary_connector = std::make_unique<TcpConnectJob::Connector>(c.mutable_owner.get(), "stale_primary"); break; }
      case Action::MakeSecondary: { c.State().ipv4_connector = std::make_unique<TcpConnectJob::Connector>(c.mutable_owner.get(), "second"); break; }
      case Action::MakeInitialPrimary: { Mutable().fresh_state_.primary_connector = std::make_unique<TcpConnectJob::Connector>(c.mutable_owner.get(), "first"); break; }
      case Action::SwapConnectors: { std::swap(c.State().primary_connector, c.State().ipv4_connector); break; }
      case Action::CheckNoSecondary: { DCHECK(!c.State().ipv4_connector); break; }
      case Action::SlowEvent: { j.net_log().AddEvent(NetLogEventType::TCP_CONNECT_JOB_CREATE_SECOND_CONNECTOR); break; }
      case Action::SlowRunning: { *output = c.ReadState().slow_timer.IsRunning(); break; }
      case Action::StopSlow: { c.State().slow_timer.Stop(); break; }
      case Action::FallbackDuration: { c.delay = c.fallback.delay; break; }
      case Action::StartTimePresent: {
        *output = !c.ReadState().primary_connector->start_time().is_null();
        break;
      }
      case Action::SubtractElapsed: { const auto start = c.State().primary_connector->start_time();
 const auto now = base::TimeTicks::Now(); if(!c.owner) return false;
 c.delay = c.delay - (now - start); break; }
      case Action::DelayNegative: { *output = c.delay < base::TimeDelta(); break; }
      case Action::ZeroDelay: { c.delay = base::TimeDelta(); break; }
      case Action::StartSlow: { c.State().slow_timer.Start(FROM_HERE, c.delay,
 base::BindOnce(&TcpConnectJob::OnSlow, base::Unretained(c.mutable_owner.get()), c.state_index != 0)); break; }
      case Action::ResetPrimary: { c.connector = nullptr; c.State().primary_connector.reset(); break; }
      case Action::ResetV4: { c.connector = nullptr; c.State().ipv4_connector.reset(); break; }
      case Action::SetPreferV6: { c.State().prefer_ipv6 = true; break; }
      case Action::PreferV6: { *output = c.ReadState().prefer_ipv6; break; }
      case Action::InputIsPrimary: { *output = c.input_connector == c.ReadState().primary_connector.get(); break; }
      case Action::InputIsStale: { *output = j.IsStaleConnector(*c.input_connector); break; }
      case Action::OtherExists: { *output = c.other != nullptr; break; }
      case Action::OtherWaitingEndpoint: { *output = c.other->is_waiting_for_endpoint(); break; }
      case Action::InputDone: { *output = c.input_connector->is_done(); break; }
      case Action::SetOtherV4: { c.other = c.ReadState().ipv4_connector.get(); break; }
      case Action::SetOtherPrimary: { c.other = c.ReadState().primary_connector.get(); break; }
      case Action::DnsResults: { DCHECK(!j.endpoint_override_); c.results = j.dns_request_->GetEndpointResults(); break; }
      case Action::OverrideResults: { DCHECK(j.endpoint_override_); c.results = j.endpoint_override_->endpoints; break; }
      case Action::DnsAliases: { DCHECK(!j.endpoint_override_); c.aliases = &j.dns_request_->GetDnsAliasResults(); break; }
      case Action::OverrideAliases: { DCHECK(j.endpoint_override_); c.aliases = &j.endpoint_override_->dns_aliases; break; }
      case Action::DnsCrypto: { DCHECK(!j.endpoint_override_); *output = j.dns_request_->EndpointsCryptoReady(); break; }
      case Action::ResultsCount: { *output = c.results.size(); break; }
      case Action::SelectEndpoint: { c.endpoint = &c.results[static_cast<size_t>(argument)]; break; }
      case Action::Alternative: { *output = !c.endpoint->metadata.supported_protocol_alpns.empty(); break; }
      case Action::Ech: { *output = !c.endpoint->metadata.ech_config_list.empty(); break; }
      case Action::AlpnCount: { *output = c.endpoint->metadata.supported_protocol_alpns.size(); break; }
      case Action::AlpnMatches: { *output = j.params_->supported_alpns().contains(c.endpoint->metadata.supported_protocol_alpns[static_cast<size_t>(argument)]); break; }
      case Action::Svcb: { *output = j.is_svcb_optional_; break; }
      case Action::SetSvcb: { Mutable().is_svcb_optional_ = argument != 0; break; }
      case Action::Https: { const auto* p = std::get_if<url::SchemeHostPort>(&j.params_->destination());
 *output = p && p->scheme() == url::kHttpsScheme; break; }
      case Action::EchEnabled: { const auto* p = std::get_if<url::SchemeHostPort>(&j.params_->destination());
 *output = j.common_connect_job_params()->ssl_client_context->IsEchEnabled(p->host()); break; }
      case Action::SetFamily: { c.v6 = argument != 0; break; }
      case Action::IpCount: { *output = c.Ips().size(); break; }
      case Action::SelectIp: { c.ip = c.Ips()[static_cast<size_t>(argument)]; break; }
      case Action::IpAlreadyAttempted: { *output = j.attempted_addresses_.contains(*c.ip); break; }
      case Action::InsertIp: { *output = Mutable().attempted_addresses_.insert(*c.ip).second; break; }
      case Action::ReturnIp: { *c.ip_output = *c.ip; break; }
      case Action::InputIpV6: { *output = c.input_ip->GetFamily() == ADDRESS_FAMILY_IPV6; break; }
      case Action::IpMatchesInput: { *output = *c.ip == *c.input_ip; break; }
      case Action::ReturnEndpoint: { c.endpoint_output = c.endpoint; break; }
      case Action::PrimaryAddressToInput: { c.ip = c.State().primary_connector->current_address(); break; }
      case Action::V4AddressToInput: { c.ip = c.State().ipv4_connector->current_address(); break; }
      case Action::ConnectorHasAddress: { *output = c.connector->current_address().has_value(); break; }
      case Action::ConnectorAddressToInput: { c.ip = c.connector->current_address(); break; }
      case Action::IpEqualsSaved: { *output = c.Ips()[static_cast<size_t>(argument)] == *c.saved_ip; break; }
      case Action::SaveIp: { c.saved_ip = c.ip; break; }
      case Action::PromotePrimary: { Mutable().fresh_state_.primary_connector = std::move(Mutable().stale_state_.primary_connector); break; }
      case Action::PromoteV4: { Mutable().fresh_state_.ipv4_connector = std::move(Mutable().stale_state_.ipv4_connector); break; }
      case Action::StoreResolution: { auto details = j.dns_request_->GetResolutionDetails(); if(c.owner) Mutable().resolution_details_ = std::move(details); break; }
      case Action::CheckConnector: { DCHECK(c.connector); break; }
      case Action::PassSocket: { c.socket_payload = c.connector->PassSocket();
 if (!c.owner) return false;
 c.aliases_payload = *c.aliases;
 c.base->socket_input = &c.socket_payload; c.base->aliases = &c.aliases_payload; break; }
      case Action::PassFinalEndpoint: { Mutable().final_service_endpoint_ = c.connector->PassFinalServiceEndpoint(); c.endpoint = &*j.final_service_endpoint_; break; }
      case Action::AttemptsEmpty: { *output = j.connection_attempts_.empty(); break; }
      case Action::AddEmptyAttempt: { Mutable().connection_attempts_.emplace_back(IPEndPoint(), static_cast<int>(argument)); break; }
      case Action::LastAttempt: { *output = j.connection_attempts_.back().result; break; }
      case Action::CheckStaleUnset: { CHECK(!j.is_connected_via_stale_dns_.has_value()); break; }
      case Action::StoreStale: { Mutable().is_connected_via_stale_dns_ = argument != 0; break; }
      case Action::ConnectorIsStale: { *output = j.IsStaleConnector(*c.connector); break; }
      case Action::ResetRequest: { c.results = {}; c.aliases = nullptr; c.endpoint = nullptr; Mutable().dns_request_.reset(); break; }
      case Action::InvalidateTasks: { Mutable().weak_ptr_factory_.InvalidateWeakPtrs(); break; }
      case Action::SetDone: { Mutable().is_done_ = true; break; }
      case Action::Notify: { NOTREACHED(); }
      case Action::CheckV2: { DCHECK(base::FeatureList::IsEnabled(features::kHappyEyeballsV2)); break; }
      case Action::CheckInitialIp: { DCHECK(!c.endpoint->ipv4_endpoints.empty() || !c.endpoint->ipv6_endpoints.empty()); break; }
      case Action::CheckUsable: { DCHECK(argument); break; }
      case Action::StaleValue: { *output = j.is_connected_via_stale_dns_.value_or(false); break; }

      case Action::CheckStateActive: { CHECK(argument); break; }
      case Action::HasSsl: { *output = j.common_connect_job_params()->ssl_client_context != nullptr; break; }
      default: NOTREACHED();
    }
    return c.owner != nullptr;
  }
};

TcpConnectJob::ServiceEndpointOverride::ServiceEndpointOverride(
    ServiceEndpoint endpoint,
    base::flat_set<std::string> dns_aliases)
    : endpoints({std::move(endpoint)}), dns_aliases(std::move(dns_aliases)) {}
TcpConnectJob::ServiceEndpointOverride::ServiceEndpointOverride(
    ServiceEndpointOverride&&) = default;
TcpConnectJob::ServiceEndpointOverride::ServiceEndpointOverride(
    const ServiceEndpointOverride&) = default;
TcpConnectJob::ServiceEndpointOverride::~ServiceEndpointOverride() = default;

TcpConnectJob::TcpConnectJob(
    RequestPriority priority,
    const SocketTag& socket_tag,
    const CommonConnectJobParams* common_connect_job_params,
    const scoped_refptr<TransportSocketParams>& params,
    ConnectJob::Delegate* delegate,
    const NetLogWithSource* net_log,
    std::optional<ServiceEndpointOverride> endpoint_result_override,
    bool disable_stale_dns)
    : ConnectJob(priority,
                 socket_tag,
                 ConnectionTimeout(),
                 common_connect_job_params,
                 delegate,
                 net_log,
                 NetLogSourceType::TCP_CONNECT_JOB,
                 NetLogEventType::TCP_CONNECT_JOB_CONNECT),
      params_(params),
      endpoint_override_(std::move(endpoint_result_override)),
      disable_stale_dns_(disable_stale_dns) {
  tcp_connect_jobRustBridge c(this);
  c.Run(tcp_connect_jobRustBridge::Op::Init);
}

TcpConnectJob::~TcpConnectJob() {
  tcp_connect_jobRustBridge::Invalidate(static_cast<const ConnectJob*>(this));
}

LoadState TcpConnectJob::GetLoadState() const {
tcp_connect_jobRustBridge c(this); return static_cast<LoadState>(c.Run(tcp_connect_jobRustBridge::Op::LoadState));
}

bool TcpConnectJob::HasEstablishedConnection() const {
  return has_established_connection_;
}

ConnectionAttempts TcpConnectJob::GetConnectionAttempts() const {
  return connection_attempts_;
}

ResolveErrorInfo TcpConnectJob::GetResolveErrorInfo() const {
  return resolve_error_info_;
}

std::optional<HostResolverEndpointResult>
TcpConnectJob::GetHostResolverEndpointResult() const {
  // Callers should call GetServiceEndpoint() instead, if they're using this
  // class.
  NOTREACHED();
}

std::optional<ResolutionDetails> TcpConnectJob::GetResolutionDetails() const {
  return resolution_details_;
}

ServiceEndpoint TcpConnectJob::PassServiceEndpoint() {
  CHECK(final_service_endpoint_);
  return std::move(final_service_endpoint_).value();
}

base::TimeDelta TcpConnectJob::ConnectionTimeout() {
return base::Microseconds(connection_jobs_rust_run(2, static_cast<uint32_t>(tcp_connect_job_rust::Operation::Timeout), 0, nullptr, nullptr));
}

size_t TcpConnectJob::GetFreshConnectorCountForTesting() const {
tcp_connect_jobRustBridge c(this); return c.Run(tcp_connect_jobRustBridge::Op::Count, 0);
}

size_t TcpConnectJob::GetStaleConnectorCountForTesting() const {
tcp_connect_jobRustBridge c(this); return c.Run(tcp_connect_jobRustBridge::Op::Count, 1);
}

int TcpConnectJob::ConnectInternal() {
tcp_connect_jobRustBridge c(this); return c.Run(tcp_connect_jobRustBridge::Op::Connect);
}

void TcpConnectJob::ChangePriorityInternal(RequestPriority priority) {
tcp_connect_jobRustBridge c(this); c.Run(tcp_connect_jobRustBridge::Op::Priority, priority);
}

int TcpConnectJob::DoServiceEndpointsUpdated(
    std::optional<int> dns_request_final_result) {
tcp_connect_jobRustBridge c(this);
  // Presence selects update vs finished; zero is a valid synchronous result.
  if (dns_request_final_result) return c.Run(tcp_connect_jobRustBridge::Op::UpdateSync, *dns_request_final_result);
  return connection_jobs_rust_run(2, 28, 0, &c, &tcp_connect_jobRustBridge::Service);
}

void TcpConnectJob::TryAdvanceWaitingConnectorsAsync(
    bool decrement_waiting_on_possible_async_deletion_count) {
tcp_connect_jobRustBridge c(this); c.Run(tcp_connect_jobRustBridge::Op::AdvanceAsync, decrement_waiting_on_possible_async_deletion_count);
}

int TcpConnectJob::DoTryAdvanceWaitingConnectors() {
tcp_connect_jobRustBridge c(this); return c.Run(tcp_connect_jobRustBridge::Op::Advance);
}

int TcpConnectJob::DoConnectorComplete(int result, Connector& connector) {
tcp_connect_jobRustBridge c(this);
  c.connector = &connector;
  return c.Run(tcp_connect_jobRustBridge::Op::Complete, result);
}

void TcpConnectJob::OnServiceEndpointsUpdated() {
tcp_connect_jobRustBridge c(this); c.Run(tcp_connect_jobRustBridge::Op::Updated);
}

void TcpConnectJob::OnServiceEndpointRequestFinished(int rv) {
tcp_connect_jobRustBridge c(this); c.Run(tcp_connect_jobRustBridge::Op::Finished, rv);
}

void TcpConnectJob::OnConnectorComplete(int result, Connector& connector) {
tcp_connect_jobRustBridge c(this);
  c.connector = &connector;
  c.Run(tcp_connect_jobRustBridge::Op::CompleteAsync, result);
}

void TcpConnectJob::OnSlow(bool is_stale) {
tcp_connect_jobRustBridge c(this); c.Run(tcp_connect_jobRustBridge::Op::Slow, is_stale);
}

// static
base::TimeDelta TcpConnectJob::GetIPv6FallbackTime(
    const CommonConnectJobParams* common_connect_job_params,
    const TransportSocketParams* params) {
  base::TimeDelta fallback_time = kIPv6FallbackTime;

  if (base::FeatureList::IsEnabled(features::kAdjustIPv6FallbackTime)) {
    fallback_time = features::kIPv6FallbackTime.Get();
  }

  if (base::FeatureList::IsEnabled(features::kIPv6FallbackBasedOnRTT)) {
    std::optional<base::TimeDelta> rtt =
        [&]() -> std::optional<base::TimeDelta> {
      // 1. Try to get destination-specific RTT from HttpServerProperties.
      if (common_connect_job_params->http_server_properties) {
        url::SchemeHostPort scheme_host_port;
        if (std::holds_alternative<url::SchemeHostPort>(
                params->destination())) {
          scheme_host_port =
              std::get<url::SchemeHostPort>(params->destination());
        } else {
          const auto& host_port_pair =
              std::get<HostPortPair>(params->destination());
          scheme_host_port = url::SchemeHostPort("https", host_port_pair.host(),
                                                 host_port_pair.port());
        }

        const ServerNetworkStats* stats =
            common_connect_job_params->http_server_properties
                ->GetServerNetworkStats(scheme_host_port,
                                        params->network_anonymization_key());
        if (stats && !stats->srtt.is_zero()) {
          return stats->srtt;
        }
      }

      // 2. Fallback to global network RTT from NetworkQualityEstimator.
      if (common_connect_job_params->network_quality_estimator) {
        std::optional<base::TimeDelta> nqe_rtt =
            common_connect_job_params->network_quality_estimator
                ->GetTransportRTT();
        if (nqe_rtt.has_value()) {
          return nqe_rtt;
        }
      }

      return std::nullopt;
    }();

    // 3. Calculate and clamp fallback time.
    if (rtt.has_value()) {
      base::TimeDelta rtt_based_fallback =
          rtt.value() * features::kIPv6FallbackRTTMultiplier.Get();

      rtt_based_fallback =
          std::clamp(rtt_based_fallback, features::kIPv6FallbackMin.Get(),
                     features::kIPv6FallbackMax.Get());
      return rtt_based_fallback;
    }
  }

  return fallback_time;
}

int TcpConnectJob::AdvanceConnectionState(ConnectionState& state,
                                          bool is_stale) {
tcp_connect_jobRustBridge c(this);
  DCHECK_EQ(&state, is_stale ? &stale_state_ : &fresh_state_);
  return c.Run(tcp_connect_jobRustBridge::Op::AdvanceState, is_stale);
}

void TcpConnectJob::MaybePromoteStaleConnectors() {
tcp_connect_jobRustBridge c(this); c.Run(tcp_connect_jobRustBridge::Op::Promote);
}

TcpConnectJob::IPEndPointInfo TcpConnectJob::GetNextIPEndPoint(
    const Connector& connector) {
tcp_connect_jobRustBridge c(this);
  IPEndPointInfo output = base::unexpected(ERR_IO_PENDING);
  c.input_connector = &connector; c.ip_output = &output;
  int rv = c.Run(tcp_connect_jobRustBridge::Op::NextIp);
  if (rv != OK) return base::unexpected(static_cast<Error>(rv));
  return output;
}

bool TcpConnectJob::IsEndpointResultUsable(
    const ServiceEndpoint& service_endpoint) const {
tcp_connect_jobRustBridge c(this);
  c.endpoint = &service_endpoint; return c.Run(tcp_connect_jobRustBridge::Op::Usable);
}

const ServiceEndpoint* TcpConnectJob::FindServiceEndpoint(
    const IPEndPoint& ip_endpoint) const {
tcp_connect_jobRustBridge c(this);
  c.input_ip = &ip_endpoint; c.Run(tcp_connect_jobRustBridge::Op::Find);
  return c.endpoint_output;
}

void TcpConnectJob::UpdateSvcbOptional() {
tcp_connect_jobRustBridge c(this); c.Run(tcp_connect_jobRustBridge::Op::UpdateSvcb);
}

void TcpConnectJob::ResetConnectionState(ConnectionState& state) {
tcp_connect_jobRustBridge c(this);
  c.Run(tcp_connect_jobRustBridge::Op::ResetState, &state == &stale_state_);
}

// static
bool TcpConnectJob::IsStateDone(const ConnectionState& state) {
return connection_jobs_rust_state_done(
      !!state.primary_connector, state.primary_connector && state.primary_connector->is_done(),
      !!state.ipv4_connector, state.ipv4_connector && state.ipv4_connector->is_done());
}

int TcpConnectJob::SetDone(int result, Connector* connector) {
tcp_connect_jobRustBridge c(this);
  c.connector = connector; return c.Run(tcp_connect_jobRustBridge::Op::SetDone, result);
}

base::span<const ServiceEndpoint> TcpConnectJob::GetEndpointResults() const {
tcp_connect_jobRustBridge c(this);
  c.Run(tcp_connect_jobRustBridge::Op::GetResults); return c.results;
}

bool TcpConnectJob::EndpointsCryptoReady() const {
tcp_connect_jobRustBridge c(this); return c.Run(tcp_connect_jobRustBridge::Op::CryptoReady);
}

const base::flat_set<std::string>& TcpConnectJob::GetDnsAliasResults() const {
tcp_connect_jobRustBridge c(this);
  c.Run(tcp_connect_jobRustBridge::Op::GetAliases); CHECK(c.aliases); return *c.aliases;
}

void TcpConnectJob::NotifyDelegateIfDone(int result) {
tcp_connect_jobRustBridge c(this); c.Run(tcp_connect_jobRustBridge::Op::Notify, result);
}

bool TcpConnectJob::IsConnectedViaStaleDns() const {
tcp_connect_jobRustBridge c(this); return c.Run(tcp_connect_jobRustBridge::Op::Stale);
}

}  // namespace net
