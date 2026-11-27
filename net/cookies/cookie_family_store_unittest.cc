// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
// Authored ordinary tests; no compiler/native execution in the draft phase.
#include "base/test/bind.h"
#include "net/cookies/cookie_family_store_test_support.h"
#include <sstream>
#include "testing/gtest/include/gtest/gtest.h"
namespace net::cookie_family_store_test {
TEST(CookieFamilyStoreTest, FifteenCompleteStoreLifecyclesMatchOriginal) {
  base::test::TaskEnvironment env(base::test::TaskEnvironment::TimeSource::MOCK_TIME);
  const Inputs input{base::Time::Now()};
  for(const auto& scenario:kCases){SCOPED_TRACE(scenario.id);
    const auto original=Run<Original>(scenario,input,env);
    const auto production=Run<Production>(scenario,input,env);
    EXPECT_EQ(production,original);
  }
}
template<class Family> std::vector<std::string> ValidationEdges(base::Time now) {
  struct Input {const char* name;const char* value;const char* domain;const char* path;bool secure;bool http;};
  const Input cases[]={
    {"n","value","example.test","/",false,false},
    {"n\x01","value","example.test","/",false,false},
    {"n","value\n","example.test","/",false,false},
    {"n","value","EXAMPLE.TEST","/",false,false},
    {"n","value","example.test","relative",false,false},
    {"__Secure-n","value","example.test","/",false,false},
    {"__Secure-n","value","example.test","/",true,false},
    {"__Host-n","value",".example.test","/",true,false},
    {"__Http-n","value","example.test","/",true,false},
    {"__Host-Http-n","value","example.test","/",true,true},
    {"","__Secure-hidden","example.test","/",true,true},
    {"","a=b","example.test","/",false,false},
  };
  std::vector<std::string> out;
  for(const auto& input:cases){
    auto c=Family::Cookie::CreateUnsafeCookieForTesting(input.name,input.value,input.domain,input.path,now,
      now+base::Days(1),now,now,input.secure,input.http,Family::SameSite::LAX_MODE,
      static_cast<typename Family::Priority>(1),Family::SourceType::kOther,std::nullopt,
      static_cast<typename Family::SourceScheme>(input.secure?2:1),input.secure?443:80);
    const auto storage=c->IsCanonicalForFromStorage();const auto canonical=c->IsCanonical();
    std::ostringstream text;text<<storage;
    out.push_back(std::to_string(static_cast<bool>(storage))+":"+std::to_string(static_cast<bool>(canonical))+":"+text.str());
    out.push_back(std::to_string(c->IsPersistent())+std::to_string(c->IsExpired(now))+
      std::to_string(c->IsExpired(now+base::Days(1)))+std::to_string(c->IsExpired(now+base::Days(2))));
  }
  // Null creation with non-null last access is an independently frozen failure.
  auto c=Family::Cookie::CreateUnsafeCookieForTesting("n","v","example.test","/",base::Time(),base::Time(),now,now,
    false,false,Family::SameSite::LAX_MODE,static_cast<typename Family::Priority>(1),Family::SourceType::kOther);
  std::ostringstream text;text<<c->IsCanonicalForFromStorage();out.push_back(text.str());return out;
}
TEST(CookieFamilyStoreTest, CompleteStorageValidationAndExactFailureText) {
  const auto now=base::Time::FromInternalValue(13380163200000000LL);
  EXPECT_EQ(ValidationEdges<Production>(now),ValidationEdges<Original>(now));
}
template<class Family> std::vector<std::string> CompareEdges(const Inputs& input) {
  std::vector<std::unique_ptr<typename Family::Cookie>> cookies;
  cookies.push_back(Cookie<Family>(input,"a","v"));cookies.push_back(Cookie<Family>(input,"a","different",1));
  cookies.push_back(Cookie<Family>(input,"b","v",2));cookies.push_back(Cookie<Family>(input,"a","v",3,true));
  auto key=Family::Key::FromURLForTesting(GURL("https://top.test"),Family::Key::AncestorChainBit::kCrossSite);
  cookies.push_back(Cookie<Family>(input,"a","v",4,true,false,key));
  const auto token=base::UnguessableToken::Deserialize(7,9).value();
  auto nonced=Family::Key::FromURLForTesting(GURL("https://top.test"),Family::Key::AncestorChainBit::kSameSite,token);
  cookies.push_back(Cookie<Family>(input,"a","v",5,true,false,nonced));
  std::vector<std::string> out;
  for(const auto& a:cookies) {
    for(const auto& b:cookies){const auto order=*a<=>*b;
    out.push_back(std::to_string(*a==*b)+std::to_string(order<0)+std::to_string(order>0)+
      std::to_string(a->IsEquivalent(*b))+std::to_string(a->IsEquivalentForSecureCookieMatching(*b))+
      std::to_string(a->IsProbablyEquivalentTo(*b))+std::to_string(a->IsWebEquivalentTo(*b))+
      std::to_string(a->IsValueEqual(*b))+std::to_string(a->IsValueEqual(std::string_view("v"))));
    }
  }
  return out;
}
TEST(CookieFamilyStoreTest, KeyEqualityOrderingAndLazyValueComparisonMatrix) {
  const Inputs input{base::Time::FromInternalValue(13380163200000000LL)};
  EXPECT_EQ(CompareEdges<Production>(input),CompareEdges<Original>(input));
}
template<class Family> auto CancelLoad(const Inputs& input,base::test::TaskEnvironment& env) {
  auto backend=base::MakeRefCounted<RecordingBackend<Family>>();size_t callbacks=0;
  {
    auto store=std::make_unique<typename Family::Monster>(backend,nullptr);
    auto options=Family::Options::MakeAllInclusive();
    store->SetCanonicalCookieAsync(Cookie<Family>(input,"n","v"),GURL("http://example.test/"),options,
      base::BindLambdaForTesting([&](typename Family::Access){++callbacks;}),std::nullopt);
    store->GetAllCookiesAsync(base::BindLambdaForTesting([&](const typename Family::List&){++callbacks;}));
    CHECK_EQ(backend->loads,1u);CHECK_EQ(backend->key_loads,1u);
  }
  backend->FinishAll();backend->FinishKey("example.test");env.RunUntilIdle();CHECK_EQ(callbacks,0u);
  return std::make_pair(callbacks,backend->trace);
}
TEST(CookieFamilyStoreTest, StoreDestructionCancelsPendingGlobalAndKeyTasks) {
  base::test::TaskEnvironment env;const Inputs input{base::Time::Now()};
  EXPECT_EQ(CancelLoad<Production>(input,env),CancelLoad<Original>(input,env));
}
template<class Family> std::vector<std::string> ReplaceAll(const Inputs& input,base::test::TaskEnvironment& env) {
  typename Family::Monster store(nullptr,nullptr);auto options=Family::Options::MakeAllInclusive();
  auto set=[&](std::unique_ptr<typename Family::Cookie> cookie){bool done=false;
    store.SetCanonicalCookieAsync(std::move(cookie),GURL("https://example.test/"),options,
      base::BindLambdaForTesting([&](typename Family::Access access){CHECK(access.status.IsInclude());done=true;}),std::nullopt);
    env.RunUntilIdle();CHECK(done);};
  set(Cookie<Family>(input,"old","unpartitioned",0,true));
  auto key=Family::Key::FromURLForTesting(GURL("https://top.test"),Family::Key::AncestorChainBit::kSameSite);
  set(Cookie<Family>(input,"kept","partitioned",1,true,false,key));
  typename Family::List replacement;
  replacement.push_back(*Cookie<Family>(input,"new","replacement",2,true));
  replacement.push_back(*Cookie<Family>(input,"expired","ignored",3,true,false,std::nullopt,1,input.creation));
  bool done=false;store.SetAllCookiesAsync(replacement,base::BindLambdaForTesting([&](typename Family::Access access){CHECK(access.status.IsInclude());done=true;}));
  env.RunUntilIdle();CHECK(done);std::vector<std::string> out;done=false;
  store.GetAllCookiesAsync(base::BindLambdaForTesting([&](const typename Family::List& rows){CHECK_EQ(rows.size(),2u);
    for(const auto& c:rows){CHECK(c.Name()=="new"||c.Name()=="kept");out.push_back(CookieDigest<Family>(c));}done=true;}));
  env.RunUntilIdle();CHECK(done);return out;
}
TEST(CookieFamilyStoreTest, DeprecatedSetAllKeepsPartitionsAndSkipsExpiredInputWithoutDelegate) {
  base::test::TaskEnvironment env;const Inputs input{base::Time::Now()};
  EXPECT_EQ(ReplaceAll<Production>(input,env),ReplaceAll<Original>(input,env));
}
template <class Family>
std::vector<std::string> DeleteFromCallback(
    bool on_read, const Inputs& input, base::test::TaskEnvironment& env) {
  auto store = std::make_unique<typename Family::Monster>(nullptr, nullptr);
  auto options = Family::Options::MakeAllInclusive();
  std::vector<std::string> trace;
  store->SetCanonicalCookieAsync(
      Cookie<Family>(input, "name", "value"), GURL("http://example.test/"),
      options, base::BindLambdaForTesting([&](typename Family::Access access) {
        CHECK(access.status.IsInclude());
        trace.push_back("set");
        if (!on_read) {
          store.reset();
        }
      }), std::nullopt);
  env.RunUntilIdle();
  if (on_read) {
    CHECK(store);
    store->GetAllCookiesAsync(base::BindLambdaForTesting([&](const typename Family::List& cookies) {
          CHECK_EQ(cookies.size(), 1u);
          const auto expected = CookieDigest<Family>(cookies.front());
          store.reset();
          // The callback's owned result must remain live after store deletion.
          CHECK_EQ(CookieDigest<Family>(cookies.front()), expected);
          trace.push_back(expected);
        }));
    env.RunUntilIdle();
  }
  CHECK(!store);
  return trace;
}

TEST(CookieFamilyStoreTest, CompletionMayDeleteStoreWhileResultRemainsLive) {
  base::test::TaskEnvironment env;
  const Inputs input{base::Time::Now()};
  for (bool on_read : {false, true}) {
    EXPECT_EQ(DeleteFromCallback<Production>(on_read, input, env),
              DeleteFromCallback<Original>(on_read, input, env));
  }
}
}  // namespace net::cookie_family_store_test
