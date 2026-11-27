// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/raw_ref.h"
#include "net/dns/host_resolver_manager_service_endpoint_request_impl.h"

#include <sstream>

#include "base/containers/flat_set.h"
#include "base/containers/to_vector.h"
#include "base/feature_list.h"
#include "base/memory/safe_ref.h"
#include "base/notreached.h"
#include "base/types/optional_util.h"
#include "net/base/features.h"
#include "net/base/net_errors.h"
#include "net/base/network_handle.h"
#include "net/dns/dns_alias_utility.h"
#include "net/dns/dns_task_results_manager.h"
#include "net/dns/host_cache.h"
#include "net/dns/host_resolver.h"
#include "net/dns/host_resolver_manager.h"
#include "net/dns/public/resolve_error_info.h"
#include "net/http/http_network_session.h"
#include "net/socket/client_socket_factory.h"
#include "net/url_request/url_request_context.h"
#include "url/scheme_host_port.h"

#include <array>
#include "net/dns/dns_request_family_ffi.h"
#include "net/dns/dns_request_partition_provider.h"
namespace net {

class HostResolverManager::ServiceEndpointRequestImpl::RustFrame {
 public:
  explicit RustFrame(const ServiceEndpointRequestImpl& request) : c(request) {
    static_assert(static_cast<int>(State::kNone)==0 && static_cast<int>(State::kCheckIPv6Reachability)==1 &&
                  static_cast<int>(State::kCheckIPv6ReachabilityComplete)==2 &&
                  static_cast<int>(State::kDoResolveLocally)==3 && static_cast<int>(State::kStartJob)==4);
  }
  explicit RustFrame(ServiceEndpointRequestImpl& request) : c(request), m(&request) {}
  raw_ptr<Delegate> incoming_delegate = nullptr;
  raw_ptr<const HostCache::Entry> incoming_result = nullptr;
  ResolutionDetails incoming_details;
  std::stringstream debug;
  DnsResolverWords Run(dns_request_family::EndpointEvent event, DnsResolverWords in = {}) {
    return rustium_dns_request_family_run(2, static_cast<uint32_t>(event), in, this, &Call);
  }
 private:
  using O = dns_request_family::EndpointOp;
  static DnsResolverWords Call(void* context, uint32_t op, DnsResolverWords in) {
    return static_cast<RustFrame*>(context)->Native(static_cast<O>(op), in);
  }
  ServiceEndpointRequestImpl& Mutable() { CHECK(m); return *m; }
  const HostCache::Entry& Result() const { return incoming_result ? *incoming_result : *local; }
  DnsResolverWords Native(O op, DnsResolverWords in) {
    using dns_request_family::W;
    switch (op) {
      case O::kRead:
        switch (in.x) {
          case 0: return W(c->job_.has_value());
          case 1: return W(!!c->resolve_context_);
          case 2: return W(c->parameters_.source == HostResolverSource::LOCAL_ONLY);
          case 3: return W(static_cast<int>(c->parameters_.cache_usage));
          case 4: return W(c->stale_info_.has_value());
          case 5: return W(c->stale_info_->is_stale());
          case 6: return W(c->finalized_result_.has_value());
          case 7: return W(!c->stale_endpoints_.empty());
          case 8: return W(!!c->delegate_);
          case 9: return W(static_cast<int>(c->next_state_));
          case 10: return W(c->tasks_.empty());
          case 11: return W(c->parameters_.is_speculative);
          case 12: return W(!c->finalized_result_->endpoints.empty());
          default: NOTREACHED();
        }
      case O::kWrite:
        switch (in.x) {
          case 1: Mutable().next_state_ = static_cast<State>(in.y); break;
          case 2: Mutable().priority_ = static_cast<RequestPriority>(in.y); break;
          case 3: Mutable().delegate_ = incoming_delegate; break;
          case 4: Mutable().delegate_ = nullptr; break;
          default: NOTREACHED();
        }
        return {};
      case O::kCheck:
        switch (in.x) {
          case 0: CHECK(!c->delegate_); CHECK(c->manager_); break;
          case 1: CHECK(c->previous() == nullptr); CHECK(c->next() == nullptr); break;
          case 2: CHECK(c->delegate_); break;
          case 3: { CHECK(c->job_); CHECK(c->delegate_); DCHECK_CALLED_ON_VALID_SEQUENCE(c->sequence_checker_); break; }
          case 4: CHECK(c->job_); break;
          case 6: CHECK(c->job_); CHECK(c->delegate_); break;
          case 7: CHECK(!c->finalized_result_); break;
          case 8: CHECK_EQ(cache_usage, ResolveHostParameters::CacheUsage::STALE_ALLOWED); break;
          case 9: CHECK(!fresh_info || !fresh_info->is_stale()); break;
          case 10: CHECK(c->parameters_.source != HostResolverSource::LOCAL_ONLY); break;
          case 11: CHECK(!c->tasks_.empty()); break;
          case 12: NOTREACHED() << "Invalid state";
          default: NOTREACHED();
        }
        return {};
      case O::kCancelJob: c->job_.value()->CancelServiceEndpointRequest(&Mutable()); return {};
      case O::kLogCancel: Mutable().LogCancelRequest(); return {};
      case O::kResetJob: Mutable().job_.reset(); return {};
      case O::kRunFinished: c->delegate_->OnServiceEndpointRequestFinished(in.x); return {};
      case O::kRunUpdated: c->delegate_->OnServiceEndpointsUpdated(); return {};
      case O::kInitializeKey:
        Mutable().job_key_.emplace(c->host_, c->target_network_, c->resolve_context_.get());
        c->manager_->InitializeJobKeyAndIPAddress(c->network_anonymization_key_, c->parameters_, c->net_log_, *Mutable().job_key_, ip); return {};
      case O::kResolveLocal:
        cache_usage = static_cast<ResolveHostParameters::CacheUsage>(in.x);
        local.emplace(c->manager_->ResolveLocally(false, *c->job_key_, ip, cache_usage, c->parameters_.secure_dns_policy, c->parameters_.source, c->net_log_, c->host_cache(), &Mutable().tasks_, &Mutable().stale_info_)); return {};
      case O::kLocalError: return W(Result().error());
      case O::kFreshError: return W(fresh->error());
      case O::kClearTasks: Mutable().tasks_.clear(); return {};
      case O::kResolveFresh:
        fresh.emplace(c->manager_->ResolveLocally(false, *c->job_key_, ip, ResolveHostParameters::CacheUsage::ALLOWED, c->parameters_.secure_dns_policy, c->parameters_.source, c->net_log_, c->host_cache(), &Mutable().tasks_, &fresh_info, false)); return {};
      case O::kAdoptFresh: Mutable().stale_info_ = fresh_info; local = std::move(fresh); return {};
      case O::kCurrentNetwork: return W(c->host_cache()->network_changes());
      case O::kResultNetwork: return W(Result().network_changes());
      case O::kConvertStale: Mutable().stale_endpoints_ = Result().ConvertToServiceEndpoints(c->host_.GetPort()); return {};
      case O::kResetStale:
        switch (in.x) {
          case 0: Mutable().stale_endpoints_.clear(); [[fallthrough]];
          case 1: Mutable().stale_info_.reset(); break;
          default: NOTREACHED();
        }
        return {};
      case O::kLogStale:
        c->net_log_.AddEvent(NetLogEventType::HOST_RESOLVER_SERVICE_ENDPOINTS_STALE_RESULTS, [&] {
          base::ListValue endpoints;
          for (const auto& endpoint : c->stale_endpoints_) { endpoints.Append(endpoint.ToValue()); }
          return base::DictValue().Set("endpoints", std::move(endpoints));
        }); return {};
      case O::kPostStale:
        base::SequencedTaskRunner::GetCurrentDefault()->PostTask(FROM_HERE, base::BindOnce(&ServiceEndpointRequestImpl::NotifyDelegateOfUpdated, Mutable().weak_ptr_factory_.GetWeakPtr())); return {};
      case O::kStartJob:
        Mutable().initial_tasks_ = base::ToVector(c->tasks_);
        c->manager_->CreateAndStartJobForServiceEndpointRequest(std::move(*Mutable().job_key_), std::move(Mutable().tasks_), &Mutable()); return {};
      case O::kFinalizeResult:
        Mutable().finalized_result_.emplace(Result().ConvertToServiceEndpoints(c->host_.GetPort()), dns_alias_utility::FixUpDnsAliases(Result().aliases())); return {};
      case O::kMakeEmpty: Mutable().finalized_result_.emplace(std::vector<ServiceEndpoint>{}, base::flat_set<std::string>{}); return {};
      case O::kSquash: return W(HostResolver::SquashErrorCode(in.x));
      case O::kSetError: Mutable().error_info_ = ResolveErrorInfo(in.x, in.y); return {};
      case O::kSetDetails:
        switch (in.x) {
          case 0: Mutable().resolution_details_ = incoming_details; break;
          default: Mutable().resolution_details_.emplace(); Mutable().resolution_details_->source = static_cast<ResolutionSource>(in.x); break;
        }
        return {};
      case O::kHasTaskManager: return W(!!c->job_.value()->dns_task_results_manager());
      case O::kGetCryptoReady: return W(c->job_.value()->dns_task_results_manager()->IsMetadataReady());
      case O::kStartIPv6:
        switch (in.x) {
          case 1: return W(c->manager_->StartIPv6ReachabilityCheck(c->target_network_, c->net_log_, Mutable().GetClientSocketFactory(), base::DoNothingAs<void(int)>()));
          case 0: return W(c->manager_->StartIPv6ReachabilityCheck(c->target_network_, c->net_log_, Mutable().GetClientSocketFactory(), base::BindOnce(&ServiceEndpointRequestImpl::OnIOComplete, Mutable().weak_ptr_factory_.GetWeakPtr())));
          default: NOTREACHED();
        }
      case O::kChangeJobPriority: c->job_.value()->ChangeServiceEndpointRequestPriority(&Mutable(), static_cast<RequestPriority>(in.x)); return {};
      case O::kDebugTaskCount: return W(0, c->initial_tasks_.size());
      case O::kDebugTask: return W(static_cast<int>(c->initial_tasks_.at(in.a)));
      case O::kDebugAppend:
        switch (in.x) {
          case 0: { static constexpr std::array<const char*, 4> parts{"it=[", ",", "],j=", ",rm="}; debug << parts.at(in.a); break; }
          case 1: debug << in.y; break;
          default: NOTREACHED();
        }
        return {};
      case O::kReleaseLocal: local.reset(); fresh.reset(); fresh_info.reset(); return {};
    }
    NOTREACHED();
  }
  raw_ref<const ServiceEndpointRequestImpl> c;
  raw_ptr<ServiceEndpointRequestImpl> m = nullptr;
  IPAddress ip;
  ResolveHostParameters::CacheUsage cache_usage = ResolveHostParameters::CacheUsage::ALLOWED;
  std::optional<HostCache::Entry> local;
  std::optional<HostCache::Entry> fresh;
  std::optional<HostCache::EntryStaleness> fresh_info;
};

HostResolverManager::ServiceEndpointRequestImpl::FinalizedResult::FinalizedResult(std::vector<ServiceEndpoint> endpoints,
                    base::flat_set<std::string> dns_aliases)
    : endpoints(std::move(endpoints)), dns_aliases(std::move(dns_aliases)) {}

HostResolverManager::ServiceEndpointRequestImpl::FinalizedResult::~FinalizedResult() = default;

HostResolverManager::ServiceEndpointRequestImpl::FinalizedResult::FinalizedResult(FinalizedResult&&) = default;
HostResolverManager::ServiceEndpointRequestImpl::FinalizedResult&
HostResolverManager::ServiceEndpointRequestImpl::FinalizedResult::operator=(
    FinalizedResult&&) = default;

HostResolverManager::ServiceEndpointRequestImpl::ServiceEndpointRequestImpl(
    HostResolver::Host host,
    NetworkAnonymizationKey network_anonymization_key,
    handles::NetworkHandle target_network,
    NetLogWithSource net_log,
    ResolveHostParameters parameters,
    base::WeakPtr<ResolveContext> resolve_context,
    base::WeakPtr<HostResolverManager> manager,
    const base::TickClock* tick_clock)
    : host_(std::move(host)),
      network_anonymization_key_(
          DnsRequestRetainPartitionKey(std::move(network_anonymization_key))),
      target_network_(target_network),
      net_log_(std::move(net_log)),
      parameters_(std::move(parameters)),
      resolve_context_(std::move(resolve_context)),
      manager_(std::move(manager)),
      tick_clock_(tick_clock),
      priority_(parameters_.initial_priority) {}

HostResolverManager::ServiceEndpointRequestImpl::~ServiceEndpointRequestImpl() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  RustFrame frame(*this);
  frame.Run(dns_request_family::EndpointEvent::kDestroy);
}

