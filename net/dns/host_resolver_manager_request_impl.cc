// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/raw_ref.h"
#include "net/dns/host_resolver_manager_request_impl.h"

#include <deque>
#include <optional>
#include <string>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/containers/linked_list.h"
#include "base/feature_list.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/safe_ref.h"
#include "base/memory/weak_ptr.h"
#include "base/metrics/histogram_macros.h"
#include "base/sequence_checker.h"
#include "base/time/tick_clock.h"
#include "net/base/address_list.h"
#include "net/base/completion_once_callback.h"
#include "net/base/features.h"
#include "net/base/network_anonymization_key.h"
#include "net/base/request_priority.h"
#include "net/dns/dns_alias_utility.h"
#include "net/dns/host_cache.h"
#include "net/dns/host_resolver.h"
#include "net/dns/host_resolver_manager.h"
#include "net/dns/host_resolver_manager_job.h"
#include "net/dns/public/host_resolver_results.h"
#include "net/dns/public/resolve_error_info.h"
#include "net/http/http_network_session.h"
#include "net/log/net_log_with_source.h"
#include "net/socket/client_socket_factory.h"
#include "net/url_request/url_request_context.h"

#include "net/dns/dns_request_family_ffi.h"
#include "net/dns/dns_request_partition_provider.h"
namespace net {

class HostResolverManager::RequestImpl::RustFrame {
 public:
  explicit RustFrame(RequestImpl& request) : r(request) {
    static_assert(STATE_IPV6_REACHABILITY==0 && STATE_GET_PARAMETERS==1 &&
                  STATE_GET_PARAMETERS_COMPLETE==2 && STATE_RESOLVE_LOCALLY==3 &&
                  STATE_START_JOB==4 && STATE_FINISH_REQUEST==5 && STATE_NONE==6);
  }
  CompletionOnceCallback incoming_callback;
  raw_ptr<const JobKey> incoming_key = nullptr;
  DnsResolverWords Run(dns_request_family::RequestEvent event, DnsResolverWords in = {}) {
    return rustium_dns_request_family_run(1, static_cast<uint32_t>(event), in, this, &Call);
  }
 private:
  using O = dns_request_family::RequestOp;
  static DnsResolverWords Call(void* context, uint32_t op, DnsResolverWords in) {
    return static_cast<RustFrame*>(context)->Native(static_cast<O>(op), in);
  }
  DnsResolverWords Native(O op, DnsResolverWords in) {
    using dns_request_family::W;
    switch (op) {
      case O::kRead:
        switch (in.x) {
          case 0: return W(r->job_.has_value());
          case 1: return W(!!r->resolve_context_);
          case 2: return W(r->parameters_.source == HostResolverSource::LOCAL_ONLY);
          case 3: return W(r->parameters_.is_speculative);
          case 4: return W(static_cast<int>(r->next_state_));
          case 5: return W(!r->callback_.is_null());
          case 7: return W(r->tasks_.empty());
          case 8: return W(r->parameters_.include_canonical_name);
          default: NOTREACHED();
        }
      case O::kWrite:
        switch (in.x) {
          case 1: r->next_state_ = static_cast<ResolveState>(in.y); break;
          case 2: r->complete_ = in.y; break;
          case 3: r->only_ipv6_reachable_ = in.y; break;
          case 4: r->priority_ = static_cast<RequestPriority>(in.y); break;
          case 5: r->callback_ = std::move(incoming_callback); break;
          case 6: r->error_info_ = ResolveErrorInfo(in.y, !!in.a); break;
          default: NOTREACHED();
        }
        return {};
      case O::kCheck:
        switch (in.x) {
          case 0:
            DCHECK(incoming_callback); CHECK(!r->job_); DCHECK(!r->complete_);
            DCHECK(!r->callback_); DCHECK(r->resolver_); break;
          case 3: DCHECK(!r->complete_); break;
          case 4:
            DCHECK(r->results_); DCHECK(r->legacy_address_results_.empty());
            DCHECK(r->endpoint_results_.empty()); DCHECK(r->fixed_up_dns_alias_results_.empty()); break;
          case 5: DCHECK_LE(r->fixed_up_dns_alias_results_.size(), 1u); break;
          case 6: CHECK(!r->job_); break;
          case 7: DCHECK_EQ(OK, in.y); break;
          case 8: NOTREACHED() << "next_state_: " << r->next_state_;
          case 10: CHECK(r->job_); CHECK(*incoming_key == r->job_.value()->key()); break;
          case 11: DCHECK(!r->complete_); DCHECK(r->callback_); break;
          case 12: DCHECK(!r->results_); break;
          case 13: DCHECK(r->callback_); break;
          default: NOTREACHED();
        }
        return {};
      case O::kLogStart: r->LogStartRequest(); return {};
      case O::kLogFinish: r->LogFinishRequest(in.x, in.y); return {};
      case O::kLogCancel: r->LogCancelRequest(); return {};
      case O::kStartIPv6:
        switch (in.x) {
          case 1: return W(r->resolver_->StartIPv6ReachabilityCheck(r->target_network_, r->source_net_log_, r->GetClientSocketFactory(), base::DoNothingAs<void(int)>()));
          case 0: return W(r->resolver_->StartIPv6ReachabilityCheck(r->target_network_, r->source_net_log_, r->GetClientSocketFactory(), base::BindOnce(&RequestImpl::OnIOComplete, r->weak_ptr_factory_.GetWeakPtr())));
          default: NOTREACHED();
        }
      case O::kInitializeKey:
        r->resolver_->InitializeJobKeyAndIPAddress(r->network_anonymization_key_, r->parameters_, r->source_net_log_, r->job_key_, r->ip_address_); return {};
      case O::kNat64Facts: return W(static_cast<int>(r->parameters_.source), !!(r->job_key_.flags & HOST_RESOLVER_DEFAULT_FAMILY_SET_DUE_TO_NO_IPV6), r->ip_address_.IsIPv4(), r->ip_address_.IsLoopback());
      case O::kLastIPv6: return W(r->resolver_->last_ipv6_probe_result_);
      case O::kStartGlobal:
        return W(r->resolver_->StartGloballyReachableCheck(r->ip_address_, r->target_network_, r->source_net_log_, r->GetClientSocketFactory(), base::BindOnce(&RequestImpl::OnIOComplete, r->weak_ptr_factory_.GetWeakPtr())));
      case O::kResolveLocal:
        local.emplace(r->resolver_->ResolveLocally(r->only_ipv6_reachable_, r->job_key_, r->ip_address_, r->parameters_.cache_usage, r->parameters_.secure_dns_policy, r->parameters_.source, r->source_net_log_, r->host_cache(), &r.get().tasks_, &stale)); return {};
      case O::kLocalError: return W(local->error());
      case O::kHasStale: return W(stale.has_value());
      case O::kCopyLocalResults:
        DCHECK(!r->complete_); DCHECK(!r->results_); DCHECK(!r->resolution_details_); DCHECK(!r->parameters_.is_speculative);
        r->results_ = local->CopyWithDefaultPort(r->request_host_.GetPort());
        r->resolution_details_.emplace();
        // Native enum assignment; Rust selects cache/local using the POD flag.
        r->resolution_details_->source = static_cast<ResolutionSource>(in.x);
        return {};
      case O::kMoveStale: r->set_stale_info(std::move(*stale)); return {};
      case O::kStartJob:
        r->resolver_->CreateAndStartJob(std::move(r->job_key_), std::move(r->tasks_), &r.get()); return {};
      case O::kSquash: return W(HostResolver::SquashErrorCode(in.x));
      case O::kRunCallback: std::move(r->callback_).Run(in.x); return {};
      case O::kCancelJob: r->job_.value()->CancelRequest(&r.get()); return {};
      case O::kChangeJobPriority: r->job_.value()->ChangeRequestPriority(&r.get(), static_cast<RequestPriority>(in.x)); return {};
      case O::kResetCallback: r->callback_.Reset(); return {};
      case O::kResetJob: r->job_.reset(); return {};
      case O::kResetResolver: r->resolver_.reset(); return {};
      case O::kCopyEndpoints: r->endpoint_results_ = r->results_->GetEndpoints(); return {};
      case O::kCopyAliases: r->fixed_up_dns_alias_results_ = r->results_->aliases(); return {};
      case O::kFixAliases: r->fixed_up_dns_alias_results_ = dns_alias_utility::FixUpDnsAliases(r->fixed_up_dns_alias_results_); return {};
      case O::kCopyLegacy: r->legacy_address_results_ = HostResolver::EndpointResultToAddressList(r->endpoint_results_, r->fixed_up_dns_alias_results_); return {};
      case O::kReleaseLocal: local.reset(); stale.reset(); return {};
    }
    NOTREACHED();
  }
  raw_ref<RequestImpl> r;
  std::optional<HostCache::Entry> local;
  std::optional<HostCache::EntryStaleness> stale;
};

HostResolverManager::RequestImpl::RequestImpl(
    NetLogWithSource source_net_log,
    HostResolver::Host request_host,
    NetworkAnonymizationKey network_anonymization_key,
    handles::NetworkHandle target_network,
    std::optional<ResolveHostParameters> optional_parameters,
    base::WeakPtr<ResolveContext> resolve_context,
    base::WeakPtr<HostResolverManager> resolver,
    const base::TickClock* tick_clock)
    : source_net_log_(std::move(source_net_log)),
      request_host_(std::move(request_host)),
      network_anonymization_key_(
          DnsRequestRetainPartitionKey(std::move(network_anonymization_key))),
      target_network_(target_network),
      parameters_(optional_parameters ? std::move(optional_parameters).value()
                                      : ResolveHostParameters()),
      resolve_context_(std::move(resolve_context)),
      priority_(parameters_.initial_priority),
      job_key_(request_host_, target_network_, resolve_context_.get()),
      resolver_(std::move(resolver)),
      tick_clock_(tick_clock) {
  CHECK_NE(parameters_.cache_usage,
           ResolveHostParameters::CacheUsage::STALE_ALLOWED_WHILE_REFRESHING);
}

HostResolverManager::RequestImpl::~RequestImpl() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  RustFrame frame(*this);
  frame.Run(dns_request_family::RequestEvent::kDestroy);
}

