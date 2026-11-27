// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/dns/host_resolver_manager_job.h"
#include "net/dns/dns_resolution_policy_ffi.h"

#include <algorithm>
#include <deque>
#include <memory>
#include <optional>
#include <set>
#include <variant>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/containers/linked_list.h"
#include "base/feature_list.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/safe_ref.h"
#include "base/memory/weak_ptr.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/strcat.h"
#include "base/task/sequenced_task_runner.h"
#include "base/time/time.h"
#include "net/base/address_family.h"
#include "net/base/ech_mode.h"
#include "net/base/features.h"
#include "net/base/network_anonymization_key.h"
#include "net/base/network_handle.h"
#include "net/base/prioritized_dispatcher.h"
#include "net/base/url_util.h"
#include "net/dns/dns_client.h"
#include "net/dns/dns_task_results_manager.h"
#include "net/dns/dns_transaction.h"
#include "net/dns/host_cache.h"
#include "net/dns/host_resolver.h"
#include "net/dns/host_resolver_dns_task.h"
#include "net/dns/host_resolver_internal_result.h"
#include "net/dns/host_resolver_manager.h"
#include "net/dns/host_resolver_manager_request_impl.h"
#include "net/dns/host_resolver_manager_service_endpoint_request_impl.h"
#include "net/dns/host_resolver_mdns_task.h"
#include "net/dns/host_resolver_nat64_task.h"
#include "net/dns/host_resolver_system_task.h"
#include "net/dns/public/dns_query_type.h"
#include "net/dns/public/secure_dns_mode.h"
#include "net/log/net_log_with_source.h"
#include "url/url_constants.h"

namespace net {

namespace {

// Default TTL for successful resolutions with HostResolverSystemTask.
const unsigned kCacheEntryTTLSeconds = 60;

// Default TTL for unsuccessful resolutions with HostResolverSystemTask.
const unsigned kNegativeCacheEntryTTLSeconds = 0;

// Minimum TTL for successful resolutions with HostResolverDnsTask.
const unsigned kMinimumTTLSeconds = kCacheEntryTTLSeconds;

// ICANN uses this localhost address to indicate a name collision.
//
// The policy in Chromium is to fail host resolving if it resolves to
// this special address.
//
// Not however that IP literals are exempt from this policy, so it is still
// possible to navigate to http://127.0.53.53/ directly.
//
// For more details: https://www.icann.org/news/announcement-2-2014-08-01-en
constexpr uint8_t kIcanNameCollisionIp[] = {127, 0, 53, 53};

bool ContainsIcannNameCollisionIp(const std::vector<IPEndPoint>& endpoints) {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { return RWords::Unsigned(static_cast<uint64_t>(endpoints.size())); }
      case 2: { return RWords::Bool(endpoints.at(args.a).address().IsIPv4()); }
      case 3: { const auto& bytes=endpoints.at(args.a).address().bytes();return RWords::Unsigned(static_cast<uint64_t>(bytes[0]) | (static_cast<uint64_t>(bytes[1])<<8) | (static_cast<uint64_t>(bytes[2])<<16) | (static_cast<uint64_t>(bytes[3])<<24)); }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 55, native_service);
  return rust_reply.a!=0;
}

// Creates NetLog parameters for HOST_RESOLVER_MANAGER_JOB_ATTACH/DETACH events.
base::DictValue NetLogJobAttachParams(const NetLogSource& source,
                                      RequestPriority priority) {
  base::DictValue dict;
  source.AddToEventParameters(dict);
  dict.Set("priority", RequestPriorityToString(priority));
  return dict;
}

bool IsAttemptModeSecure(DnsTransactionFactory::AttemptMode attempt_mode) {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { return RWords::Unsigned(static_cast<uint64_t>(attempt_mode)); }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 56, native_service);
  return rust_reply.a!=0;
}

}  // namespace

HostResolverManager::JobKey::JobKey(HostResolver::Host host,
                                    handles::NetworkHandle target_network,
                                    ResolveContext* resolve_context)
    : host(std::move(host)),
      resolve_context(resolve_context->GetWeakPtr()),
      target_network(target_network) {}

HostResolverManager::JobKey::~JobKey() = default;

HostResolverManager::JobKey::JobKey(const JobKey& other) = default;
HostResolverManager::JobKey& HostResolverManager::JobKey::operator=(
    const JobKey& other) = default;

bool HostResolverManager::JobKey::operator<(const JobKey& other) const {
  using RWords = DnsResolutionWords;

handles::NetworkHandle network,other_network;const ResolveContext* context=nullptr;const ResolveContext* other_context=nullptr;

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { switch(args.a){case 0:return RWords::Unsigned(query_types.ToEnumBitmask());case 1:return RWords::Signed(flags);case 2:return RWords::Unsigned(static_cast<uint64_t>(source));case 3:return RWords::Unsigned(static_cast<uint64_t>(secure_dns_mode));default:NOTREACHED();}
         }
      case 2: { switch(args.a){case 0:return RWords::Unsigned(other.query_types.ToEnumBitmask());case 1:return RWords::Signed(other.flags);case 2:return RWords::Unsigned(static_cast<uint64_t>(other.source));case 3:return RWords::Unsigned(static_cast<uint64_t>(other.secure_dns_mode));default:NOTREACHED();}
         }
      case 3: { switch(args.a){case 0:return RWords::Bool(std::compare_three_way{}(context,other_context)<0);case 1:return RWords::Bool(host < other.host);case 2:return RWords::Bool(network_anonymization_key < other.network_anonymization_key);default:NOTREACHED();}
         }
      case 4: { switch(args.a){case 0:return RWords::Bool(std::compare_three_way{}(other_context,context)<0);case 1:return RWords::Bool(other.host < host);case 2:return RWords::Bool(other.network_anonymization_key < network_anonymization_key);default:NOTREACHED();}
         }
      case 5: { return RWords::Signed(static_cast<int64_t>(network)); }
      case 6: { return RWords::Signed(static_cast<int64_t>(other_network)); }
      case 7: { network=GetTargetNetwork();other_network=other.GetTargetNetwork();context=&*resolve_context;other_context=&*other.resolve_context;
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 36, native_service);
  return rust_reply.a!=0;
}

bool HostResolverManager::JobKey::operator==(const JobKey& other) const {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { return RWords::Bool(*this<other); }
      case 2: { return RWords::Bool(other<*this); }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 37, native_service);
  return rust_reply.a!=0;
}

HostCache::Key HostResolverManager::JobKey::ToCacheKey(bool secure) const {
  using RWords = DnsResolutionWords;

std::variant<url::SchemeHostPort,std::string> host_for_cache;std::optional<HostCache::Key> result;

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { return RWords::Unsigned(static_cast<uint64_t>(query_types.size())); }
      case 2: { DCHECK(Difference(query_types,{DnsQueryType::A,DnsQueryType::AAAA,DnsQueryType::HTTPS}).empty());
        return {}; }
      case 3: { return RWords::Unsigned(static_cast<uint64_t>(*query_types.begin())); }
      case 4: { return RWords::Bool(host.HasScheme()); }
      case 5: { host_for_cache=host.AsSchemeHostPort();
        return {}; }
      case 6: { host_for_cache=std::string(host.GetHostnameWithoutBrackets());
        return {}; }
      case 7: { result.emplace(std::move(host_for_cache),static_cast<DnsQueryType>(args.a),flags,source,network_anonymization_key,GetTargetNetwork());result->secure=secure;
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 38, native_service);
  return std::move(result).value();
}

