// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/raw_span.h"
#include "net/socket/transport_connect_job.h"
#include "net/socket/transport_connect_job_rust_ffi.h"
#include "net/socket/connection_jobs_upper_rust_ffi.h"
#include "net/socket/connect_job_rust_native.h"
#include "net/socket/tcp_connect_fallback_rust_native.h"

#include <memory>
#include <set>
#include <utility>
#include <variant>

#include "base/check_op.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/notreached.h"
#include "base/task/single_thread_task_runner.h"
#include "base/time/time.h"
#include "base/trace_event/trace_event.h"
#include "net/base/ech_mode.h"
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
#include "net/socket/tcp_connect_job.h"
#include "net/socket/transport_connect_sub_job.h"
#include "net/ssl/ssl_config_service.h"
#include "url/scheme_host_port.h"
#include "url/url_constants.h"

namespace net {

namespace {

// TODO(crbug.com/40181080): Delete once endpoint usage is converted to using
// url::SchemeHostPort when available.
HostPortPair ToLegacyDestinationEndpoint(
    const TransportSocketParams::Endpoint& endpoint) {
  if (std::holds_alternative<url::SchemeHostPort>(endpoint)) {
    return HostPortPair::FromSchemeHostPort(
        std::get<url::SchemeHostPort>(endpoint));
  }

  DCHECK(std::holds_alternative<HostPortPair>(endpoint));
  return std::get<HostPortPair>(endpoint);
}

}  // namespace


// Invocation-local observer. Invalidated at the most-derived destructor entry.
// No typed native pointer crosses the Rust ABI or survives a service call.
struct transport_connect_jobRustBridge : connection_jobs_upper_rust::Invocation {

  static_assert(TransportConnectJob::STATE_RESOLVE_HOST==0);
  static_assert(TransportConnectJob::STATE_RESOLVE_HOST_COMPLETE==1);
  static_assert(TransportConnectJob::STATE_RESOLVE_HOST_CALLBACK_COMPLETE==2);
  static_assert(TransportConnectJob::STATE_TRANSPORT_CONNECT==3);
  static_assert(TransportConnectJob::STATE_TRANSPORT_CONNECT_COMPLETE==4);
  static_assert(TransportConnectJob::STATE_NONE==5);
  static_assert(TransportConnectJob::SUB_JOB_IPV4==0 && TransportConnectJob::SUB_JOB_IPV6==1);
  static_assert(ADDRESS_FAMILY_IPV4==1 && ADDRESS_FAMILY_IPV6==2);
  static_assert(LOAD_STATE_IDLE==0 && LOAD_STATE_RESOLVING_HOST==10 && LOAD_STATE_CONNECTING==11);

  using Op = transport_connect_job_rust::Operation;
  using Action = transport_connect_job_rust::Action;
  raw_ptr<const TransportConnectJob> owner;
  raw_ptr<TransportConnectJob> mutable_owner = nullptr;

  raw_ptr<std::optional<TransportConnectJob::EndpointResultOverride>> override_input = nullptr;
  raw_ptr<const HostResolverEndpointResult> endpoint = nullptr;
  raw_ptr<TransportConnectSubJob> subjob = nullptr;
  base::raw_span<const HostResolverEndpointResult> results;
  size_t result_index = 0;
  std::set<IPEndPoint> seen;
  std::vector<IPEndPoint> filtered, v4, v6;
  std::optional<ConnectJobRustBridge> base;
  std::unique_ptr<StreamSocket> socket_payload;
  std::optional<base::flat_set<std::string>> aliases_payload;
  base::TimeDelta delay;
  TcpFallbackRustBridge fallback;