int HostResolverManager::ServiceEndpointRequestImpl::Start(Delegate* delegate) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  RustFrame frame(*this); frame.incoming_delegate = delegate;
  return frame.Run(dns_request_family::EndpointEvent::kStart).x;
}

const HostCache::EntryStaleness*
HostResolverManager::ServiceEndpointRequestImpl::GetStaleInfo() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return base::OptionalToPtr(stale_info_);
}

bool HostResolverManager::ServiceEndpointRequestImpl::IsStaleWhileRefreshing()
    const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  RustFrame frame(*this);
  return frame.Run(dns_request_family::EndpointEvent::kIsStaleRefreshing).x;
}

base::span<const ServiceEndpoint>
HostResolverManager::ServiceEndpointRequestImpl::GetEndpointResults() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  RustFrame frame(*this);
  switch (frame.Run(dns_request_family::EndpointEvent::kEndpointSource).x) {
    case 1: return finalized_result_->endpoints;
    case 2: return stale_endpoints_;
    case 3: return job_.value()->dns_task_results_manager()->GetCurrentEndpoints();
    default: return {};
  }
}

const base::flat_set<std::string>&
HostResolverManager::ServiceEndpointRequestImpl::GetDnsAliasResults() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  RustFrame frame(*this);
  switch (frame.Run(dns_request_family::EndpointEvent::kAliasSource).x) {
    case 1: return finalized_result_->dns_aliases;
    case 3: return job_.value()->dns_task_results_manager()->GetAliases();
    default: return dns_alias_utility::EmptyDnsAliases();
  }
}

