// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license in LICENSE.
// PREPARED ONLY: parent must compile/rebuild and execute these ordinary tests.
#include "net/http/connection_auth_original_policy.h"
#include "net/http/connection_auth_original_mock.h"
#include "net/http/connection_auth_original_basic.h"
#include "net/http/http_auth_handler_basic.h"
#include "net/http/connection_header_original_policy.h"
#include "net/log/connection_original_log.h"

#include <type_traits>
#include <vector>
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/simple_test_tick_clock.h"
#include "base/test/task_environment.h"
#include "net/base/features.h"
#include "net/http/http_auth_cache_rust_original.h"
#include "net/http/http_request_info.h"
#include "net/log/test_net_log.h"
#include "net/socket/connection_jobs_upper_rust_job_workloads.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {
namespace {
using Auth = ConnectionAuthOriginalPolicy;
using Cache = ConnectionAuthOriginalCache;
using Headers = ConnectionHeaderOriginalPolicy;
static_assert(sizeof(ConnectionAuthOriginalRegistry)==sizeof(HttpAuthHandlerRegistryFactory));
static_assert(sizeof(ConnectionAuthOriginalBasic)==sizeof(HttpAuthHandlerBasic));
constexpr auto kProxy = HttpAuth::AUTH_PROXY;
constexpr auto kBasic = HttpAuth::AUTH_SCHEME_BASIC;
const NetworkAnonymizationKey kNak;

auto Challenge(const char* value) {
  return HttpResponseHeaders::TryToCreate(std::string(
      "HTTP/1.1 407 Proxy Authentication Required\r\nProxy-Authenticate: ") +
      value + "\r\nContent-Length: 0\r\n\r\n");
}

class ConnectionAuthBaselineTest : public testing::Test {
 protected:
  ConnectionAuthBaselineTest() {
    features.InitAndDisableFeature(net::features::kHappyEyeballsV2);
    request.method = "CONNECT";
    request.url = GURL("https://www.endpoint.test:443/");
  }
  auto Controller(HttpAuthHandlerFactory* provider = nullptr) {
    return base::MakeRefCounted<HttpAuthController>(kProxy, origin.GetURL(), kNak,
        &cache, provider ? provider : factory.get(), &resolver);
  }
  base::test::ScopedFeatureList features;
  base::test::TaskEnvironment environment;
  HttpAuthCache cache{true};
  MockHostResolver resolver;
  std::unique_ptr<ConnectionAuthOriginalRegistry> factory =
      ConnectionAuthOriginalRegistry::Create(nullptr);
  const url::SchemeHostPort origin{GURL("http://proxy.test/")};
  HttpRequestInfo request;
};

TEST_F(ConnectionAuthBaselineTest, NegativeLookupChallengeResetPreemptiveAndRejection) {
  auto auth = Controller();
  EXPECT_EQ(OK, Auth::MaybeGenerateAuthToken(auth.get(), &request, SSLInfo(),
                                           base::DoNothing(), NetLogWithSource()));
  EXPECT_EQ(0u, Cache::GetEntriesSizeForTesting(&cache));
  EXPECT_FALSE(Cache::LookupByPath(&cache, origin, kProxy, kNak, "/"));
  EXPECT_EQ(OK, Auth::HandleAuthChallenge(auth.get(), Challenge("Basic realm=\"r\""),
                                        SSLInfo(), false, true, NetLogWithSource()));
  std::optional<AuthChallengeInfo> info;
  Auth::TakeAuthInfo(auth.get(), &info);
  ASSERT_TRUE(info);
  EXPECT_EQ("r", info->realm);
  EXPECT_FALSE(Auth::HaveAuth(auth.get()));
  Auth::ResetAuth(auth.get(), AuthCredentials(u"u", u"p"));
  auto* entry = Cache::Lookup(&cache, origin, kProxy, "r", kBasic, kNak);
  ASSERT_TRUE(entry);
  EXPECT_TRUE(entry->credentials().Equals(AuthCredentials(u"u", u"p")));
  EXPECT_FALSE(Cache::Lookup(&cache, origin, kProxy, "R", kBasic, kNak));
  EXPECT_EQ(OK, Auth::MaybeGenerateAuthToken(auth.get(), &request, SSLInfo(),
                                           base::DoNothing(), NetLogWithSource()));
  HttpRequestHeaders first, consumed;
  Headers::AddAuthorizationHeader(*auth, &first);
  Headers::AddAuthorizationHeader(*auth, &consumed);
  EXPECT_EQ(Headers::GetHeader(first, "proxy-authorization"), "Basic dTpw");
  EXPECT_TRUE(consumed.IsEmpty());
  auto preemptive = Controller();
  EXPECT_EQ(OK, Auth::MaybeGenerateAuthToken(preemptive.get(), &request, SSLInfo(),
                                           base::DoNothing(), NetLogWithSource()));
  EXPECT_TRUE(Auth::HaveAuth(preemptive.get()));
  EXPECT_FALSE(preemptive->has_handled_challenge());
  HttpRequestHeaders second;
  Headers::AddAuthorizationHeader(*preemptive, &second);
  EXPECT_EQ(first.GetHeaderVector(), second.GetHeaderVector());
  EXPECT_EQ(OK, Auth::HandleAuthChallenge(preemptive.get(), Challenge("Basic realm=\"r\""),
                                        SSLInfo(), false, true, NetLogWithSource()));
  EXPECT_FALSE(Cache::Lookup(&cache, origin, kProxy, "r", kBasic, kNak));
  EXPECT_FALSE(Auth::HaveAuth(preemptive.get()));
}

TEST_F(ConnectionAuthBaselineTest, NativeCacheMatchesFrozenPathsNonceCopyAndPartition) {
  HttpAuthCacheOriginal frozen(true);
  const url::SchemeHostPort server(GURL("https://server.test/"));
  const auto key_a=NetworkAnonymizationKey::CreateTransient();
  const auto key_b=NetworkAnonymizationKey::CreateTransient();
  Cache::Add(&cache,server,HttpAuth::AUTH_SERVER,"partitioned",kBasic,key_a,
             "Basic",AuthCredentials(),"/partition/");
  EXPECT_TRUE(Cache::Lookup(&cache,server,HttpAuth::AUTH_SERVER,"partitioned",kBasic,key_a));
  EXPECT_FALSE(Cache::Lookup(&cache,server,HttpAuth::AUTH_SERVER,"partitioned",kBasic,key_b));
  for (const auto* path : {"/a/b/file", "/a/c/file", "/a/file", "/z/file"}) {
    auto* a=Cache::Add(&cache, server, HttpAuth::AUTH_SERVER, "r", kBasic, kNak,
                       "Basic realm=r", AuthCredentials(u"u", u"p"), path);
    auto* b=frozen.Add(server, HttpAuth::AUTH_SERVER, "r", kBasic, kNak,
                       "Basic realm=r", AuthCredentials(u"u", u"p"), path);
    EXPECT_EQ(Cache::IncrementNonceCount(a), b->IncrementNonceCount());
  }
  for (const auto* path : {"/a/b/test", "/a/c/test", "/a/test", "/z/test", "/absent"}) {
    auto* a=Cache::LookupByPath(&cache, server, HttpAuth::AUTH_SERVER,kNak,path);
    auto* b=frozen.LookupByPath(server,HttpAuth::AUTH_SERVER,kNak,path);
    EXPECT_EQ(a!=nullptr,b!=nullptr);
    if (a && b) EXPECT_EQ(a->auth_challenge(),b->auth_challenge());
  }
  Cache::Add(&cache,origin,kProxy,"p",kBasic,kNak,"Basic realm=p",AuthCredentials(),"/");
  EXPECT_EQ(Cache::Lookup(&cache,origin,kProxy,"p",kBasic,key_a),
            Cache::Lookup(&cache,origin,kProxy,"p",kBasic,key_b));
  HttpAuthCache copied(false);
  Cache::CopyProxyEntriesFrom(&copied,cache);
  EXPECT_EQ(1u,Cache::GetEntriesSizeForTesting(&copied));
  auto* p=Cache::Lookup(&copied,origin,kProxy,"p",kBasic,kNak);
  ASSERT_TRUE(p);
  EXPECT_EQ(2,Cache::IncrementNonceCount(p));
  EXPECT_TRUE(Cache::UpdateStaleChallenge(&copied,origin,kProxy,"p",kBasic,kNak,"Basic realm=p2"));
  EXPECT_EQ(2,Cache::IncrementNonceCount(p));
  Cache::SetKeyServerEntriesByNetworkAnonymizationKey(&cache,false);
  EXPECT_EQ(1u,Cache::GetEntriesSizeForTesting(&cache));
  EXPECT_TRUE(Cache::ClearEntriesAddedBetween(&cache,base::Time::Min(),base::Time::Max(),{}));
  EXPECT_FALSE(Cache::ClearAllEntries(&cache));
}

TEST_F(ConnectionAuthBaselineTest, CacheLruEvictionAndCredentialMismatchRemoval) {
  base::SimpleTestTickClock clock;
  clock.SetNowTicks(base::TimeTicks()+base::Seconds(1));
  cache.set_tick_clock_for_testing(&clock);
  for (int i=0;i<=HttpAuthCache::kMaxNumRealmEntries;++i) {
    clock.Advance(base::Seconds(1));
    Cache::Add(&cache,origin,kProxy,base::NumberToString(i),kBasic,kNak,
               "Basic",AuthCredentials(u"u",u"p"),"/");
  }
  EXPECT_EQ(static_cast<size_t>(HttpAuthCache::kMaxNumRealmEntries),Cache::GetEntriesSizeForTesting(&cache));
  EXPECT_FALSE(Cache::Lookup(&cache,origin,kProxy,"0",kBasic,kNak));
  EXPECT_FALSE(Cache::Remove(&cache,origin,kProxy,"1",kBasic,kNak,AuthCredentials(u"wrong",u"p")));
  EXPECT_TRUE(Cache::Lookup(&cache,origin,kProxy,"1",kBasic,kNak));
  EXPECT_TRUE(Cache::Remove(&cache,origin,kProxy,"1",kBasic,kNak,AuthCredentials(u"u",u"p")));
}

TEST_F(ConnectionAuthBaselineTest, OriginalTokenizerFactoryValidInvalidAndLongSchemes) {
  for (const std::string value : std::vector<std::string>{"Basic", " bAsIc\trealm=\"one\",realm=\"two\" ",
                                 "Basic realm=\"unterminated", "", std::string(257,'X')+" abc==="}) {
    std::unique_ptr<HttpAuthHandler> a,b;
    auto production=HttpAuthHandlerFactory::CreateDefault();
    int original=ConnectionAuthOriginalFactoryPolicy::CreateAuthHandlerFromString(
        factory.get(),value,kProxy,SSLInfo(),kNak,origin,NetLogWithSource(),nullptr,&a);
    int migrated=production->CreateAuthHandlerFromString(
        value,kProxy,SSLInfo(),kNak,origin,NetLogWithSource(),nullptr,&b);
    EXPECT_EQ(original,migrated) << value;
    EXPECT_EQ(a!=nullptr,b!=nullptr);
    if (a && b) EXPECT_EQ(a->realm(),b->realm());
  }
}

TEST_F(ConnectionAuthBaselineTest, AsyncTokenCallbackCanDeleteControllerAndCancellationDropsTask) {
  for (bool cancel : {false,true}) {
    ConnectionAuthOriginalMock::Factory provider;
    provider.set_do_init_from_challenge(true);
    auto handler=std::make_unique<ConnectionAuthOriginalMock>();
    handler->set_allows_default_credentials(true);
    handler->SetGenerateExpectation(true,OK);
    provider.AddMockHandler(std::move(handler),kProxy);
    auto auth=Controller(&provider);
    EXPECT_EQ(OK,Auth::HandleAuthChallenge(auth.get(),Challenge("Mock"),SSLInfo(),false,true,NetLogWithSource()));
    unsigned callbacks=0;
    EXPECT_EQ(ERR_IO_PENDING,Auth::MaybeGenerateAuthToken(auth.get(),&request,SSLInfo(),
      base::BindOnce([&](int rv) {
        EXPECT_EQ(OK,rv);++callbacks;
        HttpRequestHeaders token;Headers::AddAuthorizationHeader(*auth,&token);
        EXPECT_EQ(Headers::GetHeader(token,"proxy-authorization"),"auth_token");
        auth.reset(); // no post-callback access to controller or handler
      }),NetLogWithSource()));
    if(cancel)auth.reset();
    environment.RunUntilIdle();
    EXPECT_EQ(cancel?0u:1u,callbacks);
    EXPECT_FALSE(auth);
  }
}

TEST_F(ConnectionAuthBaselineTest, LoggingIsLazyAndCaptureUsesFrozenResponseAndEventValues) {
  const NetLogWithSource disabled;
  // A dereference would fail: the original callback must stay unobserved.
  ConnectionOriginalNetLogResponseHeaders(disabled,NetLogEventType::HTTP_TRANSACTION_READ_TUNNEL_RESPONSE_HEADERS,nullptr);
  ConnectionOriginalNetLogRequestHeaders(disabled,NetLogEventType::HTTP_TRANSACTION_SEND_TUNNEL_HEADERS,"",nullptr);
  for (auto mode : {NetLogCaptureMode::kDefault,NetLogCaptureMode::kIncludeSensitive}) {
    RecordingNetLogObserver capture(mode);
    auto log=NetLogWithSource::Make(NetLog::Get(),NetLogSourceType::HTTP_AUTH_CONTROLLER);
    auto headers=Challenge("Negotiate abc==");
    ConnectionOriginalNetLogResponseHeaders(log,NetLogEventType::HTTP_TRANSACTION_READ_TUNNEL_RESPONSE_HEADERS,headers.get());
    auto entries=capture.GetEntriesForSource(log.source());
    ASSERT_EQ(1u,entries.size());
    EXPECT_EQ(entries[0].params,headers->NetLogParams(mode));
    capture.Clear();
    ConnectionOriginalLog::AddEventWithInt64Params(log,NetLogEventType::AUTH_HANDLER_CREATE_RESULT,"large",INT64_MAX);
    entries=capture.GetEntriesForSource(log.source());
    ASSERT_EQ(1u,entries.size());
    EXPECT_EQ(entries[0].params,net_log_values_rust_reference::NetLogParamsWithInt64("large",INT64_MAX));
    auto auth=Controller();
    EXPECT_EQ(OK,Auth::HandleAuthChallenge(auth.get(),Challenge("Basic realm=r"),SSLInfo(),false,true,log));
    EXPECT_FALSE(capture.GetEntriesWithType(NetLogEventType::AUTH_HANDLER_CREATE_RESULT).empty());
  }
}

using namespace connection_jobs_job_workloads;
template<class Types> class RestartingDelegate final : public Types::Base::Delegate {
 public:
  void OnConnectJobComplete(int rv,typename Types::Base*) override {result=rv;++completed;}
  void OnNeedsProxyAuth(const HttpResponseInfo&,HttpAuthController* auth,base::OnceClosure restart,typename Types::Base*) override {
    ++challenged;
    if constexpr(std::is_same_v<Types,Original>) Auth::ResetAuth(auth,AuthCredentials(u"u",u"p"));
    else auth->ResetAuth(AuthCredentials(u"u",u"p"));
    std::move(restart).Run(); // preserve normal posted/reentrant restart policy
  }
  std::optional<int> result;unsigned completed=0,challenged=0;
};
template<class Types> void AuthenticatedTunnel(base::test::TaskEnvironment& environment) {
  Fixture<Types> fixture(Case::kHttpAuth,environment);
  const char* first="CONNECT www.endpoint.test:443 HTTP/1.1\r\nHost: www.endpoint.test:443\r\nProxy-Connection: keep-alive\r\nUser-Agent: test-ua\r\n\r\n";
  const char* second="CONNECT www.endpoint.test:443 HTTP/1.1\r\nHost: www.endpoint.test:443\r\nProxy-Connection: keep-alive\r\nUser-Agent: test-ua\r\nProxy-Authorization: Basic dTpw\r\n\r\n";
  auto script=std::make_unique<Script>();
  script->writes.emplace_back(ASYNC,0,first);
  script->reads.emplace_back(ASYNC,1,"HTTP/1.1 407 Proxy Authentication Required\r\nProxy-Authenticate: Basic realm=r\r\nContent-Length: 0\r\n\r\n");
  script->writes.emplace_back(ASYNC,2,second);
  script->reads.emplace_back(ASYNC,3,"HTTP/1.1 200 Connection Established\r\n\r\n");
  script->data=std::make_unique<SequencedSocketData>(script->reads,script->writes);
  script->data->set_connect_data(MockConnect(SYNCHRONOUS,OK));
  fixture.deps.socket_factory->AddSocketDataProvider(script->data.get());
  fixture.scripts.push_back(std::move(script));
  RestartingDelegate<Types> delegate;
  auto job=std::make_unique<typename Types::Proxy>(DEFAULT_PRIORITY,SocketTag(),&*fixture.common,fixture.proxy_params,&delegate,nullptr);
  EXPECT_EQ(ERR_IO_PENDING,job->Connect());environment.RunUntilIdle();
  EXPECT_EQ(delegate.result,OK);EXPECT_EQ(1u,delegate.challenged);EXPECT_EQ(1u,delegate.completed);
  EXPECT_TRUE(fixture.scripts[0]->data->AllWriteDataConsumed());
  EXPECT_TRUE(fixture.scripts[0]->data->AllReadDataConsumed());
  job.reset();environment.RunUntilIdle();
}
TEST_F(ConnectionAuthBaselineTest, FullJobDelegateCredentialsRestartAndWireOrder) {
  AuthenticatedTunnel<Original>(environment);
  AuthenticatedTunnel<Production>(environment);
}
}  // namespace
}  // namespace net