  explicit transport_connect_jobRustBridge(TransportConnectJob* value) : Invocation(static_cast<const ConnectJob*>(value)), owner(value), mutable_owner(value) {
    Register(&Clear); base.emplace(value);
  }
  explicit transport_connect_jobRustBridge(const TransportConnectJob* value) : Invocation(static_cast<const ConnectJob*>(value)), owner(value) {
    Register(&Clear); base.emplace(value);
  }
  ~transport_connect_jobRustBridge() { base.reset(); Unregister(); }
  static void Clear(Invocation* value) {
    auto& c = *static_cast<transport_connect_jobRustBridge*>(value);
    c.owner = nullptr; c.mutable_owner = nullptr;
    c.endpoint = nullptr; c.subjob = nullptr; c.results = {}; c.override_input = nullptr;
  }
  int64_t Run(Op operation, int64_t argument = 0) {
    return connection_jobs_rust_run(1, static_cast<uint32_t>(operation),
                                    argument, this, &Service);
  }
  static bool Service(void* context, uint32_t module, uint32_t action,
                      int64_t argument, int64_t* output) {
    auto& c = *static_cast<transport_connect_jobRustBridge*>(context);
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
    CHECK_EQ(module, 1u);
    // Mutable access is requested only in action arms that require it. Const
    // wrappers never obtain a mutable receiver and never commit a snapshot.
    auto Mutable = [&]() -> TransportConnectJob& { CHECK(c.mutable_owner); return *c.mutable_owner; };
    switch (static_cast<Action>(action)) {
      case Action::State: { *output = j.next_state_; break; }
      case Action::SetState: { Mutable().next_state_ = static_cast<TransportConnectJob::State>(argument); break; }
      case Action::CheckLoop: { DCHECK_NE(j.next_state_, TransportConnectJob::STATE_NONE); break; }
      case Action::CheckOk: { DCHECK_EQ(OK, argument); break; }
      case Action::CheckNotPending: { DCHECK_NE(argument, ERR_IO_PENDING); break; }
      case Action::NotReached: { NOTREACHED(); }
      case Action::DnsStart: { auto now = base::TimeTicks::Now(); if (c.owner) Mutable().connect_timing_.domain_lookup_start = now; break; }
      case Action::HasOverride: { *output = j.has_dns_override_; break; }
      case Action::OverrideEnd: { DCHECK_EQ(j.endpoint_results_.size(), 1u); Mutable().connect_timing_.domain_lookup_end = j.connect_timing_.domain_lookup_start; break; }
      case Action::SchemeDestination: { *output = std::holds_alternative<url::SchemeHostPort>(j.params_->destination()); break; }
      case Action::MakeSchemeRequest: { HostResolver::ResolveHostParameters p;
 p.initial_priority = j.priority(); p.secure_dns_policy = j.params_->secure_dns_policy();
 auto request = j.common_connect_job_params()->host_resolver->CreateRequest(
 std::get<url::SchemeHostPort>(j.params_->destination()),
 j.params_->network_anonymization_key(), j.params_->target_network(), j.net_log(), p);
 if (c.owner) Mutable().request_ = std::move(request); break; }
      case Action::MakePairRequest: { HostResolver::ResolveHostParameters p;
 p.initial_priority = j.priority(); p.secure_dns_policy = j.params_->secure_dns_policy();
 auto request = j.common_connect_job_params()->host_resolver->CreateRequest(
 std::get<HostPortPair>(j.params_->destination()),
 j.params_->network_anonymization_key(), j.params_->target_network(), j.net_log(), p);
 if (c.owner) Mutable().request_ = std::move(request); break; }
      case Action::StartRequest: { *output = Mutable().request_->Start(base::BindOnce(&TransportConnectJob::OnIOComplete, base::Unretained(c.mutable_owner.get()))); break; }
      case Action::ResolveTrace: { TRACE_EVENT0(NetTracingCategory(), "TransportConnectJob::DoResolveHostComplete");
 *output = connection_jobs_rust_run(1, static_cast<uint32_t>(Op::ResolveComplete), argument, &c, &Service); break; }
      case Action::DnsEnd: { auto now = base::TimeTicks::Now(); if (c.owner) { Mutable().connect_timing_.domain_lookup_end = now; Mutable().connect_timing_.connect_start = now; } break; }
      case Action::StoreResolveError: { auto info = j.request_->GetResolveErrorInfo(); if(c.owner) Mutable().resolve_error_info_ = info; break; }
      case Action::FailedDnsAttempt: { Mutable().connection_attempts_.emplace_back(IPEndPoint(), static_cast<int>(argument)); break; }
      case Action::HasCallback: { *output = !j.params_->host_resolution_callback().is_null(); break; }
      case Action::HostCallback: { *output = j.params_->host_resolution_callback().Run(
 ToLegacyDestinationEndpoint(j.params_->destination()),
 j.request_->GetEndpointResults(), j.request_->GetDnsAliasResults()) ==
 OnHostResolutionCallbackResult::kMayBeDeletedAsync; break; }
      case Action::PostResolution: { base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
 FROM_HERE, base::BindOnce(&TransportConnectJob::OnIOComplete,
 Mutable().weak_ptr_factory_.GetWeakPtr(), OK)); break; }
      case Action::SetResultsInput: { c.results = j.request_->GetEndpointResults(); break; }
      case Action::ResultsCount: { *output = c.results.size(); break; }
      case Action::SelectResult: { c.result_index = static_cast<size_t>(argument); c.endpoint = &c.results[c.result_index]; break; }
      case Action::ResultIpCount: { *output = c.endpoint->ip_endpoints.size(); break; }
      case Action::InsertSeenIp: { *output = c.seen.insert(c.endpoint->ip_endpoints[static_cast<size_t>(argument)]).second; break; }
      case Action::AddFilteredIp: { c.filtered.push_back(c.endpoint->ip_endpoints[static_cast<size_t>(argument)]); break; }
      case Action::BeginFiltered: { c.filtered.clear(); break; }
      case Action::AppendFiltered: { HostResolverEndpointResult out;
 out.ip_endpoints = std::move(c.filtered); out.metadata = c.endpoint->metadata;
 Mutable().endpoint_results_.push_back(std::move(out)); break; }
      case Action::CopyAliases: { Mutable().dns_aliases_ = j.request_->GetDnsAliasResults(); break; }
      case Action::CopyResolution: { Mutable().resolution_details_ = j.request_->GetResolutionDetails(); break; }
      case Action::ResetRequest: { c.endpoint = nullptr; c.results = {}; Mutable().request_.reset(); break; }
      case Action::EndpointCount: { *output = j.endpoint_results_.size(); break; }
      case Action::SelectCurrentEndpoint: { CHECK_LT(j.current_endpoint_result_, j.endpoint_results_.size());
 c.endpoint = &j.endpoint_results_[j.current_endpoint_result_]; break; }
      case Action::IpFamily: { *output = c.endpoint->ip_endpoints[static_cast<size_t>(argument)].GetFamily(); break; }
      case Action::AppendV4: { c.v4.push_back(c.endpoint->ip_endpoints[static_cast<size_t>(argument)]); break; }
      case Action::AppendV6: { c.v6.push_back(c.endpoint->ip_endpoints[static_cast<size_t>(argument)]); break; }
      case Action::UnexpectedFamily: { DVLOG(1) << "Unexpected ADDRESS_FAMILY: " << argument; break; }
      case Action::V4Count: { *output = c.v4.size(); break; }
      case Action::V6Count: { *output = c.v6.size(); break; }
      case Action::MakeV4: { Mutable().ipv4_job_ = std::make_unique<TransportConnectSubJob>(std::move(c.v4), c.mutable_owner.get(), TransportConnectJob::SUB_JOB_IPV4); break; }
      case Action::MakeV6: { Mutable().ipv6_job_ = std::make_unique<TransportConnectSubJob>(std::move(c.v6), c.mutable_owner.get(), TransportConnectJob::SUB_JOB_IPV6); break; }
      case Action::HasV4: { *output = !!j.ipv4_job_; break; }
      case Action::HasV6: { *output = !!j.ipv6_job_; break; }
      case Action::V4Started: { *output = j.ipv4_job_->started(); break; }
      case Action::V6Started: { *output = j.ipv6_job_->started(); break; }
      case Action::V4Load: { *output = j.ipv4_job_->GetLoadState(); break; }
      case Action::V6Load: { *output = j.ipv6_job_->GetLoadState(); break; }
      case Action::SelectV4: { c.subjob = j.ipv4_job_.get(); break; }
      case Action::SelectV6: { c.subjob = j.ipv6_job_.get(); break; }
      case Action::StartSelected: { *output = c.subjob->Start(); break; }
      case Action::SubType: { *output = c.subjob->type(); break; }
      case Action::AdoptSocket: { c.socket_payload = c.subjob->PassSocket();
 if (!c.owner) return false;
 c.aliases_payload = j.dns_aliases_;
 c.base->socket_input = &c.socket_payload; c.base->aliases = &c.aliases_payload; break; }
      case Action::ResetV4: { c.subjob = nullptr; Mutable().ipv4_job_.reset(); break; }
      case Action::ResetV6: { c.subjob = nullptr; Mutable().ipv6_job_.reset(); break; }
      case Action::StopFallback: { Mutable().fallback_timer_.Stop(); break; }
      case Action::FallbackTime: { c.delay = c.fallback.delay; break; }
      case Action::StartFallback: { Mutable().fallback_timer_.Start(FROM_HERE, c.delay,
 base::BindOnce(&TransportConnectJob::StartIPv4JobAsync, base::Unretained(c.mutable_owner.get()))); break; }
      case Action::Latency: { DCHECK(!j.connect_timing_.connect_start.is_null());
 DCHECK(!j.connect_timing_.domain_lookup_start.is_null()); DCHECK(j.socket_);
 const auto now = base::TimeTicks::Now(); if (!c.owner) return false;
 UMA_HISTOGRAM_CUSTOM_TIMES("Net.DNS_Resolution_And_TCP_Connection_Latency2",
 now - j.connect_timing_.domain_lookup_start, base::Milliseconds(1), base::Minutes(10), 100);
 if (!c.owner) return false;
 UMA_HISTOGRAM_CUSTOM_TIMES("Net.TCP_Connection_Latency",
 now - j.connect_timing_.connect_start, base::Milliseconds(1), base::Minutes(10), 100); break; }
      case Action::NextEndpoint: { ++Mutable().current_endpoint_result_; *output = j.current_endpoint_result_; break; }
      case Action::Notify: { NOTREACHED(); }
      case Action::FallbackEvent: { j.net_log().AddEvent(NetLogEventType::TRANSPORT_CONNECT_JOB_IPV6_FALLBACK); break; }
      case Action::CheckV4: { DCHECK(j.ipv4_job_); break; }
      case Action::CheckNoV6: { DCHECK(!j.ipv6_job_); break; }
      case Action::Priority: { DCHECK(j.request_); Mutable().request_->ChangeRequestPriority(static_cast<RequestPriority>(argument)); break; }
      case Action::Https: { const auto* p = std::get_if<url::SchemeHostPort>(&j.params_->destination());
 *output = p && p->scheme() == url::kHttpsScheme; break; }
      case Action::EchEnabled: { const auto* p = std::get_if<url::SchemeHostPort>(&j.params_->destination());
 auto* ssl = j.common_connect_job_params()->ssl_client_context.get();
 *output = ssl->IsEchEnabled(p->host()); break; }
      case Action::ResultAlternative: { *output = !c.endpoint->metadata.supported_protocol_alpns.empty(); break; }
      case Action::ResultEch: { *output = !c.endpoint->metadata.ech_config_list.empty(); break; }
      case Action::AlpnCount: { *output = c.endpoint->metadata.supported_protocol_alpns.size(); break; }
      case Action::AlpnSupported: { *output = j.params_->supported_alpns().contains(c.endpoint->metadata.supported_protocol_alpns[static_cast<size_t>(argument)]); break; }
      case Action::HasInitOverride: { *output = c.override_input && c.override_input->has_value(); break; }
      case Action::MoveInitOverride: { Mutable().has_dns_override_ = true;
 Mutable().endpoint_results_ = {std::move(c.override_input->value().result)};
 Mutable().dns_aliases_ = std::move(c.override_input->value().dns_aliases);
 c.results = j.endpoint_results_; c.endpoint = &c.results.front(); break; }
      case Action::CheckOverrideIps: { DCHECK(!c.endpoint->ip_endpoints.empty()); break; }
      case Action::CheckUsable: { DCHECK(argument); break; }
      case Action::CheckLegacy: { DCHECK(!base::FeatureList::IsEnabled(features::kHappyEyeballsV2)); break; }

      case Action::HasSsl: { *output = j.common_connect_job_params()->ssl_client_context != nullptr; break; }
      default: NOTREACHED();
    }
    return c.owner != nullptr;
  }
};
TransportSocketParams::TransportSocketParams(
    Endpoint destination,
    NetworkAnonymizationKey network_anonymization_key,
    SecureDnsPolicy secure_dns_policy,
    handles::NetworkHandle target_network,
    OnHostResolutionCallback host_resolution_callback,
    base::flat_set<std::string> supported_alpns)
    : destination_(std::move(destination)),
      network_anonymization_key_(std::move(network_anonymization_key)),
      secure_dns_policy_(secure_dns_policy),
      target_network_(target_network),
      host_resolution_callback_(std::move(host_resolution_callback)),
      supported_alpns_(std::move(supported_alpns)) {
#if DCHECK_IS_ON()
  auto* scheme_host_port = std::get_if<url::SchemeHostPort>(&destination_);
  if (scheme_host_port) {
    if (scheme_host_port->scheme() == url::kHttpsScheme) {
      // HTTPS destinations will, when passed to the DNS resolver, return
      // SVCB/HTTPS-based routes. Those routes require ALPN protocols to
      // evaluate. If there are none, `IsEndpointResultUsable` will correctly
      // skip each route, but it doesn't make sense to make a DNS query if we
      // can't handle the result.
      DCHECK(!supported_alpns_.empty());
    } else if (scheme_host_port->scheme() == url::kHttpScheme) {
      // HTTP (not HTTPS) does not currently define ALPN protocols, so the list
      // should be empty. This means `IsEndpointResultUsable` will skip any
      // SVCB-based routes. HTTP also has no SVCB mapping, so `HostResolver`
      // will never return them anyway.
      //
      // `HostResolver` will still query SVCB (rather, HTTPS) records for the
      // corresponding HTTPS URL to implement an upgrade flow (section 9.5 of
      // draft-ietf-dnsop-svcb-https-08), but this will result in DNS resolution
      // failing with `ERR_DNS_NAME_HTTPS_ONLY`, not SVCB-based routes.
      DCHECK(supported_alpns_.empty());
    }
  }
#endif
}

