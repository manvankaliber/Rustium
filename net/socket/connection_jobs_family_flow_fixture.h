// Source-only full-family ordinary fixture. Not compiled, executed or measured.
#ifndef NET_SOCKET_CONNECTION_JOBS_FAMILY_FLOW_FIXTURE_H_
#define NET_SOCKET_CONNECTION_JOBS_FAMILY_FLOW_FIXTURE_H_
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>
#include <vector>
#include "base/check_op.h"
#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "base/test/task_environment.h"
#include "net/socket/connection_jobs_upper_rust_job_workloads.h"
#include "net/socket/client_socket_handle.h"
#include "net/socket/client_socket_handle_rust_original.h"
#include "net/socket/client_socket_pool.h"
#include "net/socket/client_socket_pool_rust_original.h"
#include "net/socket/connect_job_factory.h"
#include "net/socket/connect_job_factory_rust_original.h"
#include "net/socket/stream_socket_handle_rust_original.h"

namespace net::connection_jobs_family_flow {
using ScriptCase=connection_jobs_job_workloads::Case;
struct Production:connection_jobs_job_workloads::Production {
 using Pool=ClientSocketPool;using Handle=ClientSocketHandle;
 using Factory=ConnectJobFactory;using Higher=HigherLayeredPool;
 using Capacity=SocketPoolAdditionalCapacity;
};
struct Original:connection_jobs_job_workloads::Original {
 using Pool=ClientSocketPoolRustOriginal;using Handle=ClientSocketHandleRustOriginal;
 using Factory=ConnectJobFactoryRustOriginal;using Higher=HigherLayeredPoolRustOriginal;
 using Capacity=SocketPoolAdditionalCapacityRustOriginal;
};
struct Event {
 int kind;int64_t a=0,b=0;
 bool operator==(const Event&)const=default;
};
struct Trace {
  Trace();
  ~Trace();
  Trace(const Trace&);
  Trace(Trace&&) noexcept;
  Trace& operator=(const Trace&);
  Trace& operator=(Trace&&) noexcept;