handles::NetworkHandle HostResolverManager::JobKey::GetTargetNetwork() const {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { return RWords::Bool(resolve_context.get()!=nullptr); }
      case 2: { return RWords::Signed(static_cast<int64_t>(resolve_context->GetTargetNetwork())); }
      case 3: { CHECK_EQ(target_network,handles::kInvalidNetworkHandle);
        return {}; }
      case 4: { return RWords::Signed(static_cast<int64_t>(target_network)); }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 3, native_service);
  return static_cast<handles::NetworkHandle>(rust_reply.x);
}

EchMode HostResolverManager::JobKey::GetEchMode() const {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { return RWords::Bool(resolve_context.get()!=nullptr); }
      case 2: { return RWords::Unsigned(static_cast<uint64_t>(resolve_context->GetEchMode(host.GetHostnameWithoutBrackets()))); }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 4, native_service);
  return static_cast<EchMode>(rust_reply.a);
}

// static
// WARNING: This method assumes the current task ordering configured by
// HostResolverManager::ResolveLocally() and
// HostResolverManager::CreateTaskSequence(). If you modify the task ordering
// in those methods, update this method accordingly.
std::optional<HostResolverManager::Job::ResolveFallbackPath>
HostResolverManager::Job::CalculateResolvePath(TaskType task_type,
                                               bool secure_dns_failed,
                                               bool classic_dns_failed,
                                               bool platform_dns_failed) {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { return RWords::Unsigned(static_cast<uint64_t>(task_type)); }
      case 2: { return {.a=secure_dns_failed,.b=classic_dns_failed,.x=platform_dns_failed}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 1, native_service);
  return rust_reply.a?std::make_optional(static_cast<ResolveFallbackPath>(rust_reply.b)):std::nullopt;
}

HostResolverManager::Job::Job(
    const base::WeakPtr<HostResolverManager>& resolver,
    JobKey key,
    ResolveHostParameters::CacheUsage cache_usage,
    HostCache* host_cache,
    std::deque<TaskType> tasks,
    RequestPriority priority,
    const NetLogWithSource& source_net_log,
    const base::TickClock* tick_clock,
    const HostResolver::HttpsSvcbOptions& https_svcb_options)
    : resolver_(resolver),
      key_(std::move(key)),
      cache_usage_(cache_usage),
      host_cache_(host_cache),
      tasks_(tasks),
      priority_tracker_(priority),
      tick_clock_(tick_clock),
      https_svcb_options_(https_svcb_options),
      net_log_(
          NetLogWithSource::Make(source_net_log.net_log(),
                                 NetLogSourceType::HOST_RESOLVER_IMPL_JOB)) {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { source_net_log.AddEvent(NetLogEventType::HOST_RESOLVER_MANAGER_CREATE_JOB);
        return {}; }
      case 2: { net_log_.BeginEvent(NetLogEventType::HOST_RESOLVER_MANAGER_JOB,[&]{return NetLogJobCreationParams(source_net_log.source());});
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 53, native_service);
  (void)rust_reply;
}

HostResolverManager::Job::~Job() {
  using RWords = DnsResolutionWords;

RequestImpl* request=nullptr;ServiceEndpointRequestImpl* endpoint=nullptr;

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { return RWords::Bool(is_queued()); }
      case 2: { return RWords::Bool(is_running()); }
      case 3: { Finish();
        return {}; }
      case 4: { net_log_.EndEventWithNetErrorCode(NetLogEventType::HOST_RESOLVER_MANAGER_JOB,ERR_ABORTED);
        return {}; }
      case 5: { net_log_.AddEvent(NetLogEventType::CANCELLED);net_log_.EndEvent(NetLogEventType::HOST_RESOLVER_MANAGER_JOB);
        return {}; }
      case 6: { return RWords::Bool(!requests_.empty()); }
      case 7: { request=requests_.head()->value();
        return {}; }
      case 8: { request->RemoveFromList();
        return {}; }
      case 9: { CHECK(key_==request->GetJobKey());
        return {}; }
      case 10: { request->OnJobCancelled(key_);
        return {}; }
      case 11: { return RWords::Bool(!service_endpoint_requests_.empty()); }
      case 12: { endpoint=service_endpoint_requests_.head()->value();
        return {}; }
      case 13: { endpoint->RemoveFromList();
        return {}; }
      case 14: { endpoint->OnJobCancelled();
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 39, native_service);
  (void)rust_reply;
}

HostResolverManager::TaskType HostResolverManager::Job::AttemptModeToTaskType(
    DnsTransactionFactory::AttemptMode attempt_mode) {
  using RWords = DnsResolutionWords;
static_assert(static_cast<int>(DnsTransactionFactory::AttemptMode::kClassic)==0 && static_cast<int>(DnsTransactionFactory::AttemptMode::kHttp)==1 && static_cast<int>(DnsTransactionFactory::AttemptMode::kPlatform)==2);
static_assert(kCacheEntryTTLSeconds==60 && kNegativeCacheEntryTTLSeconds==0 && kMinimumTTLSeconds==60);
static_assert(kIcanNameCollisionIp[0]==127 && kIcanNameCollisionIp[1]==0 && kIcanNameCollisionIp[2]==53 && kIcanNameCollisionIp[3]==53);


  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { switch(attempt_mode){case DnsTransactionFactory::AttemptMode::kClassic:return RWords::Unsigned(0);case DnsTransactionFactory::AttemptMode::kHttp:return RWords::Unsigned(1);case DnsTransactionFactory::AttemptMode::kPlatform:return RWords::Unsigned(2);}NOTREACHED(); }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 2, native_service);
  return static_cast<TaskType>(rust_reply.a);
}

void HostResolverManager::Job::Schedule(bool at_head) {
  using RWords = DnsResolutionWords;

PrioritizedDispatcher::Handle handle;

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { DCHECK(!is_queued());DCHECK(dispatched_);
        return {}; }
      case 2: { return RWords::Bool(at_head); }
      case 3: { handle=resolver_->dispatcher_->AddAtHead(this,priority());
        return {}; }
      case 4: { handle=resolver_->dispatcher_->Add(this,priority());
        return {}; }
      case 5: { return RWords::Bool(handle.is_null()); }
      case 6: { DCHECK(handle_.is_null());handle_=handle;
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 5, native_service);
  (void)rust_reply;
}

void HostResolverManager::Job::AddRequest(RequestImpl* request) {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { DCHECK_EQ(host_cache_,request->host_cache());DCHECK_EQ(key_.host.GetHostnameWithoutBrackets(),request->request_host().GetHostnameWithoutBrackets());
        return {}; }
      case 2: { request->AssignJob(weak_ptr_factory_.GetSafeRef());
        return {}; }
      case 3: { AddRequestCommon(request->priority(),request->source_net_log(),request->parameters().is_speculative);
        return {}; }
      case 4: { requests_.Append(request);
        return {}; }
      case 5: { UpdatePriority();
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 8, native_service);
  (void)rust_reply;
}

