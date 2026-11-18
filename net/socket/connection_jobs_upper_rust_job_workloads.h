#include "net/http/connection_auth_original_policy.h"
// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_SOCKET_CONNECTION_JOBS_UPPER_RUST_JOB_WORKLOADS_H_
#define NET_SOCKET_CONNECTION_JOBS_UPPER_RUST_JOB_WORKLOADS_H_
#include <array>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include "base/check_op.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/raw_ref.h"
#include "base/test/task_environment.h"
#include "net/base/net_errors.h"
#include "net/base/network_anonymization_key.h"
#include "net/dns/mock_host_resolver.h"
#include "net/http/http_network_session.h"
#include "net/http/http_proxy_connect_job.h"
#include "net/http/http_proxy_connect_job_rust_original.h"
#include "net/net_buildflags.h"
#include "net/quic/quic_context.h"
#include "net/quic/quic_session_pool.h"
#include "net/socket/connection_jobs_upper_rust_original_helpers.h"
#include "net/socket/connect_job_params.h"
#include "net/socket/connection_jobs_http2_rust_original_context.h"
#include "net/socket/connect_job_params_rust_original.h"
#include "net/socket/socks_connect_job.h"
#include "net/socket/socks_connect_job_rust_original.h"
#include "net/socket/ssl_connect_job.h"
#include "net/socket/ssl_connect_job_rust_original.h"
#include "net/socket/transport_connect_job.h"
#include "net/socket/transport_connect_job_rust_original.h"
#include "net/socket/socket_test_util.h"
#include "net/spdy/spdy_session_pool.h"
#include "net/spdy/spdy_test_util_common.h"
#include "net/ssl/ssl_config_service.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/static_http_user_agent_settings.h"

namespace net::connection_jobs_job_workloads {
enum class Case:unsigned {
 kTlsSync,kTlsAsync,kTlsError,kTlsTransportError,kTlsTimeout,
 kSocks4,kSocks5,kSocksError,kHttpDirect,kHttpTunnel,
 kHttpAuth,kHttpsDirect,kHttpsTunnel,kHttp2Pending,kQuicPending,kQuicError,
};
inline constexpr std::array<const char*,16> kNames={
 "tls_sync","tls_async","tls_error","tls_transport_error","tls_timeout",
 "socks4","socks5","socks_error","http_direct","http_tunnel",
 "http_auth","https_direct","https_tunnel","http2_pending","quic_pending","quic_error"};
inline bool Supported(Case kind){
#if !BUILDFLAG(ENABLE_QUIC_PROXY_SUPPORT)
 return kind!=Case::kQuicPending&&kind!=Case::kQuicError;
#else
 return true;
#endif
}
struct Production{
 static void PrepareAuth(SpdySessionDependencies&) {}
 using Base=ConnectJob;using Common=CommonConnectJobParams;
 using Transport=TransportSocketParams;using Params=ConnectJobParams;
 using TlsParams=SSLSocketParams;using Tls=SSLConnectJob;
 using SocksParams=SOCKSSocketParams;using Socks=SOCKSConnectJob;
 using ProxyParams=HttpProxySocketParams;using Proxy=HttpProxyConnectJob;
 using ResolveCallback=OnHostResolutionCallback;
 using Http2Context=std::unique_ptr<ConnectionJobsHttp2RustOriginalContext>;
 static Http2Context CreateHttp2Context(HttpNetworkSession&){return {};}
 static Common CommonFrom(const CommonConnectJobParams& input,const Http2Context&){return input;}
 static void CloseHttp2(HttpNetworkSession& session,const Http2Context&){session.spdy_session_pool()->CloseAllSessions();}
};
struct Original{
 static void PrepareAuth(SpdySessionDependencies& deps) {
  deps.http_auth_handler_factory=ConnectionAuthOriginalRegistry::Create(nullptr);
 }
 using Base=ConnectJobRustOriginal;using Common=CommonConnectJobParamsRustOriginal;
 using Transport=TransportSocketParamsRustOriginal;using Params=ConnectJobParamsRustOriginal;
 using TlsParams=SSLSocketParamsRustOriginal;using Tls=SSLConnectJobRustOriginal;
 using SocksParams=SOCKSSocketParamsRustOriginal;using Socks=SOCKSConnectJobRustOriginal;
 using ProxyParams=HttpProxySocketParamsRustOriginal;using Proxy=HttpProxyConnectJobRustOriginal;
 using ResolveCallback=ConnectJobRustOriginalHostResolutionCallback;
 using Http2Context=std::unique_ptr<ConnectionJobsHttp2RustOriginalContext>;
 static Http2Context CreateHttp2Context(HttpNetworkSession& session){
  // Match SpdyCreateSession's explicit no-initial-data fixture policy.
  return std::make_unique<ConnectionJobsHttp2RustOriginalContext>(session,false);}
 static Common CommonFrom(const CommonConnectJobParams& input,const Http2Context& context){
  return ConnectionJobsUpperRustOriginalStorage::CommonParams(input,context->pool());}
 static void CloseHttp2(HttpNetworkSession&,const Http2Context& context){context->pool()->CloseAllSessions();}
};
template<class Types>class Delegate final:public Types::Base::Delegate{
 public:
  void OnConnectJobComplete(int rv,typename Types::Base*)override{result=rv;++completions;}
  void OnNeedsProxyAuth(const HttpResponseInfo&,HttpAuthController*,
      base::OnceClosure callback,typename Types::Base*)override{
    ++auth_challenges;restart=std::move(callback);
  }
  std::optional<int> result;unsigned completions=0,auth_challenges=0;
  base::OnceClosure restart;
};
#if BUILDFLAG(ENABLE_QUIC_PROXY_SUPPORT)
// Request/cancel provider only; it does not claim to emulate a successful QUIC
// session or replace any connection-job algorithm. No packets or sockets.
class QuicProvider final:public QuicSessionPool{
 public:
  ~QuicProvider() override;
  explicit QuicProvider(HttpNetworkSession& session);
  void CancelRequest(QuicSessionRequest*)override;
  int RequestSession(const QuicSessionKey&,url::SchemeHostPort,
      quic::ParsedQuicVersion version,const std::optional<NetworkTrafficAnnotationTag>,
      MultiplexedSessionCreationInitiator,std::optional<ConnectionManagementConfig>,
      const HttpUserAgentSettings*,RequestPriority,bool,int,const GURL&,
      const NetLogWithSource&,QuicSessionRequest*)override;
  int result=ERR_IO_PENDING;unsigned requests=0,cancels=0;
  quic::ParsedQuicVersion last_version=quic::ParsedQuicVersion::Unsupported();
};
#endif
struct Script{
  Script();
  ~Script();

