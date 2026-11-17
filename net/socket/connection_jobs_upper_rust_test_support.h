// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_SOCKET_CONNECTION_JOBS_UPPER_RUST_TEST_SUPPORT_H_
#define NET_SOCKET_CONNECTION_JOBS_UPPER_RUST_TEST_SUPPORT_H_
#include <memory>
#include "base/no_destructor.h"
#include <utility>
#include <vector>
#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "base/check_op.h"
#include "base/functional/callback_helpers.h"
#include "base/test/task_environment.h"
#include "net/socket/connect_job_rust_test_support.h"
#include "net/socket/client_socket_handle.h"
#include "net/socket/client_socket_handle_rust_original.h"
#include "net/socket/client_socket_pool.h"
#include "net/socket/client_socket_pool_rust_original.h"
#include "net/socket/connect_job_factory.h"
#include "net/socket/connect_job_factory_rust_original.h"
#include "net/socket/stream_socket_handle_rust_original.h"
#include "net/base/schemeful_site.h"
#include "url/gurl.h"

namespace net::connection_jobs_upper_test {
struct Production {
  using Pool=ClientSocketPool;
  using Handle=ClientSocketHandle;
  using Stream=StreamSocketHandle;
  using Capacity=SocketPoolAdditionalCapacity;
  using Expand=SocketPoolExpandability;
  using Higher=HigherLayeredPool;
};
struct Original {
  using Pool=ClientSocketPoolRustOriginal;
  using Handle=ClientSocketHandleRustOriginal;
  using Stream=StreamSocketHandleRustOriginal;
  using Capacity=SocketPoolAdditionalCapacityRustOriginal;
  using Expand=SocketPoolExpandabilityRustOriginal;
  using Higher=HigherLayeredPoolRustOriginal;
};
using Trace=connect_job_rust_test::Trace;
using Mark=connect_job_rust_test::Mark;
class FakeSocket final:public StreamSocket{
 public:
  FakeSocket(Trace&trace,int id);
  ~FakeSocket()override;
  int Read(IOBuffer*,int,CompletionOnceCallback)override;
  int Write(IOBuffer*,int,CompletionOnceCallback,const NetworkTrafficAnnotationTag&)override;
  int SetReceiveBufferSize(int32_t)override;
  int SetSendBufferSize(int32_t)override;
  int Connect(CompletionOnceCallback)override;
  void Disconnect()override;
  bool IsConnected()const override;
  bool IsConnectedAndIdle()const override;
  int GetPeerAddress(IPEndPoint*)const override;
  int GetLocalAddress(IPEndPoint*)const override;
  const NetLogWithSource& NetLog()const override;
  bool WasEverUsed()const override;
  NextProto GetNegotiatedProtocol()const override;
  bool GetSSLInfo(SSLInfo*)override;
  int64_t GetTotalReceivedBytes()const override;
  void ApplySocketTag(const SocketTag&)override;
  raw_ref<Trace> trace;int id;NetLogWithSource log;
};
inline const ProxyChain& DirectFixture(){
  static const base::NoDestructor<ProxyChain> direct(ProxyChain::Direct());
  return *direct;
}

// Native provider interface for the handle. It contains no old handle/pool
// algorithm and is independently instantiated per implementation.
template<class Types>
class Pool final:public Types::Pool {
 public:
  using Base=typename Types::Pool;
  using Group=typename Base::GroupId;
  using Params=typename Base::SocketParams;
  using Limits=typename Base::RespectLimits;
  using Auth=typename Base::ProxyAuthCallback;
  using Preconnect=typename Base::PreconnectCompletionCallback;
  using Handle=typename Types::Handle;
  using Higher=typename Types::Higher;
  explicit Pool(Trace& trace):Base(256,DirectFixture(),false,nullptr,nullptr),trace(trace){}
  int RequestSocket(const Group&,scoped_refptr<Params>,
      const std::optional<NetworkTrafficAnnotationTag>&,RequestPriority,
      const SocketTag&,Limits,Handle* handle,CompletionOnceCallback callback,
      const Auth&,const NetLogWithSource&)override{
    trace->Add(Mark::kReturn,101,result);
    if(result==ERR_IO_PENDING){pending_handle=handle;pending=std::move(callback);return result;}
    if(give_socket){handle->SetSocket(std::make_unique<FakeSocket>(*trace,1));
      handle->set_group_generation(19);}
    return result;
  }
  int RequestSockets(const Group&,scoped_refptr<Params>,
      const std::optional<NetworkTrafficAnnotationTag>&,size_t,Preconnect,
      const NetLogWithSource&)override{return ERR_UNEXPECTED;}
  void SetPriority(const Group&,Handle*,RequestPriority priority)override{
    trace->Add(Mark::kPriority,priority);
  }
  void CancelRequest(const Group&,Handle*,bool cancel_job)override{
    trace->Add(Mark::kReturn,102,cancel_job);pending.Reset();pending_handle=nullptr;
  }
  void ReleaseSocket(const Group&,std::unique_ptr<StreamSocket> socket,
      int64_t generation)override{
    trace->Add(Mark::kReturn,103,generation);socket.reset();
  }
  void FlushWithError(int,const char*)override{}
  void CloseIdleSockets(const char*)override{}
  void CloseIdleSocketsInGroup(const Group&,const char*)override{
    trace->Add(Mark::kReturn,104);
  }
  size_t IdleSocketCount()const override{return 0;}
  size_t IdleSocketCountInGroup(const Group&)const override{return 0;}
  LoadState GetLoadState(const Group&,const Handle*)const override{return LOAD_STATE_CONNECTING;}
  base::Value GetInfoAsValue(const std::string&,const std::string&)const override{return base::Value();}
  bool HasActiveSocket(const Group&)const override{return false;}
  bool IsStalled()const override{return true;}
  void AddHigherLayeredPool(Higher*)override{trace->Add(Mark::kReturn,105);}
  void RemoveHigherLayeredPool(Higher*)override{trace->Add(Mark::kReturn,106);}
  size_t SocketsInUse()const override{return in_use;}
  void Finish(int rv){
    if(give_socket){pending_handle->SetSocket(std::make_unique<FakeSocket>(*trace,1));
      pending_handle->set_group_generation(19);}
    auto callback=std::move(pending);pending_handle=nullptr;
    std::move(callback).Run(rv);
  }
  void Allocate(){this->UpdateExpandabilityBeforeAllocation();}
  void Release(){this->UpdateExpandabilityAfterRelease();}
  raw_ref<Trace> trace;
  int result=OK;
  bool give_socket=true;
  size_t in_use=0;
  raw_ptr<Handle> pending_handle=nullptr;
  CompletionOnceCallback pending;
};
template<class Types>
class Higher final:public Types::Higher {
 public: bool CloseOneIdleConnection()override{return false;}
};
template<class Types>
class Stream final:public Types::Stream {
 public:
  void Reset()override{this->SetSocket(nullptr);}
  bool IsPoolStalled()const override{return false;}
};

enum class Case:unsigned{
 kHandleSync,kHandleAsync,kHandleCancel,kHandleClose,kHandleRecoverable,
 kHandleFatal,kHandleReentry,kHandleDelete,kHandlePriority,kHandleHigher,
 kGroupPlain,kGroupPrefixes,kGroupPartitioned,kIdleTimeout,kExpandBoundaries,
 kStreamEmpty,kStreamFresh,kStreamReused,kStreamTransfer,kCapacityInvalid,
 kCapacityProbability,
};
inline constexpr std::array<const char*,21> kNames={
 "handle_sync","handle_async","handle_cancel","handle_close","handle_recoverable",
 "handle_fatal","handle_reentry","handle_delete","handle_priority","handle_higher",
 "group_plain","group_prefixes","group_partitioned","idle_timeout","expand_boundaries",
 "stream_empty","stream_fresh","stream_reused","stream_transfer","capacity_invalid","capacity_probability"};

template<class Types>
uint64_t Run(Case kind,bool detailed=false){
  Trace trace(detailed);
  using Base=typename Types::Pool;
  using Group=typename Base::GroupId;
  using Capacity=typename Types::Capacity;
  const auto group=Group(url::SchemeHostPort("https","host.test",443),
    PRIVACY_MODE_DISABLED,NetworkAnonymizationKey(),SecureDnsPolicy::kAllow,false,
    handles::kInvalidNetworkHandle);
  if(kind>=Case::kGroupPlain&&kind<=Case::kGroupPartitioned){
    const bool extra=kind!=Case::kGroupPlain;
    const auto input=Group(url::SchemeHostPort("https","host.test",443),
      extra?PRIVACY_MODE_ENABLED_WITHOUT_CLIENT_CERTS:PRIVACY_MODE_DISABLED,
      kind==Case::kGroupPartitioned?NetworkAnonymizationKey::CreateSameSite(
        SchemefulSite(GURL("https://context.test"))):NetworkAnonymizationKey(),
      extra?SecureDnsPolicy::kBootstrap:SecureDnsPolicy::kAllow,
      extra,extra?handles::NetworkHandle{23}:handles::kInvalidNetworkHandle);
    const auto text=input.ToString();
    if(detailed&&extra){CHECK(text.starts_with("disable_cert_network_fetches/dns_bootstrap/pmwocc/target_network=23/"));}
    for(unsigned char ch:text){trace.Add(Mark::kReturn,ch);}
    return trace.checksum;
  }
  if(kind==Case::kIdleTimeout){
    const auto old=Base::used_idle_socket_timeout();
    Base::set_used_idle_socket_timeout(base::Seconds(7)+base::Milliseconds(999));
    trace.Add(Mark::kReturn,Base::used_idle_socket_timeout().InMicroseconds());
    if(detailed){CHECK_EQ(Base::used_idle_socket_timeout(),base::Seconds(7));}
    Base::set_used_idle_socket_timeout(old);return trace.checksum;
  }
  if(kind==Case::kExpandBoundaries||kind==Case::kCapacityInvalid||kind==Case::kCapacityProbability){
    if(kind==Case::kCapacityProbability){
      // Probability is exactly1 with base1/noise0, so decisions/checksums are
      // deterministic while the real RNG/pow/telemetry path still executes.
      const auto capacity=Capacity::CreateForTest(1.0,16,0.0,0.0);
      trace.Add(Mark::kReturn,static_cast<int>(capacity.NextExpandabilityBeforeAllocation(
        Types::Expand::kUncapped,8,0)));
      trace.Add(Mark::kReturn,static_cast<int>(capacity.NextExpandabilityAfterRelease(
        Types::Expand::kCapped,8,0)));
      return trace.checksum;
    }
    if(kind==Case::kCapacityInvalid){
      const auto invalid=Capacity::CreateForTest(-0.1,2,0.3,0.4);
      trace.Add(Mark::kReturn,invalid.capacity(),invalid==Capacity::CreateEmpty());
      if(detailed){CHECK_EQ(invalid.capacity(),0u);CHECK(invalid==Capacity::CreateEmpty());}
    }else{
      // This workload deliberately covers nonrandom hard boundaries separately.
      const auto capacity=Capacity::CreateForTest(0.1,16,0.3,0.4);
      trace.Add(Mark::kReturn,static_cast<int>(capacity.NextExpandabilityBeforeAllocation(
        Types::Expand::kUncapped,15,16)));
      trace.Add(Mark::kReturn,static_cast<int>(capacity.NextExpandabilityAfterRelease(
        Types::Expand::kCapped,32,16)));
      trace.Add(Mark::kReturn,static_cast<int>(capacity.NextExpandabilityBeforeAllocation(
        Types::Expand::kUncapped,65536,16)));
    }return trace.checksum;
  }
  if(kind>=Case::kStreamEmpty&&kind<=Case::kStreamTransfer){
    Stream<Types> stream;
    LoadTimingInfo timing;
    timing.socket_log_id=999;
    if(kind!=Case::kStreamEmpty){
      stream.SetSocket(std::make_unique<FakeSocket>(trace,1));
      LoadTimingInfo::ConnectTiming connect;
      connect.connect_start=base::TimeTicks()+base::Seconds(1);
      connect.connect_end=base::TimeTicks()+base::Seconds(2);
      stream.set_connect_timing(connect);
    }
    if(kind==Case::kStreamTransfer){
      auto socket=stream.PassSocket();
      trace.Add(Mark::kReturn,socket!=nullptr,stream.socket()==nullptr);
      socket.reset();
    }else{
      const auto& constant=stream;
      const bool ready=constant.GetLoadTimingInfo(kind==Case::kStreamReused,&timing);
      if(detailed){CHECK_EQ(ready,kind!=Case::kStreamEmpty);
        CHECK_EQ(timing.socket_reused,kind==Case::kStreamReused);
        CHECK_EQ(timing.connect_timing.connect_start.is_null(),kind!=Case::kStreamFresh);}
      trace.Add(Mark::kReturn,ready,timing.socket_log_id!=999);
      trace.Add(Mark::kReturn,timing.socket_reused,timing.connect_timing.connect_start.is_null());
    }stream.Reset();return trace.checksum;
  }
  Pool<Types> pool(trace);
  auto handle=std::make_unique<typename Types::Handle>();
  if(kind==Case::kHandleAsync||kind==Case::kHandleCancel||kind==Case::kHandleDelete||
     kind==Case::kHandlePriority||kind==Case::kHandleReentry){pool.result=ERR_IO_PENDING;}
  if(kind==Case::kHandleRecoverable){pool.result=ERR_CERT_DATE_INVALID;}
  if(kind==Case::kHandleFatal){pool.result=ERR_CONNECTION_FAILED;pool.give_socket=false;}
  unsigned completions=0;
  CompletionOnceCallback callback=base::BindOnce([](Trace* trace,unsigned* completions,
      std::unique_ptr<typename Types::Handle>* handle,Case kind,int rv){
    ++*completions;
    trace->Add(Mark::kComplete,rv,(*handle)->is_initialized());
    if(kind==Case::kHandleDelete){handle->reset();}
    else if(kind==Case::kHandleReentry){(*handle)->Reset();}
  },&trace,&completions,&handle,kind);
  const auto log=NetLogWithSource::Make(NetLog::Get(),NetLogSourceType::SOCKET);
  const auto rv=handle->Init(group,Base::SocketParams::CreateForHttpForTesting(),
    std::nullopt,DEFAULT_PRIORITY,SocketTag(),Base::RespectLimits::ENABLED,
    std::move(callback),typename Base::ProxyAuthCallback(),&pool,log);
  trace.Add(Mark::kReturn,rv,handle->is_initialized());
  if(detailed){CHECK_EQ(rv,pool.result);CHECK_EQ(handle->is_initialized(),rv!=ERR_IO_PENDING&&pool.give_socket);}
  if(kind==Case::kHandlePriority){handle->SetPriority(HIGHEST);}
  if(rv==ERR_IO_PENDING&&kind!=Case::kHandleCancel){pool.Finish(OK);}
  if(detailed){CHECK_EQ(completions,rv==ERR_IO_PENDING&&kind!=Case::kHandleCancel?1u:0u);}
  Higher<Types> higher;
  if(kind==Case::kHandleHigher){handle->AddHigherLayeredPool(&higher);}
  if(handle){
    if(kind==Case::kHandleClose){handle->ResetAndCloseSocket();}
    else if(kind==Case::kHandleCancel){handle->ResetAndCloseSocket();}
    else{handle->Reset();}
    trace.Add(Mark::kReturn,handle->is_initialized(),handle->socket()!=nullptr);
    if(detailed){CHECK(!handle->is_initialized());CHECK(!handle->socket());}
  }
  handle.reset();
  return trace.checksum;
}
static_assert(sizeof(ClientSocketHandle)==sizeof(ClientSocketHandleRustOriginal));
static_assert(alignof(ClientSocketHandle)==alignof(ClientSocketHandleRustOriginal));
static_assert(sizeof(StreamSocketHandle)==sizeof(StreamSocketHandleRustOriginal));
static_assert(sizeof(ClientSocketPool)==sizeof(ClientSocketPoolRustOriginal));
static_assert(sizeof(SocketPoolAdditionalCapacity)==sizeof(SocketPoolAdditionalCapacityRustOriginal));
}  // namespace net::connection_jobs_upper_test
#endif  // NET_SOCKET_CONNECTION_JOBS_UPPER_RUST_TEST_SUPPORT_H_