void HostResolverManager::Job::ChangeRequestPriority(RequestImpl* req,
                                                     RequestPriority priority) {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { DCHECK_EQ(key_.host,req->request_host());
        return {}; }
      case 2: { priority_tracker_.Remove(req->priority());
        return {}; }
      case 3: { req->set_priority(priority);
        return {}; }
      case 4: { priority_tracker_.Add(req->priority());
        return {}; }
      case 5: { UpdatePriority();
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 42, native_service);
  (void)rust_reply;
}

void HostResolverManager::Job::CancelRequest(RequestImpl* request) {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { DCHECK_EQ(key_.host,request->request_host());DCHECK(!requests_.empty());
        return {}; }
      case 2: { CancelRequestCommon(request->priority(),request->source_net_log());
        return {}; }
      case 3: { return RWords::Unsigned(static_cast<uint64_t>(num_active_requests())); }
      case 4: { UpdatePriority();
        return {}; }
      case 5: { request->RemoveFromList();
        return {}; }
      case 6: { CompleteRequestsWithError(ERR_DNS_REQUEST_CANCELLED,std::nullopt);
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 6, native_service);
  (void)rust_reply;
}

void HostResolverManager::Job::AddServiceEndpointRequest(
    ServiceEndpointRequestImpl* request) {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { CHECK_EQ(host_cache_,request->host_cache());
        return {}; }
      case 2: { request->AssignJob(weak_ptr_factory_.GetSafeRef());
        return {}; }
      case 3: { AddRequestCommon(request->priority(),request->net_log(),request->parameters().is_speculative);
        return {}; }
      case 4: { service_endpoint_requests_.Append(request);
        return {}; }
      case 5: { UpdatePriority();
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 9, native_service);
  (void)rust_reply;
}

void HostResolverManager::Job::CancelServiceEndpointRequest(
    ServiceEndpointRequestImpl* request) {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { CHECK(!service_endpoint_requests_.empty());
        return {}; }
      case 2: { CancelRequestCommon(request->priority(),request->net_log());
        return {}; }
      case 3: { return RWords::Unsigned(static_cast<uint64_t>(num_active_requests())); }
      case 4: { UpdatePriority();
        return {}; }
      case 5: { request->RemoveFromList();
        return {}; }
      case 6: { CompleteRequestsWithError(ERR_DNS_REQUEST_CANCELLED,std::nullopt);
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 7, native_service);
  (void)rust_reply;
}

void HostResolverManager::Job::ChangeServiceEndpointRequestPriority(
    ServiceEndpointRequestImpl* request,
    RequestPriority priority) {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { priority_tracker_.Remove(request->priority());
        return {}; }
      case 2: { request->set_priority(priority);
        return {}; }
      case 3: { priority_tracker_.Add(request->priority());
        return {}; }
      case 4: { UpdatePriority();
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 43, native_service);
  (void)rust_reply;
}

void HostResolverManager::Job::Abort() {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { CompleteRequestsWithError(ERR_NETWORK_CHANGED,std::nullopt);
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 44, native_service);
  (void)rust_reply;
}

base::OnceClosure HostResolverManager::Job::GetAbortInsecureDnsTaskClosure(
    int error,
    bool fallback_only) {
  return base::BindOnce(&Job::AbortInsecureDnsTask,
                        weak_ptr_factory_.GetWeakPtr(), error, fallback_only);
}

void HostResolverManager::Job::AbortInsecureDnsTask(int error,
                                                    bool fallback_only) {
  using RWords = DnsResolutionWords;

std::deque<TaskType>::iterator it;

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { return RWords::Bool(std::ranges::contains(tasks_,TaskType::SYSTEM)); }
      case 2: { it=tasks_.begin();
        return {}; }
      case 3: { return RWords::Bool(it!=tasks_.end()); }
      case 4: { return RWords::Unsigned(static_cast<uint64_t>(*it)); }
      case 5: { it=tasks_.erase(it);
        return {}; }
      case 6: { ++it;
        return {}; }
      case 7: { return RWords::Bool(dns_task_!=nullptr); }
      case 8: { return RWords::Bool(dns_task_->secure()); }
      case 9: { KillDnsTask();
        return {}; }
      case 10: { dns_task_error_=OK;
        return {}; }
      case 11: { RunNextTask();
        return {}; }
      case 12: { return RWords::Bool(fallback_only); }
      case 13: { CompleteRequestsWithError(error,std::nullopt);
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 10, native_service);
  (void)rust_reply;
}

void HostResolverManager::Job::OnEvicted() {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { DCHECK(!is_running());DCHECK(is_queued());
        return {}; }
      case 2: { handle_.Reset();
        return {}; }
      case 3: { net_log_.AddEvent(NetLogEventType::HOST_RESOLVER_MANAGER_JOB_EVICTED);
        return {}; }
      case 4: { DCHECK(self_iterator_);base::SequencedTaskRunner::GetCurrentDefault()->PostTask(FROM_HERE,base::BindOnce(&Job::CompleteRequestsWithError,weak_ptr_factory_.GetWeakPtr(),ERR_HOST_RESOLVER_QUEUE_TOO_LARGE,std::nullopt));
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 45, native_service);
  (void)rust_reply;
}

bool HostResolverManager::Job::ServeFromHosts() {
  using RWords = DnsResolutionWords;

std::set<std::unique_ptr<HostResolverInternalResult>> results;std::optional<HostCache::Entry> legacy;

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { DCHECK_GT(num_active_requests(),0u);
        return {}; }
      case 2: { results=resolver_->ServeFromHosts(key_.host.GetHostnameWithoutBrackets(),key_.query_types,key_.flags & HOST_RESOLVER_DEFAULT_FAMILY_SET_DUE_TO_NO_IPV6,tasks_);
        return {}; }
      case 3: { return RWords::Unsigned(static_cast<uint64_t>(results.size())); }
      case 4: { return RWords::Bool((*std::next(results.begin(),args.a))->type()==HostResolverInternalResult::Type::kData); }
      case 5: { legacy.emplace(results,base::Time::Now(),tick_clock_->NowTicks(),HostCache::Entry::SOURCE_HOSTS);
        return {}; }
      case 6: { CompleteRequests(*legacy,base::TimeDelta(),true,true,TaskType::HOSTS);
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 32, native_service);
  return rust_reply.a!=0;
}

void HostResolverManager::Job::OnAddedToJobMap(JobMap::iterator iterator) {
  CHECK(!self_iterator_);
  CHECK(iterator != resolver_->jobs_.end());
  self_iterator_ = iterator;
}

void HostResolverManager::Job::OnRemovedFromJobMap() {
  CHECK(self_iterator_);
  self_iterator_ = std::nullopt;
}