bool HostResolverManager::ServiceEndpointRequestImpl::EndpointsCryptoReady() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  RustFrame frame(*this);
  return frame.Run(dns_request_family::EndpointEvent::kCryptoReady).x;
}

std::optional<ResolutionDetails>
HostResolverManager::ServiceEndpointRequestImpl::GetResolutionDetails() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return resolution_details_;
}

ResolveErrorInfo
HostResolverManager::ServiceEndpointRequestImpl::GetResolveErrorInfo() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return error_info_;
}

void HostResolverManager::ServiceEndpointRequestImpl::ChangeRequestPriority(
    RequestPriority priority) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  RustFrame frame(*this); frame.Run(dns_request_family::EndpointEvent::kChangePriority, dns_request_family::W(priority));
}

std::string HostResolverManager::ServiceEndpointRequestImpl::DebugString()
    const {
  RustFrame frame(*this); frame.Run(dns_request_family::EndpointEvent::kDebug);
  return frame.debug.str();
}

void HostResolverManager::ServiceEndpointRequestImpl::AssignJob(
    base::SafeRef<Job> job) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(!job_.has_value());
  job_ = job;
}

void HostResolverManager::ServiceEndpointRequestImpl::OnJobCompleted(
    const HostCache::Entry& results,
    bool obtained_securely,
    ResolutionDetails resolution_details) {
  RustFrame frame(*this); frame.incoming_result = &results; frame.incoming_details = resolution_details;
  frame.Run(dns_request_family::EndpointEvent::kCompleted, dns_request_family::W(results.error(), 0, 0, obtained_securely));
}