TransportSocketParams::~TransportSocketParams() = default;

std::unique_ptr<ConnectJob> TransportConnectJob::Factory::Create(
    RequestPriority priority,
    const SocketTag& socket_tag,
    const CommonConnectJobParams* common_connect_job_params,
    const scoped_refptr<TransportSocketParams>& params,
    Delegate* delegate,
    const NetLogWithSource* net_log) {
  return CreateJob(priority, socket_tag, common_connect_job_params, params,
                   delegate, net_log);
}

std::unique_ptr<ConnectJob> TransportConnectJob::Factory::CreateJob(
    RequestPriority priority,
    const SocketTag& socket_tag,
    const CommonConnectJobParams* common_connect_job_params,
    const scoped_refptr<TransportSocketParams>& params,
    Delegate* delegate,
    const NetLogWithSource* net_log) {
  if (connection_jobs_rust_run(1, static_cast<uint32_t>(transport_connect_job_rust::Operation::Factory),
      base::FeatureList::IsEnabled(features::kHappyEyeballsV2), nullptr, nullptr)) {
    return std::make_unique<TcpConnectJob>(
        priority, socket_tag, common_connect_job_params, params, delegate,
        net_log, /*endpoint_result_override=*/std::nullopt,
        /*disable_stale_dns=*/true);
  }
  return std::make_unique<TransportConnectJob>(priority, socket_tag,
                                               common_connect_job_params,
                                               params, delegate, net_log);
}