void HostResolverManager::Job::RunNextTask() {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { return RWords::Unsigned(static_cast<uint64_t>(tasks_.size())); }
      case 2: { return RWords::Unsigned(static_cast<uint64_t>(completion_results_.size())); }
      case 3: { CompleteRequestsWithError(ERR_NAME_NOT_RESOLVED,std::nullopt);
        return {}; }
      case 4: { const auto& item=completion_results_.at(args.a);DCHECK_NE(OK,item.entry.error());MaybeCacheResult(item.entry,item.ttl,IsAttemptModeSecure(item.attempt_mode));
        return {}; }
      case 5: { const auto& item=completion_results_.back();DCHECK_NE(OK,item.entry.error());CompleteRequests(item.entry,item.ttl,true,IsAttemptModeSecure(item.attempt_mode),AttemptModeToTaskType(item.attempt_mode));
        return {}; }
      case 6: { return RWords::Unsigned(static_cast<uint64_t>(tasks_.front())); }
      case 7: { return RWords::Bool(dispatched_); }
      case 8: { dispatched_=true;job_running_=false;
        return {}; }
      case 9: { Schedule(false);
        return {}; }
      case 10: { DCHECK(is_running()||is_queued());
        return {}; }
      case 11: { return RWords::Unsigned(static_cast<uint64_t>(resolver_->dispatcher_->num_queued_jobs())); }
      case 12: { return RWords::Unsigned(static_cast<uint64_t>(resolver_->max_queued_jobs_)); }
      case 13: { Job* evicted=static_cast<Job*>(resolver_->dispatcher_->EvictOldestLowest());DCHECK(evicted);evicted->OnEvicted();
        return {}; }
      case 14: { return RWords::Bool(start_time_==base::TimeTicks()); }
      case 15: { net_log_.AddEvent(NetLogEventType::HOST_RESOLVER_MANAGER_JOB_STARTED);
        return {}; }
      case 16: { start_time_=tick_clock_->NowTicks();
        return {}; }
      case 17: { tasks_.pop_front();
        return {}; }
      case 18: { job_running_=true;
        return {}; }
      case 19: { StartSystemTask();
        return {}; }
      case 20: { StartDnsTask(DnsTransactionFactory::AttemptMode::kClassic);
        return {}; }
      case 21: { StartDnsTask(DnsTransactionFactory::AttemptMode::kPlatform);
        return {}; }
      case 22: { StartDnsTask(DnsTransactionFactory::AttemptMode::kHttp);
        return {}; }
      case 23: { StartMdnsTask();
        return {}; }
      case 24: { InsecureCacheLookup();
        return {}; }
      case 25: { StartNat64Task();
        return {}; }
      case 26: { NOTREACHED(); }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 11, native_service);
  (void)rust_reply;
}

base::DictValue HostResolverManager::Job::NetLogJobCreationParams(
    const NetLogSource& source) {
  using RWords = DnsResolutionWords;

base::DictValue dict;base::ListValue types,tasks;decltype(key_.query_types.begin()) query;

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { source.AddToEventParameters(dict);
        return {}; }
      case 2: { dict.Set("host",key_.host.ToString());
        return {}; }
      case 3: { query=key_.query_types.begin();
        return {}; }
      case 4: { return RWords::Bool(query!=key_.query_types.end()); }
      case 5: { types.Append(kDnsQueryTypes.at(*query));
        return {}; }
      case 6: { ++query;
        return {}; }
      case 7: { dict.Set("dns_query_types",std::move(types));
        return {}; }
      case 8: { return RWords::Unsigned(static_cast<uint64_t>(tasks_.size())); }
      case 9: { tasks.Append(static_cast<int>(tasks_.at(args.a)));
        return {}; }
      case 10: { dict.Set("tasks",std::move(tasks));
        return {}; }
      case 11: { dict.Set("secure_dns_mode",base::strict_cast<int>(key_.secure_dns_mode));
        return {}; }
      case 12: { dict.Set("network_anonymization_key",key_.network_anonymization_key.ToDebugString());
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 54, native_service);
  return dict;
}

void HostResolverManager::Job::Finish() {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { return RWords::Bool(is_running()); }
      case 2: { system_task_=nullptr;
        return {}; }
      case 3: { KillDnsTask();
        return {}; }
      case 4: { mdns_task_=nullptr;
        return {}; }
      case 5: { job_running_=false;
        return {}; }
      case 6: { return RWords::Bool(dispatched_); }
      case 7: { DCHECK_EQ(1,num_occupied_job_slots_);
        return {}; }
      case 8: { return RWords::Bool(resolver_.get()!=nullptr); }
      case 9: { resolver_->dispatcher_->OnJobFinished();
        return {}; }
      case 10: { num_occupied_job_slots_=0;
        return {}; }
      case 11: { return RWords::Bool(is_queued()); }
      case 12: { DCHECK(dispatched_);
        return {}; }
      case 13: { resolver_->dispatcher_->Cancel(handle_);
        return {}; }
      case 14: { handle_.Reset();
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 12, native_service);
  (void)rust_reply;
}

void HostResolverManager::Job::KillDnsTask() {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { return RWords::Bool(dns_task_!=nullptr); }
      case 2: { return RWords::Bool(dispatched_); }
      case 3: { return RWords::Signed(static_cast<int64_t>(num_occupied_job_slots_)); }
      case 4: { return RWords::Bool(is_queued()); }
      case 5: { ReduceByOneJobSlot();
        return {}; }
      case 6: { dns_task_.reset();
        return {}; }
      case 7: { dns_task_results_manager_.reset();
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 13, native_service);
  (void)rust_reply;
}

void HostResolverManager::Job::ReduceByOneJobSlot() {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { DCHECK_GE(num_occupied_job_slots_,1);DCHECK(dispatched_);
        return {}; }
      case 2: { return RWords::Bool(is_queued()); }
      case 3: { return RWords::Bool(resolver_.get()!=nullptr); }
      case 4: { resolver_->dispatcher_->Cancel(handle_);
        return {}; }
      case 5: { handle_.Reset();
        return {}; }
      case 6: { return RWords::Signed(static_cast<int64_t>(num_occupied_job_slots_)); }
      case 7: { resolver_->dispatcher_->OnJobFinished();
        return {}; }
      case 8: { --num_occupied_job_slots_;
        return {}; }
      case 9: { NOTREACHED(); }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 14, native_service);
  (void)rust_reply;
}

void HostResolverManager::Job::AddRequestCommon(
    RequestPriority request_priority,
    const NetLogWithSource& request_net_log,
    bool is_speculative) {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { priority_tracker_.Add(request_priority);
        return {}; }
      case 2: { request_net_log.AddEventReferencingSource(NetLogEventType::HOST_RESOLVER_MANAGER_JOB_ATTACH,net_log_.source());
        return {}; }
      case 3: { net_log_.AddEvent(NetLogEventType::HOST_RESOLVER_MANAGER_JOB_REQUEST_ATTACH,[&]{return NetLogJobAttachParams(request_net_log.source(),priority());});
        return {}; }
      case 4: { return RWords::Bool(is_speculative); }
      case 5: { had_non_speculative_request_=true;
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 33, native_service);
  (void)rust_reply;
}

void HostResolverManager::Job::CancelRequestCommon(
    RequestPriority request_priority,
    const NetLogWithSource& request_net_log) {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { priority_tracker_.Remove(request_priority);
        return {}; }
      case 2: { net_log_.AddEvent(NetLogEventType::HOST_RESOLVER_MANAGER_JOB_REQUEST_DETACH,[&]{return NetLogJobAttachParams(request_net_log.source(),priority());});
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 46, native_service);
  (void)rust_reply;
}

void HostResolverManager::Job::UpdatePriority() {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { return RWords::Bool(is_queued()); }
      case 2: { handle_=resolver_->dispatcher_->ChangePriority(handle_,priority());
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 15, native_service);
  (void)rust_reply;
}