int HostResolverManager::RequestImpl::Start(CompletionOnceCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  RustFrame frame(*this); frame.incoming_callback = std::move(callback);
  return frame.Run(dns_request_family::RequestEvent::kStart).x;
}

const AddressList& HostResolverManager::RequestImpl::GetAddressResults() const {
  DCHECK(complete_);
  return legacy_address_results_;
}

base::span<const HostResolverEndpointResult>
HostResolverManager::RequestImpl::GetEndpointResults() const {
  DCHECK(complete_);
  return endpoint_results_;
}

base::span<const std::string> HostResolverManager::RequestImpl::GetTextResults()
    const {
  DCHECK(complete_);
  return results_ ? results_->text_records() : base::span<const std::string>();
}

base::span<const HostPortPair>
HostResolverManager::RequestImpl::GetHostnameResults() const {
  DCHECK(complete_);
  return results_ ? results_->hostnames() : base::span<const HostPortPair>();
}

const base::flat_set<std::string>&
HostResolverManager::RequestImpl::GetDnsAliasResults() const {
  DCHECK(complete_);

  // If `include_canonical_name` param was true, should only ever have at most
  // a single alias, representing the expected "canonical name".
#if DCHECK_IS_ON()
  if (parameters().include_canonical_name) {
    DCHECK_LE(fixed_up_dns_alias_results_.size(), 1u);
    base::flat_set<std::string> address_list_aliases_set(
        GetAddressResults().dns_aliases());
    DCHECK(address_list_aliases_set == fixed_up_dns_alias_results_);
  }
#endif  // DCHECK_IS_ON()

  return fixed_up_dns_alias_results_;
}