TransportConnectJob::EndpointResultOverride::EndpointResultOverride(
    HostResolverEndpointResult result,
    base::flat_set<std::string> dns_aliases)
    : result(std::move(result)), dns_aliases(std::move(dns_aliases)) {}
TransportConnectJob::EndpointResultOverride::EndpointResultOverride(
    EndpointResultOverride&&) = default;
TransportConnectJob::EndpointResultOverride::EndpointResultOverride(
    const EndpointResultOverride&) = default;
TransportConnectJob::EndpointResultOverride::~EndpointResultOverride() =
    default;

TransportConnectJob::TransportConnectJob(
    RequestPriority priority,
    const SocketTag& socket_tag,
    const CommonConnectJobParams* common_connect_job_params,
    const scoped_refptr<TransportSocketParams>& params,
    Delegate* delegate,
    const NetLogWithSource* net_log,
    std::optional<EndpointResultOverride> endpoint_result_override)
    : ConnectJob(priority,
                 socket_tag,
                 ConnectionTimeout(),
                 common_connect_job_params,
                 delegate,
                 net_log,
                 NetLogSourceType::TRANSPORT_CONNECT_JOB,
                 NetLogEventType::TRANSPORT_CONNECT_JOB_CONNECT),
      params_(params) {
  transport_connect_jobRustBridge c(this);
  c.override_input = &endpoint_result_override;
  c.Run(transport_connect_jobRustBridge::Op::Init);
}