void HostResolverManager::ServiceEndpointRequestImpl::OnJobCancelled() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  RustFrame frame(*this);
  frame.Run(dns_request_family::EndpointEvent::kCancelled);
}

void HostResolverManager::ServiceEndpointRequestImpl::OnServiceEndpointsChanged() {
  RustFrame frame(*this);
  frame.Run(dns_request_family::EndpointEvent::kChanged);
}

base::WeakPtr<HostResolverManager::ServiceEndpointRequestImpl>
HostResolverManager::ServiceEndpointRequestImpl::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

int HostResolverManager::ServiceEndpointRequestImpl::DoLoop(int rv) {
  RustFrame frame(*this);
  return frame.Run(dns_request_family::EndpointEvent::kLoop, dns_request_family::W(rv)).x;
}

int HostResolverManager::ServiceEndpointRequestImpl::DoCheckIPv6Reachability() {
  RustFrame frame(*this);
  return frame.Run(dns_request_family::EndpointEvent::kIPv6).x;
}

int HostResolverManager::ServiceEndpointRequestImpl::DoCheckIPv6ReachabilityComplete(int rv) {
  RustFrame frame(*this);
  return frame.Run(dns_request_family::EndpointEvent::kIPv6Complete, dns_request_family::W(rv)).x;
}