net::ResolveErrorInfo HostResolverManager::RequestImpl::GetResolveErrorInfo()
    const {
  DCHECK(complete_);
  return error_info_;
}

const std::optional<HostCache::EntryStaleness>&
HostResolverManager::RequestImpl::GetStaleInfo() const {
  DCHECK(complete_);
  return stale_info_;
}

void HostResolverManager::RequestImpl::ChangeRequestPriority(
    RequestPriority priority) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  RustFrame frame(*this);
  frame.Run(dns_request_family::RequestEvent::kChangePriority, dns_request_family::W(priority));
}

std::optional<ResolutionDetails>
HostResolverManager::RequestImpl::GetResolutionDetails() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return resolution_details_;
}

void HostResolverManager::RequestImpl::SetResults(
    HostCache::Entry results,
    ResolutionDetails resolution_details) {
  // Should only be called at most once and before request is marked
  // completed.
  DCHECK(!complete_);
  DCHECK(!results_);
  DCHECK(!resolution_details_);
  DCHECK(!parameters_.is_speculative);

  results_ = std::move(results);
  resolution_details_ = resolution_details;
  FixUpEndpointAndAliasResults();
}

void HostResolverManager::RequestImpl::set_error_info(
    int error,
    bool is_secure_network_error) {
  error_info_ = ResolveErrorInfo(error, is_secure_network_error);
}

void HostResolverManager::RequestImpl::set_stale_info(
    HostCache::EntryStaleness stale_info) {
  // Should only be called at most once and before request is marked
  // completed.
  DCHECK(!complete_);
  DCHECK(!stale_info_);
  DCHECK(!parameters_.is_speculative);

  stale_info_ = std::move(stale_info);
}

void HostResolverManager::RequestImpl::AssignJob(base::SafeRef<Job> job) {
  CHECK(!job_.has_value());
  job_ = std::move(job);
}

const HostResolverManager::JobKey& HostResolverManager::RequestImpl::GetJobKey()
    const {
  CHECK(job_.has_value());
  return job_.value()->key();
}

