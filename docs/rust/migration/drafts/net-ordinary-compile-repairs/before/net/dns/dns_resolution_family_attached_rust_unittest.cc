// Copyright 2026 The Chromium Authors
// BSD-style license; see LICENSE.
// Ordinary real-adapter tests. No I/O/campaign and not yet compiled or run.
#include "net/dns/dns_resolution_family_rust_test_support.h"
#include "net/dns/host_resolver_manager_job.h"
#include "net/dns/host_resolver_manager_request_impl.h"
#include "net/dns/host_resolver_manager_service_endpoint_request_impl.h"
#include "net/dns/dns_resolution_original_host_resolver_manager_job.h"
#include "net/dns/dns_resolution_original_host_resolver_manager_request_impl.h"
#include "net/dns/dns_resolution_original_host_resolver_manager_service_endpoint_request_impl.h"
#include "base/functional/bind.h"
#include "base/test/task_environment.h"
#include "net/test/test_with_task_environment.h"
#include "net/base/mock_network_change_notifier.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {
// Access only the coherent post-start, job-attached boundary. Public full Start
// paths remain tested separately. These tests use actual controller objects,
// native ownership and callbacks, not a second policy model or old DLL.
class DnsResolutionFamilyRustPeer {
 public:
  template<class Api>using Job=typename Api::Manager::Job;
  template<class Api>using Request=typename Api::Manager::RequestImpl;
  template<class Api>using Endpoint=typename Api::Manager::ServiceEndpointRequestImpl;
  template<class Api>static std::unique_ptr<Job<Api>> MakeJob(
      typename Api::Manager& manager,typename Api::Context& context,
      const base::TickClock* tick){
    typename Api::Manager::JobKey key(typename Api::Host(typename Api::Pair("attached.test",443)),handles::kInvalidNetworkHandle,&context);
    key.network_anonymization_key=NetworkAnonymizationKey();key.query_types={DnsQueryType::A};
    key.flags=0;key.source=HostResolverSource::ANY;key.secure_dns_mode=SecureDnsMode::kOff;
    return std::make_unique<Job<Api>>(manager.weak_ptr_factory_.GetWeakPtr(),std::move(key),
        HostResolver::ResolveHostParameters::CacheUsage::DISALLOWED,context.host_cache(),
        std::deque<typename Api::Manager::TaskType>{},DEFAULT_PRIORITY,NetLogWithSource(),tick,
        HostResolver::HttpsSvcbOptions());
  }
  template<class Api>static std::unique_ptr<Request<Api>> MakeRequest(
      typename Api::Manager& manager,typename Api::Context& context,
      const base::TickClock* tick,Job<Api>& job,RequestPriority priority,
      CompletionOnceCallback callback){
    HostResolver::ResolveHostParameters params;params.initial_priority=priority;params.dns_query_type=DnsQueryType::A;
    params.cache_usage=HostResolver::ResolveHostParameters::CacheUsage::DISALLOWED;
    auto request=std::make_unique<Request<Api>>(NetLogWithSource(),typename Api::Host(typename Api::Pair("attached.test",443)),
        NetworkAnonymizationKey(),handles::kInvalidNetworkHandle,params,context.GetWeakPtr(),
        manager.weak_ptr_factory_.GetWeakPtr(),tick);
    request->job_key_=job.key();request->next_state_=Request<Api>::STATE_NONE;
    request->callback_=std::move(callback);request->request_time_=tick->NowTicks();
    request->resolver_.reset();job.AddRequest(request.get());return request;
  }
  template<class Api>static std::unique_ptr<Endpoint<Api>> MakeEndpoint(
      typename Api::Manager& manager,typename Api::Context& context,
      const base::TickClock* tick,Job<Api>& job,
      typename Endpoint<Api>::Delegate* delegate){
    HostResolver::ResolveHostParameters params;params.initial_priority=LOW;params.dns_query_type=DnsQueryType::A;
    params.cache_usage=HostResolver::ResolveHostParameters::CacheUsage::DISALLOWED;
    auto request=std::make_unique<Endpoint<Api>>(typename Api::Host(typename Api::Pair("attached.test",443)),
        NetworkAnonymizationKey(),handles::kInvalidNetworkHandle,NetLogWithSource(),params,
        context.GetWeakPtr(),manager.weak_ptr_factory_.GetWeakPtr(),tick);
    request->job_key_=job.key();request->next_state_=Endpoint<Api>::State::kNone;
    request->delegate_=delegate;job.AddServiceEndpointRequest(request.get());return request;
  }
  template<class Api>static int Priority(const Job<Api>& job){return job.priority_tracker_.highest_priority();}
  template<class Api>static size_t Count(const Job<Api>& job){return job.priority_tracker_.total_count();}
};
}