int HostResolverManager::ServiceEndpointRequestImpl::DoResolveLocally() {
  RustFrame frame(*this);
  return frame.Run(dns_request_family::EndpointEvent::kResolveLocal).x;
}

int HostResolverManager::ServiceEndpointRequestImpl::DoStartJob() {
  RustFrame frame(*this);
  return frame.Run(dns_request_family::EndpointEvent::kStartJob).x;
}

void HostResolverManager::ServiceEndpointRequestImpl::OnIOComplete(int rv) {
  RustFrame frame(*this);
  frame.Run(dns_request_family::EndpointEvent::kIOComplete, dns_request_family::W(rv));
}

void HostResolverManager::ServiceEndpointRequestImpl::SetFinalizedResultFromLegacyResults(const HostCache::Entry& results) {
  RustFrame frame(*this); frame.incoming_result = &results;
  frame.Run(dns_request_family::EndpointEvent::kFinalize, dns_request_family::W(results.error()));
}

void HostResolverManager::ServiceEndpointRequestImpl::MaybeClearStaleResults() {
  RustFrame frame(*this);
  frame.Run(dns_request_family::EndpointEvent::kClearStale);
}

void HostResolverManager::ServiceEndpointRequestImpl::LogCancelRequest() {
  net_log_.AddEvent(NetLogEventType::CANCELLED);
  net_log_.EndEvent(NetLogEventType::HOST_RESOLVER_MANAGER_REQUEST);
}

void HostResolverManager::ServiceEndpointRequestImpl::NotifyDelegateOfUpdated() {
  RustFrame frame(*this);
  frame.Run(dns_request_family::EndpointEvent::kNotify);
}

ClientSocketFactory*
HostResolverManager::ServiceEndpointRequestImpl::GetClientSocketFactory() {
  if (resolve_context_->url_request_context()) {
    return resolve_context_->url_request_context()
        ->GetNetworkSessionContext()
        ->client_socket_factory;
  }
  return ClientSocketFactory::GetDefaultFactory();
}

}  // namespace net