void HostResolverManager::Job::Start() {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { handle_.Reset();
        return {}; }
      case 2: { ++num_occupied_job_slots_;
        return {}; }
      case 3: { return RWords::Signed(static_cast<int64_t>(num_occupied_job_slots_)); }
      case 4: { return RWords::Bool(dns_task_!=nullptr); }
      case 5: { resolver_->dispatcher_->OnJobFinished();
        return {}; }
      case 6: { StartNextDnsTransaction();
        return {}; }
      case 7: { DCHECK_EQ(num_occupied_job_slots_,dns_task_->num_transactions_in_progress());
        return {}; }
      case 8: { return RWords::Signed(static_cast<int64_t>(dns_task_->num_additional_transactions_needed())); }
      case 9: { Schedule(true);
        return {}; }
      case 10: { DCHECK(!is_running());DCHECK(!tasks_.empty());
        return {}; }
      case 11: { RunNextTask();
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 16, native_service);
  (void)rust_reply;
}

void HostResolverManager::Job::StartSystemTask() {
  using RWords = DnsResolutionWords;

std::optional<HostResolverSystemTask::CacheParams> cache_params;

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { DCHECK(dispatched_);DCHECK_EQ(1,num_occupied_job_slots_);DCHECK(HasAddressType(key_.query_types));
        return {}; }
      case 2: { return RWords::Bool(key_.resolve_context->host_resolver_cache()!=nullptr); }
      case 3: { cache_params.emplace(*key_.resolve_context->host_resolver_cache(),key_.network_anonymization_key,key_.GetTargetNetwork());
        return {}; }
      case 4: { system_task_=HostResolverSystemTask::Create(std::string(key_.host.GetHostnameWithoutBrackets()),static_cast<AddressFamily>(args.a),key_.flags,priority(),resolver_->host_resolver_system_params_,net_log_,key_.GetTargetNetwork(),std::move(cache_params));
        return {}; }
      case 5: { system_task_->Start(base::BindOnce(&Job::OnSystemTaskComplete,base::Unretained(this),tick_clock_->NowTicks()));
        return {}; }
      case 6: { return {.a=key_.query_types.Has(DnsQueryType::A),.b=key_.query_types.Has(DnsQueryType::AAAA)}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 28, native_service);
  (void)rust_reply;
}

void HostResolverManager::Job::OnSystemTaskComplete(
    base::TimeTicks start_time,
    const AddressList& addr_list,
    int /*os_error*/,
    int net_error) {
  using RWords = DnsResolutionWords;

base::TimeDelta duration;

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { DCHECK(system_task_);
        return {}; }
      case 2: { duration=tick_clock_->NowTicks()-start_time;
        return {}; }
      case 3: { return RWords::Signed(static_cast<int64_t>(net_error)); }
      case 4: { UMA_HISTOGRAM_LONG_TIMES_100("Net.DNS.SystemTask.SuccessTime",duration);
        return {}; }
      case 5: { UMA_HISTOGRAM_LONG_TIMES_100("Net.DNS.SystemTask.FailureTime",duration);
        return {}; }
      case 6: { return RWords::Signed(static_cast<int64_t>(dns_task_error_)); }
      case 7: { resolver_->OnFallbackResolve(dns_task_error_);
        return {}; }
      case 8: { fallback_to_system_task_after_dns_task_=true;
        return {}; }
      case 9: { return RWords::Bool(ContainsIcannNameCollisionIp(addr_list.endpoints())); }
      case 10: { return RWords::Signed(static_cast<int64_t>(ERR_ICANN_NAME_COLLISION)); }
      case 11: { auto aliases=base::flat_set<std::string>(addr_list.dns_aliases());CompleteRequests(HostCache::Entry(static_cast<int>(args.x),args.a?addr_list.endpoints():std::vector<IPEndPoint>(),std::move(aliases),HostCache::Entry::SOURCE_UNKNOWN),base::Seconds(args.a?60:0),true,false,TaskType::SYSTEM,duration);
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 25, native_service);
  (void)rust_reply;
}

void HostResolverManager::Job::InsecureCacheLookup() {
  using RWords = DnsResolutionWords;

std::optional<HostCache::EntryStaleness> stale_info;std::optional<HostCache::Entry> resolved;

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { DCHECK(cache_usage_!=ResolveHostParameters::CacheUsage::STALE_ALLOWED);
        return {}; }
      case 2: { resolved=resolver_->MaybeServeFromCache(host_cache_,key_.ToCacheKey(false),cache_usage_,false,net_log_,&stale_info);
        return {}; }
      case 3: { return RWords::Bool(resolved.has_value()); }
      case 4: { DCHECK(stale_info);DCHECK(!stale_info->is_stale());
        return {}; }
      case 5: { CompleteRequestsWithoutCache(*resolved,std::move(stale_info),TaskType::INSECURE_CACHE_LOOKUP);
        return {}; }
      case 6: { RunNextTask();
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 17, native_service);
  (void)rust_reply;
}

void HostResolverManager::Job::StartDnsTask(
    DnsTransactionFactory::AttemptMode attempt_mode) {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { return RWords::Bool(IsAttemptModeSecure(attempt_mode)); }
      case 2: { DCHECK_EQ(args.a!=0,!dispatched_);DCHECK_EQ(dispatched_?1:0,num_occupied_job_slots_);DCHECK(!resolver_->ShouldForceSystemResolverDueToTestOverride());
        return {}; }
      case 3: { dns_task_=std::make_unique<HostResolverDnsTask>(resolver_->dns_client_.get(),key_.host,key_.network_anonymization_key,key_.query_types,&*key_.resolve_context,attempt_mode,key_.secure_dns_mode,key_.GetTargetNetwork(),this,net_log_,tick_clock_,!tasks_.empty(),https_svcb_options_);
        return {}; }
      case 4: { dns_task_executed_=true;
        return {}; }
      case 5: { secure_dns_attempted_=true;
        return {}; }
      case 6: { return RWords::Bool(base::FeatureList::IsEnabled(features::kEnableIntermediateDnsResults)); }
      case 7: { return RWords::Bool(base::FeatureList::IsEnabled(features::kAsyncDnsQuicJob)); }
      case 8: { return RWords::Bool(resolver_->IsHappyEyeballsV3Enabled()); }
      case 9: { dns_task_results_manager_=std::make_unique<DnsTaskResultsManager>(this,key_.host,key_.query_types,net_log_);
        return {}; }
      case 10: { dns_task_->StartNextTransaction();
        return {}; }
      case 11: { return RWords::Signed(static_cast<int64_t>(dns_task_->num_additional_transactions_needed())); }
      case 12: { DCHECK_EQ(dns_task_->num_additional_transactions_needed(),0);
        return {}; }
      case 13: { Schedule(true);
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 18, native_service);
  (void)rust_reply;
}

void HostResolverManager::Job::StartNextDnsTransaction() {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { DCHECK(dns_task_);DCHECK_EQ(dns_task_->secure(),!dispatched_);DCHECK(!dispatched_ || num_occupied_job_slots_==dns_task_->num_transactions_in_progress()+1);DCHECK_GE(dns_task_->num_additional_transactions_needed(),1);
        return {}; }
      case 2: { dns_task_->StartNextTransaction();
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 47, native_service);
  (void)rust_reply;
}