 std::vector<MockRead> reads;std::vector<MockWrite>writes;
 std::unique_ptr<SequencedSocketData> data;
 std::unique_ptr<SSLSocketDataProvider> ssl;
 std::optional<spdy::SpdySerializedFrame> h2_request,h2_reset;
};

template<class Types>class Fixture{
 public:
  using Base=typename Types::Base;
  explicit Fixture(Case kind,base::test::TaskEnvironment& environment)
    :kind(kind),environment(environment){
    deps.host_resolver=std::make_unique<MockHostResolver>(
      MockHostResolverBase::RuleResolver::GetLocalhostResult());
    deps.http_user_agent_settings=std::make_unique<StaticHttpUserAgentSettings>("*","test-ua");
    Types::PrepareAuth(deps);
    session=SpdySessionDependencies::SpdyCreateSession(&deps);
    http2_context=Types::CreateHttp2Context(*session);
    common.emplace(Types::CommonFrom(session->CreateCommonConnectJobParams(),http2_context));
    deps.host_resolver->set_synchronous_mode(kind!=Case::kTlsAsync);
#if BUILDFLAG(ENABLE_QUIC_PROXY_SUPPORT)
    if(kind==Case::kQuicPending||kind==Case::kQuicError){
      quic=std::make_unique<QuicProvider>(*session);quic->result=kind==Case::kQuicError?ERR_FAILED:ERR_IO_PENDING;
      common->quic_session_pool=quic.get();
    }
#endif
    const auto transport=[&](const HostPortPair& host){
      return base::MakeRefCounted<typename Types::Transport>(host,NetworkAnonymizationKey(),
        SecureDnsPolicy::kAllow,handles::kInvalidNetworkHandle,typename Types::ResolveCallback(),
        base::flat_set<std::string>{});};
    const HostPortPair endpoint("www.endpoint.test",443),proxy_host("proxy.test",443);
    if(kind<=Case::kTlsTimeout){
      tls_params=base::MakeRefCounted<typename Types::TlsParams>(
        typename Types::Params(transport(endpoint)),endpoint,SSLConfig(),NetworkAnonymizationKey());
    }else if(kind<=Case::kSocksError){
      const bool v5=kind!=Case::kSocks4;
      socks_params=base::MakeRefCounted<typename Types::SocksParams>(
        typename Types::Params(transport(proxy_host)),v5,
        HostPortPair(v5?kSOCKS5TestHost:kSOCKS4TestHost,v5?kSOCKS5TestPort:kSOCKS4TestPort),
        NetworkAnonymizationKey(),TRAFFIC_ANNOTATION_FOR_TESTS);
    }else{
      const bool secure=kind==Case::kHttpsDirect||kind==Case::kHttpsTunnel||kind==Case::kHttp2Pending;
      const bool is_quic=kind==Case::kQuicPending||kind==Case::kQuicError;
      const ProxyChain chain(ProxyServer(is_quic?ProxyServer::SCHEME_QUIC:
        secure?ProxyServer::SCHEME_HTTPS:ProxyServer::SCHEME_HTTP,proxy_host));
      if(is_quic){
        SSLConfig config;config.disable_cert_verification_network_fetches=true;
        proxy_params=base::MakeRefCounted<typename Types::ProxyParams>(config,endpoint,chain,0,true,
          TRAFFIC_ANNOTATION_FOR_TESTS,NetworkAnonymizationKey(),SecureDnsPolicy::kAllow,handles::kInvalidNetworkHandle);
      }else{
        auto nested=typename Types::Params(transport(proxy_host));
        if(secure){nested=typename Types::Params(base::MakeRefCounted<typename Types::TlsParams>(
          std::move(nested),proxy_host,SSLConfig(),NetworkAnonymizationKey()));}
        proxy_params=base::MakeRefCounted<typename Types::ProxyParams>(std::move(nested),endpoint,chain,0,
          kind!=Case::kHttpDirect&&kind!=Case::kHttpsDirect,TRAFFIC_ANNOTATION_FOR_TESTS,
          NetworkAnonymizationKey(),SecureDnsPolicy::kAllow,handles::kInvalidNetworkHandle);
      }
    }
  }
  bool Supported()const{
#if !BUILDFLAG(ENABLE_QUIC_PROXY_SUPPORT)
    return kind!=Case::kQuicPending&&kind!=Case::kQuicError;
#else
    return true;
#endif
  }
  ~Fixture(){
    Types::CloseHttp2(*session,http2_context);environment->RunUntilIdle();
#if BUILDFLAG(ENABLE_QUIC_PROXY_SUPPORT)
    if(quic){common->quic_session_pool=nullptr;quic.reset();}
#endif
    common.reset();http2_context.reset();session.reset();deps.socket_factory.reset();scripts.clear();
  }
  void Prepare(unsigned iterations){
    // Called before clocks: independent scripted provider instances per whole
    // lifecycle. No mutable provider reset or input construction is timed.
    Types::CloseHttp2(*session,http2_context);environment->RunUntilIdle();
    scripts.clear();deps.socket_factory->ResetNextMockIndexes();
    // ResetNextMockIndexes reuses registered pointers, so each Fixture is used
    // for one Prepare/Batch only. The perftest constructs a fresh Fixture for
    // each sample side and destroys it after stopping the clock.
    CHECK(!prepared);prepared=true;
    for(unsigned i=0;i<iterations;++i){
      auto script=std::make_unique<Script>();const auto io=kind==Case::kTlsAsync?ASYNC:SYNCHRONOUS;
      if(kind==Case::kSocks4){script->writes.emplace_back(SYNCHRONOUS,0,kSOCKS4OkRequestLocalHostPort80);
        script->reads.emplace_back(SYNCHRONOUS,1,kSOCKS4OkReply);}
      if(kind==Case::kSocks5){script->writes.emplace_back(SYNCHRONOUS,0,kSOCKS5GreetRequest);
        script->reads.emplace_back(SYNCHRONOUS,1,kSOCKS5GreetResponse);
        script->writes.emplace_back(SYNCHRONOUS,2,kSOCKS5OkRequest);
        script->reads.emplace_back(SYNCHRONOUS,3,kSOCKS5OkResponse);}
      if(kind==Case::kSocksError){script->writes.emplace_back(SYNCHRONOUS,ERR_UNEXPECTED,0);}
      if(kind==Case::kHttpTunnel||kind==Case::kHttpAuth||kind==Case::kHttpsTunnel){
        script->writes.emplace_back(SYNCHRONOUS,0,
          "CONNECT www.endpoint.test:443 HTTP/1.1\r\nHost: www.endpoint.test:443\r\nProxy-Connection: keep-alive\r\nUser-Agent: test-ua\r\n\r\n");
        script->reads.emplace_back(SYNCHRONOUS,1,std::string_view(kind==Case::kHttpAuth?
          "HTTP/1.1 407 Proxy Authentication Required\r\nProxy-Authenticate: Basic realm=\"r\"\r\nContent-Length: 0\r\n\r\n":
          "HTTP/1.1 200 Connection Established\r\n\r\n"));
      }
      if(kind==Case::kHttp2Pending){
        SpdyTestUtil util;const std::string_view headers[]={"user-agent","test-ua"};
        script->h2_request.emplace(util.ConstructSpdyConnect(headers,1,DEFAULT_PRIORITY,HostPortPair("www.endpoint.test",443)));
        script->h2_reset.emplace(util.ConstructSpdyRstStream(1,spdy::ERROR_CODE_CANCEL));
        // H2 starts reading immediately. Keep the read pending without
        // pausing the subsequent CONNECT and reset writes.
        script->reads.emplace_back(SYNCHRONOUS,ERR_IO_PENDING,0);
        script->writes.push_back(CreateMockWrite(*script->h2_request,1));
        script->writes.push_back(CreateMockWrite(*script->h2_reset,2));
      }
      script->data=std::make_unique<SequencedSocketData>(script->reads,script->writes);
      script->data->set_connect_data(MockConnect(io,kind==Case::kTlsTransportError?ERR_CONNECTION_FAILED:OK));
      deps.socket_factory->AddSocketDataProvider(script->data.get());
      if(kind<=Case::kTlsTimeout||kind==Case::kHttpsDirect||kind==Case::kHttpsTunnel||kind==Case::kHttp2Pending){
        script->ssl=std::make_unique<SSLSocketDataProvider>(io,kind==Case::kTlsError?ERR_FAILED:
          kind==Case::kTlsTimeout?ERR_IO_PENDING:OK);
        if(kind==Case::kHttp2Pending){script->ssl->next_proto=NextProto::kProtoHTTP2;}
        deps.socket_factory->AddSSLSocketDataProvider(script->ssl.get());
      }
      scripts.push_back(std::move(script));
    }
  }
  uint64_t Run(unsigned iterations,bool verify=false){
    uint64_t sum=0;
    for(unsigned i=0;i<iterations;++i){
      Delegate<Types> delegate;std::unique_ptr<Base> job;
      if(tls_params){job=std::make_unique<typename Types::Tls>(DEFAULT_PRIORITY,SocketTag(),&*common,tls_params,&delegate,nullptr);}
      else if(socks_params){job=std::make_unique<typename Types::Socks>(DEFAULT_PRIORITY,SocketTag(),&*common,socks_params,&delegate,nullptr);}
      else{job=std::make_unique<typename Types::Proxy>(DEFAULT_PRIORITY,SocketTag(),&*common,proxy_params,&delegate,nullptr);}
      const int immediate=job->Connect();environment->RunUntilIdle();
      if(kind==Case::kTlsTimeout){environment->FastForwardBy(base::Seconds(30));}
      const int result=delegate.result.value_or(immediate);
      sum=sum*131+static_cast<uint64_t>(result+10000)+delegate.completions+delegate.auth_challenges;
      sum=sum*131+(job->socket()!=nullptr)+job->IsSSLError();
      sum=sum*131+static_cast<uint64_t>(job->GetResolveErrorInfo().error+10000);
      const auto& timing=job->connect_timing();
      sum=sum*131+(!timing.connect_start.is_null())+2*(!timing.connect_end.is_null())+4*(!timing.ssl_start.is_null());
      if(verify){
        const bool pending=kind==Case::kHttpAuth||kind==Case::kHttp2Pending||kind==Case::kQuicPending;
        const int expected=kind==Case::kTlsError||kind==Case::kQuicError?ERR_FAILED:
          kind==Case::kTlsTransportError?ERR_CONNECTION_FAILED:kind==Case::kTlsTimeout?ERR_TIMED_OUT:
          kind==Case::kSocksError?ERR_UNEXPECTED:pending?ERR_IO_PENDING:OK;
        CHECK_EQ(expected,result);CHECK_EQ(delegate.auth_challenges,kind==Case::kHttpAuth?1u:0u);
      }
      job.reset();environment->RunUntilIdle();
      if(kind==Case::kHttp2Pending){Types::CloseHttp2(*session,http2_context);environment->RunUntilIdle();}
      if(verify&&kind!=Case::kHttp2Pending&&kind!=Case::kQuicPending&&kind!=Case::kQuicError){
        CHECK(scripts.at(i)->data->AllReadDataConsumed());CHECK(scripts.at(i)->data->AllWriteDataConsumed());
      }
    }
#if BUILDFLAG(ENABLE_QUIC_PROXY_SUPPORT)
    if(quic){sum=sum*131+quic->requests;sum=sum*131+quic->cancels;}
#endif
    return sum;
  }
  Case kind;raw_ref<base::test::TaskEnvironment> environment;
  SpdySessionDependencies deps;std::unique_ptr<HttpNetworkSession> session;
  typename Types::Http2Context http2_context;
  std::optional<typename Types::Common> common;
  scoped_refptr<typename Types::TlsParams> tls_params;
  scoped_refptr<typename Types::SocksParams> socks_params;
  scoped_refptr<typename Types::ProxyParams> proxy_params;
  std::vector<std::unique_ptr<Script>> scripts;bool prepared=false;
#if BUILDFLAG(ENABLE_QUIC_PROXY_SUPPORT)
  std::unique_ptr<QuicProvider> quic;
#endif
};
}  // namespace net::connection_jobs_job_workloads
#endif  // NET_SOCKET_CONNECTION_JOBS_UPPER_RUST_JOB_WORKLOADS_H_