// We don't worry about cancelling the host resolution and TCP connect, since
// ~HostResolver::Request and ~TransportConnectSubJob will take care of it.
TransportConnectJob::~TransportConnectJob() {
  transport_connect_jobRustBridge::Invalidate(static_cast<const ConnectJob*>(this));
}

LoadState TransportConnectJob::GetLoadState() const {
transport_connect_jobRustBridge c(this); return static_cast<LoadState>(c.Run(transport_connect_jobRustBridge::Op::LoadState));
}

bool TransportConnectJob::HasEstablishedConnection() const {
  // No need to ever return true, since NotifyComplete() is called as soon as a
  // connection is established.
  return false;
}

ConnectionAttempts TransportConnectJob::GetConnectionAttempts() const {
  return connection_attempts_;
}

ResolveErrorInfo TransportConnectJob::GetResolveErrorInfo() const {
  return resolve_error_info_;
}

std::optional<HostResolverEndpointResult>
TransportConnectJob::GetHostResolverEndpointResult() const {
  CHECK_LT(current_endpoint_result_, endpoint_results_.size());
  return endpoint_results_[current_endpoint_result_];
}

std::optional<ResolutionDetails> TransportConnectJob::GetResolutionDetails()
    const {
  return resolution_details_;
}

base::TimeDelta TransportConnectJob::ConnectionTimeout() {
return base::Microseconds(connection_jobs_rust_run(1, static_cast<uint32_t>(transport_connect_job_rust::Operation::Timeout), 0, nullptr, nullptr));
}