void HostResolverManager::Job::OnDnsTaskFailure(
    const base::WeakPtr<HostResolverDnsTask>& dns_task,
    base::TimeDelta duration,
    bool allow_fallback,
    const HostCache::Entry& failure_results,
    DnsTransactionFactory::AttemptMode attempt_mode) {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { DCHECK_NE(OK,failure_results.error());
        return {}; }
      case 2: { base::UmaHistogramLongTimes100(base::StrCat({"Net.DNS.DnsTask.",args.a?"Secure":"Insecure",".FailureTime"}),duration);
        return {}; }
      case 3: { return RWords::Bool(IsAttemptModeSecure(attempt_mode)); }
      case 4: { return RWords::Bool(dns_task.get()!=nullptr); }
      case 5: { base::UmaHistogramLongTimes100("Net.DNS.JobQueueTime.Failure",total_transaction_time_queued_);
        return {}; }
      case 6: { completion_results_.emplace_back(failure_results,args.a?failure_results.ttl():base::Seconds(0),attempt_mode);
        return {}; }
      case 7: { return RWords::Bool(failure_results.has_ttl()); }
      case 8: { dns_task_error_=failure_results.error();
        return {}; }
      case 9: { KillDnsTask();
        return {}; }
      case 10: { return RWords::Bool(allow_fallback); }
      case 11: { tasks_.clear();
        return {}; }
      case 12: { RunNextTask();
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 26, native_service);
  (void)rust_reply;
}

void HostResolverManager::Job::OnDnsTaskComplete(
    base::TimeTicks start_time,
    bool allow_fallback,
    HostResolverDnsTask::Results results,
    DnsTransactionFactory::AttemptMode attempt_mode) {
  using RWords = DnsResolutionWords;

std::optional<HostCache::Entry> legacy;base::TimeDelta duration,bounded;

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { DCHECK(dns_task_);
        return {}; }
      case 2: { return RWords::Bool(dns_task_results_manager_!=nullptr); }
      case 3: { dns_task_results_manager_->ProcessDnsTaskComplete(results);
        return {}; }
      case 4: { legacy.emplace(results,base::Time::Now(),tick_clock_->NowTicks(),HostCache::Entry::SOURCE_DNS);
        return {}; }
      case 5: { DCHECK(!key_.query_types.Has(DnsQueryType::UNSPECIFIED));
        return {}; }
      case 6: { return {.a=key_.query_types.Has(DnsQueryType::A),.b=key_.query_types.Has(DnsQueryType::AAAA)}; }
      case 7: { return RWords::Signed(static_cast<int64_t>(legacy->error())); }
      case 8: { return RWords::Unsigned(static_cast<uint64_t>(legacy->ip_endpoints().size())); }
      case 9: { legacy->set_error(ERR_NAME_NOT_RESOLVED);
        return {}; }
      case 10: { duration=tick_clock_->NowTicks()-start_time;
        return {}; }
      case 11: { OnDnsTaskFailure(dns_task_->AsWeakPtr(),duration,allow_fallback,*legacy,attempt_mode);
        return {}; }
      case 12: { dns_task_https_disabled_=dns_task_->https_disabled();
        return {}; }
      case 13: { return RWords::Bool(IsAttemptModeSecure(attempt_mode)); }
      case 14: { base::UmaHistogramLongTimes100(base::StrCat({"Net.DNS.DnsTask.",args.a?"Secure":"Insecure",".SuccessTime"}),duration);
        return {}; }
      case 15: { base::UmaHistogramLongTimes100("Net.DNS.JobQueueTime.Success",total_transaction_time_queued_);
        return {}; }
      case 16: { resolver_->dns_client_->ClearInsecureFallbackFailures();
        return {}; }
      case 17: { return RWords::Bool(legacy->ttl()<base::Seconds(60)); }
      case 18: { bounded=base::Seconds(60);
        return {}; }
      case 19: { bounded=legacy->ttl();
        return {}; }
      case 20: { return RWords::Bool(ContainsIcannNameCollisionIp(legacy->ip_endpoints())); }
      case 21: { CompleteRequestsWithError(ERR_ICANN_NAME_COLLISION,AttemptModeToTaskType(attempt_mode));
        return {}; }
      case 22: { CompleteRequests(*legacy,bounded,true,args.a!=0,AttemptModeToTaskType(attempt_mode),duration,dns_task_->GetDohResolutionDetails());
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 27, native_service);
  (void)rust_reply;
}

void HostResolverManager::Job::OnIntermediateTransactionsComplete(
    std::optional<HostResolverDnsTask::SingleTransactionResults>
        single_transaction_results) {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { return RWords::Bool(dispatched_); }
      case 2: { DCHECK_GE(num_occupied_job_slots_,dns_task_->num_transactions_in_progress());
        return {}; }
      case 3: { return RWords::Signed(static_cast<int64_t>(num_occupied_job_slots_)); }
      case 4: { return RWords::Signed(static_cast<int64_t>(dns_task_->num_transactions_in_progress())); }
      case 5: { return RWords::Signed(static_cast<int64_t>(dns_task_->num_additional_transactions_needed())); }
      case 6: { dns_task_->StartNextTransaction();
        return {}; }
      case 7: { return RWords::Bool(is_queued()); }
      case 8: { resolver_->dispatcher_->Cancel(handle_);
        return {}; }
      case 9: { handle_.Reset();
        return {}; }
      case 10: { ReduceByOneJobSlot();
        return {}; }
      case 11: { return RWords::Bool(dns_task_results_manager_!=nullptr); }
      case 12: { return RWords::Bool(single_transaction_results.has_value()); }
      case 13: { dns_task_results_manager_->ProcessDnsTransactionResults(single_transaction_results->query_type,std::move(single_transaction_results->results));
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 19, native_service);
  (void)rust_reply;
}

bool HostResolverManager::Job::ShouldSortTransactionsIndividually() const {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { return RWords::Bool(dns_task_results_manager_!=nullptr); }
      case 2: { return RWords::Bool(resolver_->IsHappyEyeballsV3Enabled()); }
      case 3: { return RWords::Bool(base::FeatureList::IsEnabled(features::kEnableIntermediateDnsResults)); }
      case 4: { return RWords::Bool(features::kEnableIntermediateDnsResultsSortTransactionsIndividually.Get()); }
      case 5: { return RWords::Bool(base::FeatureList::IsEnabled(features::kAsyncDnsQuicJob)); }
      case 6: { return RWords::Bool(features::kAsyncDnsQuicJobSortTransactionsIndividually.Get()); }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 20, native_service);
  return rust_reply.a!=0;
}

void HostResolverManager::Job::AddTransactionTimeQueued(
    base::TimeDelta time_queued) {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { total_transaction_time_queued_+=time_queued;
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 48, native_service);
  (void)rust_reply;
}

void HostResolverManager::Job::OnServiceEndpointsUpdated() {
  using RWords = DnsResolutionWords;

decltype(service_endpoint_requests_.head()) node=nullptr;

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { node=service_endpoint_requests_.head();
        return {}; }
      case 2: { return RWords::Bool(node!=service_endpoint_requests_.end()); }
      case 3: { base::SequencedTaskRunner::GetCurrentDefault()->PostTask(FROM_HERE,base::BindOnce(&ServiceEndpointRequestImpl::OnServiceEndpointsChanged,node->value()->GetWeakPtr()));
        return {}; }
      case 4: { node=node->next();
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 31, native_service);
  (void)rust_reply;
}