namespace net::dns_resolution_family_test {
using Peer=::net::DnsResolutionFamilyRustPeer;
template<class Api>struct Attached {
  Attached(){
    tick.SetNowTicks(base::TimeTicks()+base::Seconds(10000));wall.SetNow(base::Time::FromSecondsSinceUnixEpoch(100000));
    HostResolver::ManagerOptions options;options.insecure_dns_mode=InsecureDnsMode::kDisabled;
    manager=std::make_unique<typename Api::Manager>(options,nullptr,nullptr);manager->SetTickClockForTesting(&tick);
    context=std::make_unique<typename Api::Context>(nullptr,true,wall,tick);manager->RegisterResolveContext(context.get());
    job=Peer::MakeJob<Api>(*manager,*context,&tick);
  }
  ~Attached(){job.reset();if(manager)manager->DeregisterResolveContext(context.get());context.reset();manager.reset();}
  base::SimpleTestTickClock tick;base::SimpleTestClock wall;
  std::unique_ptr<typename Api::Manager> manager;std::unique_ptr<typename Api::Context> context;
  std::unique_ptr<Peer::Job<Api>> job;
};
template<class Api>std::vector<int> PriorityAndAbort(){
  Attached<Api> f;std::vector<int> callbacks;
  auto first=Peer::MakeRequest<Api>(*f.manager,*f.context,&f.tick,*f.job,LOW,base::BindOnce([&](int rv){callbacks.push_back(rv);}));
  auto second=Peer::MakeRequest<Api>(*f.manager,*f.context,&f.tick,*f.job,MEDIUM,base::BindOnce([&](int rv){callbacks.push_back(rv);}));
  std::vector<int> state={Peer::Priority<Api>(*f.job),static_cast<int>(Peer::Count<Api>(*f.job))};
  first->ChangeRequestPriority(HIGHEST);state.push_back(Peer::Priority<Api>(*f.job));
  first.reset();state.push_back(Peer::Priority<Api>(*f.job));state.push_back(static_cast<int>(Peer::Count<Api>(*f.job)));
  f.job->Abort();state.push_back(static_cast<int>(callbacks.size()));state.push_back(second->GetResolveErrorInfo().error);
  state.insert(state.end(),callbacks.begin(),callbacks.end());return state;
}
TEST(DnsResolutionFamilyRust, NativeAttachedPriorityAndAbort){
  NetTaskEnvironment env(base::test::TaskEnvironment::MainThreadType::IO, base::test::TaskEnvironment::TimeSource::MOCK_TIME);test::ScopedMockNetworkChangeNotifier notifier;
  const auto original=PriorityAndAbort<Original>();const auto actual=PriorityAndAbort<Production>();
  EXPECT_EQ(original,actual);EXPECT_EQ(actual,(std::vector<int>{MEDIUM,2,HIGHEST,MEDIUM,1,1,ERR_NETWORK_CHANGED,ERR_NAME_NOT_RESOLVED}));
}
template<class Api>std::vector<int> LastCancellation(){
  Attached<Api> f;int callbacks=0;
  auto request=Peer::MakeRequest<Api>(*f.manager,*f.context,&f.tick,*f.job,LOW,base::BindOnce([&](int){++callbacks;}));
  request.reset();return {callbacks,static_cast<int>(Peer::Count<Api>(*f.job)),Peer::Priority<Api>(*f.job)};
}
TEST(DnsResolutionFamilyRust, NativeLastCancellationSuppressesUserCallback){
  NetTaskEnvironment env(base::test::TaskEnvironment::MainThreadType::IO, base::test::TaskEnvironment::TimeSource::MOCK_TIME);test::ScopedMockNetworkChangeNotifier notifier;
  EXPECT_EQ(LastCancellation<Original>(),LastCancellation<Production>());
  EXPECT_EQ(LastCancellation<Production>(),(std::vector<int>{0,0,MINIMUM_PRIORITY}));
}
template<class Api>std::vector<int> EndpointCancellation(){
  struct Delegate:Peer::Endpoint<Api>::Delegate{int updates=0,finished=0;
    void OnServiceEndpointsUpdated()override{++updates;}void OnServiceEndpointRequestFinished(int)override{++finished;}}delegate;
  Attached<Api> f;auto request=Peer::MakeEndpoint<Api>(*f.manager,*f.context,&f.tick,*f.job,&delegate);
  request->ChangeRequestPriority(HIGHEST);const int before=Peer::Priority<Api>(*f.job);request.reset();
  return {before,delegate.updates,delegate.finished,static_cast<int>(Peer::Count<Api>(*f.job))};
}
TEST(DnsResolutionFamilyRust, NativeEndpointCancellationClearsDelegate){
  NetTaskEnvironment env(base::test::TaskEnvironment::MainThreadType::IO, base::test::TaskEnvironment::TimeSource::MOCK_TIME);test::ScopedMockNetworkChangeNotifier notifier;
  EXPECT_EQ(EndpointCancellation<Original>(),EndpointCancellation<Production>());
  EXPECT_EQ(EndpointCancellation<Production>(),(std::vector<int>{HIGHEST,0,0,0}));
}
template<class Api>std::vector<int> ManagerDeletedByCallback(){
  Attached<Api> f;int first_calls=0,second_calls=0;
  auto first=Peer::MakeRequest<Api>(*f.manager,*f.context,&f.tick,*f.job,LOW,base::BindOnce([&](int){++first_calls;f.manager.reset();}));
  auto second=Peer::MakeRequest<Api>(*f.manager,*f.context,&f.tick,*f.job,LOW,base::BindOnce([&](int){++second_calls;}));
  f.job->Abort();f.job.reset();return {first_calls,second_calls,second->HasJob()};
}
TEST(DnsResolutionFamilyRust, NativeCompletionStopsAfterManagerDeletion){
  NetTaskEnvironment env(base::test::TaskEnvironment::MainThreadType::IO, base::test::TaskEnvironment::TimeSource::MOCK_TIME);test::ScopedMockNetworkChangeNotifier notifier;
  EXPECT_EQ(ManagerDeletedByCallback<Original>(),ManagerDeletedByCallback<Production>());
  EXPECT_EQ(ManagerDeletedByCallback<Production>(),(std::vector<int>{1,0,0}));
}
}