void TransportConnectJob::OnIOComplete(int result) {
transport_connect_jobRustBridge c(this); c.Run(transport_connect_jobRustBridge::Op::OnIO, result);
}

int TransportConnectJob::DoLoop(int result) {
transport_connect_jobRustBridge c(this); return c.Run(transport_connect_jobRustBridge::Op::Loop, result);
}

int TransportConnectJob::DoResolveHost() {
transport_connect_jobRustBridge c(this); return c.Run(transport_connect_jobRustBridge::Op::Resolve);
}

int TransportConnectJob::DoResolveHostComplete(int result) {
transport_connect_jobRustBridge c(this); return c.Run(transport_connect_jobRustBridge::Op::ResolveComplete, result);
}

int TransportConnectJob::DoResolveHostCallbackComplete() {
transport_connect_jobRustBridge c(this); return c.Run(transport_connect_jobRustBridge::Op::Resolved);
}

int TransportConnectJob::DoTransportConnect() {
transport_connect_jobRustBridge c(this); return c.Run(transport_connect_jobRustBridge::Op::Transport);
}

int TransportConnectJob::DoTransportConnectComplete(int result) {
transport_connect_jobRustBridge c(this); return c.Run(transport_connect_jobRustBridge::Op::TransportComplete, result);
}

int TransportConnectJob::HandleSubJobComplete(int result,
                                              TransportConnectSubJob* job) {
transport_connect_jobRustBridge c(this); c.subjob = job; return c.Run(transport_connect_jobRustBridge::Op::Handle, result);
}

void TransportConnectJob::OnSubJobComplete(int result,
                                           TransportConnectSubJob* job) {
transport_connect_jobRustBridge c(this); c.subjob = job; c.Run(transport_connect_jobRustBridge::Op::OnSub, result);
}

void TransportConnectJob::StartIPv4JobAsync() {
transport_connect_jobRustBridge c(this); c.Run(transport_connect_jobRustBridge::Op::StartV4);
}

int TransportConnectJob::ConnectInternal() {
transport_connect_jobRustBridge c(this); return c.Run(transport_connect_jobRustBridge::Op::Connect);
}

void TransportConnectJob::ChangePriorityInternal(RequestPriority priority) {
transport_connect_jobRustBridge c(this); c.Run(transport_connect_jobRustBridge::Op::Priority, priority);
}

bool TransportConnectJob::IsSvcbOptional(
    base::span<const HostResolverEndpointResult> results) const {
transport_connect_jobRustBridge c(this); c.results = results; return c.Run(transport_connect_jobRustBridge::Op::Svcb);
}

bool TransportConnectJob::IsEndpointResultUsable(
    const HostResolverEndpointResult& result,
    bool svcb_optional) const {
transport_connect_jobRustBridge c(this); c.endpoint = &result; return c.Run(transport_connect_jobRustBridge::Op::Usable, svcb_optional);
}

const HostResolverEndpointResult&
TransportConnectJob::GetEndpointResultForCurrentSubJobs() const {
  CHECK_LT(current_endpoint_result_, endpoint_results_.size());
  return endpoint_results_[current_endpoint_result_];
}

}  // namespace net