void HostResolverManager::RequestImpl::OnJobCancelled(const JobKey& job_key) {
  RustFrame frame(*this); frame.incoming_key = &job_key;
  frame.Run(dns_request_family::RequestEvent::kCancelled);
}

void HostResolverManager::RequestImpl::OnJobCompleted(
    const JobKey& job_key,
    int error,
    bool is_secure_network_error) {
  RustFrame frame(*this); frame.incoming_key = &job_key;
  frame.Run(dns_request_family::RequestEvent::kCompleted, dns_request_family::W(error, 0, 0, is_secure_network_error));
}

int HostResolverManager::RequestImpl::DoLoop(int rv) {
  RustFrame frame(*this);
  return frame.Run(dns_request_family::RequestEvent::kLoop, dns_request_family::W(rv)).x;
}

void HostResolverManager::RequestImpl::OnIOComplete(int rv) {
  RustFrame frame(*this);
  frame.Run(dns_request_family::RequestEvent::kIOComplete, dns_request_family::W(rv));
}

int HostResolverManager::RequestImpl::DoIPv6Reachability() {
  RustFrame frame(*this);
  return frame.Run(dns_request_family::RequestEvent::kIPv6).x;
}

int HostResolverManager::RequestImpl::DoGetParameters() {
  RustFrame frame(*this);
  return frame.Run(dns_request_family::RequestEvent::kParameters).x;
}

int HostResolverManager::RequestImpl::DoGetParametersComplete(int rv) {
  RustFrame frame(*this);
  return frame.Run(dns_request_family::RequestEvent::kParametersComplete, dns_request_family::W(rv)).x;
}

int HostResolverManager::RequestImpl::DoResolveLocally() {
  RustFrame frame(*this);
  return frame.Run(dns_request_family::RequestEvent::kResolveLocal).x;
}

int HostResolverManager::RequestImpl::DoStartJob() {
  RustFrame frame(*this);
  return frame.Run(dns_request_family::RequestEvent::kStartJob).x;
}

int HostResolverManager::RequestImpl::DoFinishRequest(int rv) {
  RustFrame frame(*this);
  return frame.Run(dns_request_family::RequestEvent::kFinish, dns_request_family::W(rv)).x;
}

void HostResolverManager::RequestImpl::FixUpEndpointAndAliasResults() {
  RustFrame frame(*this);
  frame.Run(dns_request_family::RequestEvent::kFixResults);
}

void HostResolverManager::RequestImpl::LogStartRequest() {
  DCHECK(request_time_.is_null());
  request_time_ = tick_clock_->NowTicks();

  source_net_log_.BeginEvent(
      NetLogEventType::HOST_RESOLVER_MANAGER_REQUEST, [this] {
        base::DictValue dict;
        dict.Set("host", request_host_.ToString());
        dict.Set("dns_query_type",
                 kDnsQueryTypes.at(parameters_.dns_query_type));
        dict.Set("allow_cached_response",
                 parameters_.cache_usage !=
                     ResolveHostParameters::CacheUsage::DISALLOWED);
        dict.Set("is_speculative", parameters_.is_speculative);
        dict.Set("network_anonymization_key",
                 network_anonymization_key_.ToDebugString());
        dict.Set("secure_dns_policy",
                 base::strict_cast<int>(parameters_.secure_dns_policy));
        return dict;
      });
}

void HostResolverManager::RequestImpl::LogFinishRequest(int net_error,
                                                        bool async_completion) {
  source_net_log_.EndEventWithNetErrorCode(
      NetLogEventType::HOST_RESOLVER_MANAGER_REQUEST, net_error);

  if (!parameters_.is_speculative) {
    DCHECK(!request_time_.is_null());
    base::TimeDelta duration = tick_clock_->NowTicks() - request_time_;

    DEPRECATED_UMA_HISTOGRAM_MEDIUM_TIMES("Net.DNS.Request.TotalTime",
                                          duration);
    if (async_completion) {
      DEPRECATED_UMA_HISTOGRAM_MEDIUM_TIMES("Net.DNS.Request.TotalTimeAsync",
                                            duration);
    }
  }
}

void HostResolverManager::RequestImpl::LogCancelRequest() {
  source_net_log_.AddEvent(NetLogEventType::CANCELLED);
  source_net_log_.EndEvent(NetLogEventType::HOST_RESOLVER_MANAGER_REQUEST);
}

ClientSocketFactory*
HostResolverManager::RequestImpl::GetClientSocketFactory() {
  if (resolve_context_->url_request_context()) {
    return resolve_context_->url_request_context()
        ->GetNetworkSessionContext()
        ->client_socket_factory;
  } else {
    return ClientSocketFactory::GetDefaultFactory();
  }
}

}  // namespace net