 uint64_t checksum=0;std::vector<Event> events;
 void Add(int kind,int64_t a=0,int64_t b=0);
};
// The abstract-pool provider is shared test scaffolding, not an implementation
// of a production derived pool. It drives the real common Pool->Factory->Params
// ->job graph, and the real Handle/StreamHandle publication/reset policy.
template<class Types>
class FlowPool final:public Types::Pool,public Types::Base::Delegate {
 public:
 using BasePool=typename Types::Pool;using Job=typename Types::Base;
 using Handle=typename Types::Handle;using Group=typename BasePool::GroupId;
 using Params=typename BasePool::SocketParams;
 using Limits=typename BasePool::RespectLimits;
 using Auth=typename BasePool::ProxyAuthCallback;
 using Preconnect=typename BasePool::PreconnectCompletionCallback;
 using Higher=typename Types::Higher;
 using Capacity=typename Types::Capacity;
 FlowPool(const typename Types::Common* common,const ProxyChain& chain,bool websocket,Trace& trace)
  :BasePool(256,chain,websocket,common,std::make_unique<typename Types::Factory>()),trace_(trace) {
  this->SetAdditionalCapacityForTest(Capacity::CreateEmpty());
 }
 ~FlowPool()override{CHECK(!pending_handle_);CHECK(!job_);}
 int RequestSocket(const Group& group,scoped_refptr<Params> params,
   const std::optional<NetworkTrafficAnnotationTag>& annotation,RequestPriority priority,
   const SocketTag& tag,Limits,Handle* handle,CompletionOnceCallback callback,
   const Auth& auth,const NetLogWithSource&)override {
  CHECK(!job_);CHECK(!pending_handle_);
  this->UpdateExpandabilityBeforeAllocation();in_use_=1;
  pending_handle_=handle;auth_=auth;
  trace_->Add(1,priority);
  job_=this->CreateConnectJob(group,std::move(params),annotation,priority,tag,this);
  CHECK(job_);
  const int rv=job_->Connect();trace_->Add(2,rv);
  if(rv==ERR_IO_PENDING){callback_=std::move(callback);return rv;}
  Publish(rv);job_.reset();return rv;
 }
 int RequestSockets(const Group&,scoped_refptr<Params>,
   const std::optional<NetworkTrafficAnnotationTag>&,size_t,Preconnect,
   const NetLogWithSource&)override{return ERR_NOT_IMPLEMENTED;}
 void SetPriority(const Group&,Handle*,RequestPriority priority)override {
  trace_->Add(3,priority);if(job_)job_->ChangePriority(priority);
 }
 void CancelRequest(const Group&,Handle* handle,bool cancel_job)override {
  CHECK_EQ(handle,pending_handle_);trace_->Add(4,cancel_job);
  pending_handle_=nullptr;callback_.Reset();auth_.Reset();job_.reset();
  in_use_=0;this->UpdateExpandabilityAfterRelease();
 }
 void ReleaseSocket(const Group&,std::unique_ptr<StreamSocket> socket,int64_t generation)override {
  trace_->Add(5,generation,socket!=nullptr);socket.reset();
  in_use_=0;this->UpdateExpandabilityAfterRelease();
 }
 void FlushWithError(int,const char*)override{CHECK(false)<<"Not a queueing pool fixture";}
 void CloseIdleSockets(const char*)override{}
 void CloseIdleSocketsInGroup(const Group&,const char*)override{trace_->Add(6);}
 size_t IdleSocketCount()const override{return 0;}
 size_t IdleSocketCountInGroup(const Group&)const override{return 0;}
 LoadState GetLoadState(const Group&,const Handle*)const override {
  return job_?job_->GetLoadState():LOAD_STATE_IDLE;
 }
 base::Value GetInfoAsValue(const std::string&,const std::string&)const override{return base::Value();}
 bool HasActiveSocket(const Group&)const override{return in_use_!=0;}
 bool IsStalled()const override{return false;}
 void AddHigherLayeredPool(Higher*)override{trace_->Add(7);}
 void RemoveHigherLayeredPool(Higher*)override{trace_->Add(8);}
 size_t SocketsInUse()const override{return in_use_;}
 void OnConnectJobComplete(int result,Job* job)override {
  CHECK_EQ(job,job_.get());trace_->Add(9,result);
  auto callback=std::move(callback_);Publish(result);job_.reset();
  // This terminal callback may delete/reset the handle or the pool. No member
  // is touched afterward.
  std::move(callback).Run(result);
 }
 void OnNeedsProxyAuth(const HttpResponseInfo& response,HttpAuthController* controller,
   base::OnceClosure restart,Job* job)override {
  CHECK_EQ(job,job_.get());trace_->Add(10);
  auto auth=auth_;auth.Run(response,controller,std::move(restart));
 }
 private:
 void Publish(int result) {
  auto* handle=std::exchange(pending_handle_,nullptr).get();CHECK(handle);CHECK(job_);
  handle->set_connect_timing(job_->connect_timing());
  handle->set_resolution_details(job_->GetResolutionDetails());
  if(result!=OK)handle->SetAdditionalErrorState(job_.get());
  auto socket=job_->PassSocket();
  if(socket){handle->SetSocket(std::move(socket));handle->set_group_generation(19);}
  else {in_use_=0;this->UpdateExpandabilityAfterRelease();}
  trace_->Add(11,result,handle->socket()!=nullptr);
 }
 raw_ref<Trace> trace_;size_t in_use_=0;raw_ptr<Handle> pending_handle_=nullptr;
 std::unique_ptr<Job> job_;CompletionOnceCallback callback_;Auth auth_;
};

struct Route {
 Route();
 ~Route();
 Route(const Route&);
 Route(Route&&);
 Route& operator=(const Route&);
 Route& operator=(Route&&);
 ProxyChain proxy=ProxyChain::Direct();
 url::SchemeHostPort endpoint{"https","www.endpoint.test",443};
 bool websocket=false;
};
inline Route RouteFor(ScriptCase kind) {
 Route route;
 if(kind>=ScriptCase::kSocks4&&kind<=ScriptCase::kSocksError) {
  const bool v5=kind!=ScriptCase::kSocks4;
  route.proxy=ProxyChain(v5?ProxyServer::SCHEME_SOCKS5:ProxyServer::SCHEME_SOCKS4,
    HostPortPair("proxy.test",443));
  route.endpoint=url::SchemeHostPort("http",v5?kSOCKS5TestHost:kSOCKS4TestHost,
    v5?kSOCKS5TestPort:kSOCKS4TestPort);
 } else if(kind>=ScriptCase::kHttpDirect) {
  const bool secure=kind==ScriptCase::kHttpsDirect||kind==ScriptCase::kHttpsTunnel||
                    kind==ScriptCase::kHttp2Pending;
  const bool quic=kind==ScriptCase::kQuicPending||kind==ScriptCase::kQuicError;
  route.proxy=ProxyChain(quic?ProxyServer::SCHEME_QUIC:
    secure?ProxyServer::SCHEME_HTTPS:ProxyServer::SCHEME_HTTP,HostPortPair("proxy.test",443));
  route.endpoint=url::SchemeHostPort("http","www.endpoint.test",443);
  route.websocket=kind!=ScriptCase::kHttpDirect&&kind!=ScriptCase::kHttpsDirect;
 }
 return route;
}
// Script/session/provider preparation remains outside any future timing
// boundary. This function runs only ordinary correctness fixtures now.
template<class Types>
Trace Run(ScriptCase kind,base::test::TaskEnvironment& environment,
          bool close_pending=false,bool reset_on_completion=false,
          bool delete_on_completion=false) {
 connection_jobs_job_workloads::Fixture<Types> scripted(kind,environment);
 scripted.Prepare(1);
 const Route route=RouteFor(kind);
 Trace trace;
 {
  FlowPool<Types> pool(&*scripted.common,route.proxy,route.websocket,trace);
  using Pool=typename Types::Pool;using Handle=typename Types::Handle;
  const typename Pool::GroupId group(route.endpoint,PRIVACY_MODE_DISABLED,
    NetworkAnonymizationKey(),SecureDnsPolicy::kAllow,false,handles::kInvalidNetworkHandle);
  auto handle=std::make_unique<Handle>();
  const auto log=NetLogWithSource::Make(NetLog::Get(),NetLogSourceType::SOCKET);
  std::optional<int> completed;
  auto callback=base::BindOnce([](Trace* trace,std::unique_ptr<Handle>* handle,
      std::optional<int>* completed,bool reset,bool delete_handle,int result) {
   CHECK(!completed->has_value());*completed=result;
   trace->Add(12,result,(*handle)->is_initialized());
   if(delete_handle){handle->reset();}
   else if(reset){(*handle)->Reset();}
  },&trace,&handle,&completed,reset_on_completion,delete_on_completion);
  typename Pool::ProxyAuthCallback auth=base::BindRepeating(
   [](Trace* trace,const HttpResponseInfo&,HttpAuthController*,base::OnceClosure) {
    trace->Add(13);
   },&trace);
  const int immediate=handle->Init(group,base::MakeRefCounted<typename Pool::SocketParams>(
    std::vector<SSLConfig::CertAndStatus>{}),TRAFFIC_ANNOTATION_FOR_TESTS,DEFAULT_PRIORITY,
    SocketTag(),Pool::RespectLimits::ENABLED,std::move(callback),auth,&pool,log);
  trace.Add(14,immediate,handle->is_initialized());
  if(close_pending&&immediate==ERR_IO_PENDING) {
   handle->ResetAndCloseSocket();environment.RunUntilIdle();trace.Add(15);
   CHECK(!completed);
  } else {
   environment.RunUntilIdle();
   if(kind==ScriptCase::kTlsTimeout)environment.FastForwardBy(base::Seconds(30));
   const bool pending=kind==ScriptCase::kHttpAuth||kind==ScriptCase::kHttp2Pending||
                      kind==ScriptCase::kQuicPending;
   const int expected=kind==ScriptCase::kTlsError||kind==ScriptCase::kQuicError?ERR_FAILED:
    kind==ScriptCase::kTlsTransportError?ERR_CONNECTION_FAILED:
    kind==ScriptCase::kTlsTimeout?ERR_TIMED_OUT:
    kind==ScriptCase::kSocksError?ERR_UNEXPECTED:pending?ERR_IO_PENDING:OK;
   CHECK_EQ(completed.value_or(immediate),expected);
   if(handle){
    CHECK_EQ(handle->is_initialized(),expected==OK&&!reset_on_completion);
    trace.Add(16,handle->is_initialized(),handle->socket()!=nullptr);
    trace.Add(17,handle->resolve_error_info().error,handle->connection_attempts().size());
    if(handle->is_initialized()) {
     LoadTimingInfo timing;
     trace.Add(18,handle->GetLoadTimingInfo(false,&timing),
       handle->connect_timing().ssl_start.is_null());
     handle->CloseIdleSocketsInGroup("family ordinary fixture");
    }
    handle->Reset();environment.RunUntilIdle();
   }else{
    // Only test-owned state is read after the terminal completion deleted the
    // handle. Real handle/pool release already completed inside its destructor.
    CHECK(delete_on_completion);CHECK(completed.has_value());CHECK_EQ(*completed,OK);
    trace.Add(19);
   }
   if(kind!=ScriptCase::kHttp2Pending&&kind!=ScriptCase::kQuicPending&&kind!=ScriptCase::kQuicError) {
    CHECK(scripted.scripts.front()->data->AllReadDataConsumed());
    CHECK(scripted.scripts.front()->data->AllWriteDataConsumed());
   }
  }
  if(handle){CHECK(!handle->is_initialized());CHECK(!handle->socket());handle.reset();}
  else{CHECK(delete_on_completion);}
 }
 environment.RunUntilIdle();
 return trace;
}
}
#endif