void HostResolverManager::Job::StartMdnsTask() {
  using RWords = DnsResolutionWords;

MDnsClient* client=nullptr;

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { DCHECK_EQ(0,key_.flags & ~HOST_RESOLVER_DEFAULT_FAMILY_SET_DUE_TO_NO_IPV6);
        return {}; }
      case 2: { return RWords::Signed(static_cast<int64_t>(resolver_->GetOrCreateMdnsClient(&client))); }
      case 3: { mdns_task_=std::make_unique<HostResolverMdnsTask>(client,std::string(key_.host.GetHostnameWithoutBrackets()),key_.query_types,priority());
        return {}; }
      case 4: { mdns_task_->Start(base::BindOnce(&Job::OnMdnsTaskComplete,base::Unretained(this),tick_clock_->NowTicks()));
        return {}; }
      case 5: { base::SequencedTaskRunner::GetCurrentDefault()->PostTask(FROM_HERE,base::BindOnce(&Job::OnMdnsImmediateFailure,weak_ptr_factory_.GetWeakPtr(),static_cast<int>(args.x)));
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 29, native_service);
  (void)rust_reply;
}

void HostResolverManager::Job::OnMdnsTaskComplete(base::TimeTicks start_time) {
  using RWords = DnsResolutionWords;

std::set<std::unique_ptr<HostResolverInternalResult>> results;std::optional<HostCache::Entry> legacy;

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { DCHECK(mdns_task_);
        return {}; }
      case 2: { results=mdns_task_->GetResults();legacy.emplace(results,base::Time::Now(),tick_clock_->NowTicks(),HostCache::Entry::SOURCE_UNKNOWN);
        return {}; }
      case 3: { return RWords::Bool(ContainsIcannNameCollisionIp(legacy->ip_endpoints())); }
      case 4: { CompleteRequestsWithError(ERR_ICANN_NAME_COLLISION,TaskType::MDNS);
        return {}; }
      case 5: { CompleteRequestsWithoutCache(*legacy,std::nullopt,TaskType::MDNS,tick_clock_->NowTicks()-start_time);
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 30, native_service);
  (void)rust_reply;
}

void HostResolverManager::Job::OnMdnsImmediateFailure(int rv) {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { DCHECK(mdns_task_);DCHECK_NE(OK,rv);
        return {}; }
      case 2: { CompleteRequestsWithError(rv,TaskType::MDNS);
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 49, native_service);
  (void)rust_reply;
}

void HostResolverManager::Job::StartNat64Task() {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { DCHECK(!nat64_task_);
        return {}; }
      case 2: { nat64_task_=std::make_unique<HostResolverNat64Task>(key_.host.GetHostnameWithoutBrackets(),key_.network_anonymization_key,key_.GetTargetNetwork(),net_log_,&*key_.resolve_context,resolver_,priority());
        return {}; }
      case 3: { nat64_task_->Start(base::BindOnce(&Job::OnNat64TaskComplete,weak_ptr_factory_.GetWeakPtr(),tick_clock_->NowTicks()));
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 50, native_service);
  (void)rust_reply;
}

void HostResolverManager::Job::OnNat64TaskComplete(
    base::TimeTicks start_time,
    std::unique_ptr<HostResolverInternalResult> result) {
  using RWords = DnsResolutionWords;

std::set<std::unique_ptr<HostResolverInternalResult>> results;std::optional<HostCache::Entry> legacy;

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { CHECK(nat64_task_);CHECK(result);
        return {}; }
      case 2: { results.insert(std::move(result));
        return {}; }
      case 3: { legacy.emplace(results,base::Time::Now(),tick_clock_->NowTicks(),HostCache::Entry::SOURCE_UNKNOWN);
        return {}; }
      case 4: { CompleteRequestsWithoutCache(*legacy,std::nullopt,TaskType::NAT64,tick_clock_->NowTicks()-start_time);
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 51, native_service);
  (void)rust_reply;
}

void HostResolverManager::Job::RecordJobHistograms(
    const HostCache::Entry& results,
    std::optional<TaskType> task_type) {
  using RWords = DnsResolutionWords;

base::TimeDelta duration;

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { return RWords::Signed(static_cast<int64_t>(results.error())); }
      case 2: { duration=tick_clock_->NowTicks()-start_time_;
        return {}; }
      case 3: { return RWords::Bool(had_non_speculative_request_); }
      case 4: { UMA_HISTOGRAM_LONG_TIMES_100("Net.DNS.ResolveSuccessTime",duration);
        return {}; }
      case 5: { return RWords::Signed(static_cast<int64_t>(ERR_NETWORK_CHANGED)); }
      case 6: { return RWords::Signed(static_cast<int64_t>(ERR_HOST_RESOLVER_QUEUE_TOO_LARGE)); }
      case 7: { UMA_HISTOGRAM_LONG_TIMES_100("Net.DNS.ResolveFailureTime",duration);
        return {}; }
      case 8: { DCHECK_LT(args.a,6u);UMA_HISTOGRAM_ENUMERATION("Net.DNS.ResolveCategory",static_cast<int>(args.a),6);
        return {}; }
      case 9: { return RWords::Bool(start_time_==base::TimeTicks()); }
      case 10: { return RWords::Bool(duration<base::Milliseconds(10)); }
      case 11: { base::UmaHistogramSparse("Net.DNS.ResolveError.Fast",std::abs(results.error()));
        return {}; }
      case 12: { base::UmaHistogramSparse("Net.DNS.ResolveError.Slow",std::abs(results.error()));
        return {}; }
      case 13: { return RWords::Bool(task_type.has_value()); }
      case 14: { return RWords::Unsigned(static_cast<uint64_t>(completion_results_.size())); }
      case 15: { switch(completion_results_.at(args.a).attempt_mode){case DnsTransactionFactory::AttemptMode::kClassic:return RWords::Unsigned(0);case DnsTransactionFactory::AttemptMode::kHttp:return RWords::Unsigned(1);case DnsTransactionFactory::AttemptMode::kPlatform:return RWords::Unsigned(2);}NOTREACHED(); }
      case 16: { return RWords::Unsigned(static_cast<uint64_t>(*task_type)); }
      case 17: { base::UmaHistogramEnumeration("Net.DNS.ResolveFallbackPath",static_cast<ResolveFallbackPath>(args.a));
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 35, native_service);
  (void)rust_reply;
}

void HostResolverManager::Job::RecordJobHttpsHistograms() {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { return RWords::Bool(key_.query_types.Has(DnsQueryType::HTTPS)); }
      case 2: { return RWords::Bool(dns_task_executed_); }
      case 3: { return RWords::Bool(dns_task_https_disabled_); }
      case 4: { base::UmaHistogramBoolean("Net.DNS.JobAttemptedHttps",args.a!=0);
        return {}; }
      case 5: { return RWords::Bool(base::FeatureList::IsEnabled(features::kUseDnsHttpsSvcb)); }
      case 6: { return RWords::Bool(base::FeatureList::IsEnabled(features::kAsyncDns)); }
      case 7: { return RWords::Bool(resolver_->dns_client_!=nullptr); }
      case 8: { return RWords::Bool(resolver_->dns_client_->FallbackFromInsecureTransactionPreferred(key_.GetEchMode())); }
      case 9: { return RWords::Bool(fallback_to_system_task_after_dns_task_); }
      case 10: { base::UmaHistogramEnumeration("Net.DNS.JobHttpsNotAttemptedReason",static_cast<HttpsNotAttemptedReason>(args.a));
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 34, native_service);
  (void)rust_reply;
}

void HostResolverManager::Job::MaybeCacheResult(const HostCache::Entry& results,
                                                base::TimeDelta ttl,
                                                bool secure) {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { return RWords::Bool(results.did_complete()); }
      case 2: { resolver_->CacheResult(host_cache_,key_.ToCacheKey(secure),results,ttl);
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 21, native_service);
  (void)rust_reply;
}

void HostResolverManager::Job::CompleteRequests(
    const HostCache::Entry& results,
    base::TimeDelta ttl,
    bool allow_cache,
    bool secure,
    std::optional<TaskType> task_type,
    std::optional<base::TimeDelta> task_completion_delay,
    std::optional<DohResolutionDetails> doh_details) {
  using RWords = DnsResolutionWords;

ResolutionDetails details;std::unique_ptr<Job> self_deleter;RequestImpl* req=nullptr;ServiceEndpointRequestImpl* endpoint=nullptr;

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { CHECK(resolver_.get());
        return {}; }
      case 2: { return RWords::Bool(task_type.has_value()); }
      case 3: { return RWords::Unsigned(static_cast<uint64_t>(*task_type)); }
      case 4: { NOTREACHED(); }
      case 5: { switch(args.a){case 0:details.source=ResolutionSource::kSecure;break;case 1:details.source=ResolutionSource::kInsecure;break;case 2:details.source=ResolutionSource::kSystem;break;case 3:details.source=ResolutionSource::kPlatform;break;case 4:details.source=ResolutionSource::kMdns;break;case 5:details.source=ResolutionSource::kNat64;break;case 6:details.source=ResolutionSource::kCache;break;default:NOTREACHED();}
        return {}; }
      case 6: { return RWords::Signed(static_cast<int64_t>(results.error())); }
      case 7: { details.task_completion_delay=task_completion_delay;
        return {}; }
      case 8: { details.secure_dns_attempted=secure_dns_attempted_;details.doh_details=std::move(doh_details);
        return {}; }
      case 9: { return RWords::Bool(self_iterator_.has_value()); }
      case 10: { self_deleter=resolver_->RemoveJob(*self_iterator_);
        return {}; }
      case 11: { Finish();
        return {}; }
      case 12: { return RWords::Bool(results.error()==ERR_DNS_REQUEST_CANCELLED); }
      case 13: { CancelRequests();
        return {}; }
      case 14: { net_log_.EndEventWithNetErrorCode(NetLogEventType::HOST_RESOLVER_MANAGER_JOB,results.error());
        return {}; }
      case 15: { return RWords::Bool(allow_cache); }
      case 16: { MaybeCacheResult(results,ttl,secure);
        return {}; }
      case 17: { RecordJobHistograms(results,task_type);
        return {}; }
      case 18: { return RWords::Bool(had_non_speculative_request_); }
      case 19: { return RWords::Unsigned(static_cast<uint64_t>(key_.source)); }
      case 20: { RecordJobHttpsHistograms();
        return {}; }
      case 21: { return RWords::Bool(!requests_.empty()); }
      case 22: { req=requests_.head()->value();
        return {}; }
      case 23: { req->RemoveFromList();
        return {}; }
      case 24: { CHECK(key_==req->GetJobKey());
        return {}; }
      case 25: { return RWords::Bool(req->parameters().is_speculative); }
      case 26: { req->SetResults(results.CopyWithDefaultPort(req->request_host().GetPort()),details);
        return {}; }
      case 27: { req->OnJobCompleted(key_,results.error(),secure&&results.error()!=OK);
        return {}; }
      case 28: { return RWords::Bool(resolver_.get()!=nullptr); }
      case 29: { return RWords::Bool(!service_endpoint_requests_.empty()); }
      case 30: { endpoint=service_endpoint_requests_.head()->value();
        return {}; }
      case 31: { endpoint->RemoveFromList();
        return {}; }
      case 32: { endpoint->OnJobCompleted(results,secure,details);
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 24, native_service);
  (void)rust_reply;
}

void HostResolverManager::Job::CompleteRequestsWithoutCache(
    const HostCache::Entry& results,
    std::optional<HostCache::EntryStaleness> stale_info,
    TaskType task_type,
    std::optional<base::TimeDelta> task_completion_delay) {
  using RWords = DnsResolutionWords;

decltype(requests_.head()) node=nullptr;

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { return RWords::Bool(stale_info.has_value()); }
      case 2: { node=requests_.head();
        return {}; }
      case 3: { return RWords::Bool(node!=requests_.end()); }
      case 4: { return RWords::Bool(node->value()->parameters().is_speculative); }
      case 5: { node->value()->set_stale_info(*stale_info);
        return {}; }
      case 6: { node=node->next();
        return {}; }
      case 7: { CompleteRequests(results,base::TimeDelta(),false,false,task_type,task_completion_delay);
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 23, native_service);
  (void)rust_reply;
}

void HostResolverManager::Job::CompleteRequestsWithError(
    int net_error,
    std::optional<TaskType> task_type) {
  using RWords = DnsResolutionWords;



  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { DCHECK_NE(OK,net_error);
        return {}; }
      case 2: { CompleteRequests(HostCache::Entry(net_error,HostCache::Entry::SOURCE_UNKNOWN),base::TimeDelta(),true,false,task_type);
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 52, native_service);
  (void)rust_reply;
}

void HostResolverManager::Job::CancelRequests() {
  using RWords = DnsResolutionWords;

RequestImpl* req=nullptr;ServiceEndpointRequestImpl* endpoint=nullptr;

  auto native_service = [&](uint32_t operation, [[maybe_unused]] RWords args) -> RWords {
    switch (operation) {
      case 1: { net_log_.AddEvent(NetLogEventType::CANCELLED);net_log_.EndEventWithNetErrorCode(NetLogEventType::HOST_RESOLVER_MANAGER_JOB,OK);
        return {}; }
      case 2: { return RWords::Bool(!requests_.empty()); }
      case 3: { req=requests_.head()->value();
        return {}; }
      case 4: { req->RemoveFromList();
        return {}; }
      case 5: { req->OnJobCancelled(key_);
        return {}; }
      case 6: { return RWords::Bool(resolver_.get()!=nullptr); }
      case 7: { return RWords::Bool(!service_endpoint_requests_.empty()); }
      case 8: { endpoint=service_endpoint_requests_.head()->value();
        return {}; }
      case 9: { endpoint->RemoveFromList();
        return {}; }
      case 10: { endpoint->OnJobCancelled();
        return {}; }
      default: NOTREACHED();
    }
  };
  [[maybe_unused]] const RWords rust_reply = DnsResolutionRun(2, 22, native_service);
  (void)rust_reply;
}

RequestPriority HostResolverManager::Job::priority() const {
  return priority_tracker_.highest_priority();
}

}  // namespace net
