// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
// Source-only connected fixtures. The family compiler/test milestone is pending.
#ifndef NET_COOKIES_COOKIE_FAMILY_STORE_TEST_SUPPORT_H_
#define NET_COOKIES_COOKIE_FAMILY_STORE_TEST_SUPPORT_H_
#include <array>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include "base/check.h"
#include "base/functional/bind.h"
#include "base/test/bind.h"
#include "base/location.h"
#include "base/memory/ref_counted.h"
#include "base/task/single_thread_task_runner.h"
#include "base/test/task_environment.h"
#include "net/cookies/cookie_family_test_support.h"
#include "net/cookies/cookie_monster.h"
#include "net/cookies/cookie_family_original/cookies/cookie_monster.h"

namespace net::cookie_family_store_test {
namespace common=net::cookie_family_test;
namespace original=net::cookie_family_original;
#define COOKIE_STORE_TRAITS(Name,Parent,Namespace) \
struct Name:common::Parent { \
  using Monster=Namespace::CookieMonster;using Backend=typename Monster::PersistentCookieStore; \
  using Access=Namespace::CookieAccessResult;using AccessList=Namespace::CookieAccessResultList; \
  using List=Namespace::CookieList;using Change=Namespace::CookieChangeInfo; \
  using Priority=Namespace::CookiePriority;using SourceType=Namespace::CookieSourceType; \
};
COOKIE_STORE_TRAITS(Production,Production,net)
COOKIE_STORE_TRAITS(Original,Original,net::cookie_family_original)
#undef COOKIE_STORE_TRAITS

struct Case {const char* id;const char* label;};
inline constexpr std::array<Case,15> kCases={{
  {"store_empty_queries","Empty store, asynchronous query, flush and destruction"},
  {"store_set_get_delete","Set, include, persistence, notification and explicit deletion"},
  {"store_overwrite_same_value","Same-value overwrites, inherited creation and exact causes"},
  {"store_overwrite_changed_value","Changed-value overwrite and creation replacement"},
  {"store_expired_overwrite","Expired overwrite deletes without insertion"},
  {"store_secure_overwrite_rejection","Secure overwrite exclusion and retained stored cookie"},
  {"store_http_only_rejection","HttpOnly overwrite exclusion and retained stored cookie"},
  {"store_partitioned_isolation","Unpartitioned plus two isolated partition queries"},
  {"store_nonced_partition","Nonced partition suppresses unpartitioned lookup"},
  {"store_domain_priority_gc","181-cookie priority eviction to the domain quota"},
  {"store_partition_bytes_gc","Partition byte limit evicts the oldest cookie"},
  {"store_load_deduplicate","Persistent load retains newest creation per unique key"},
  {"store_load_control_chars","Persistent load removes legacy control characters"},
  {"store_key_load_queue","Chained foreign-domain key load drains set/get in queue order"},
  {"store_global_load_queue","Global load prepends key tasks before global callbacks"},
}};
struct Inputs {base::Time creation;};
// Exact observations are compared, not only a checksum. Timestamps supplied by
// the fixture are retained; sampled runtime telemetry/clock fields are excluded.
struct Observations {
  std::vector<std::string> callbacks,statuses,queries,stored,persistence;
  std::vector<int> causes;
  std::vector<int64_t> creation;
  std::array<size_t,3> priorities{};
  size_t deleted=0,load_all=0,load_key=0;
  bool operator==(const Observations&) const=default;
  uint64_t Digest(uint64_t seed) const {
    auto add=[&](const auto& list){for(const auto& value:list)seed=common::Hash(seed,value);};
    add(callbacks);add(statuses);add(queries);add(stored);add(persistence);
    for(auto v:causes)seed=common::Hash(seed,std::to_string(v));
    for(auto v:creation)seed=common::Hash(seed,std::to_string(v));
    for(auto v:priorities)seed=common::Hash(seed,std::to_string(v));
    return common::Hash(seed,std::to_string(deleted)+":"+std::to_string(load_all)+":"+std::to_string(load_key));
  }
};
template<class Family> std::string CookieDigest(const typename Family::Cookie& c) {
  std::string out=c.Name()+"="+c.Value()+";"+c.Domain()+";"+c.Path()+";"+
      std::to_string(c.CreationDate().ToInternalValue())+";"+std::to_string(c.ExpiryDate().ToInternalValue())+";"+
      std::to_string(c.LastAccessDate().ToInternalValue())+";"+std::to_string(c.LastUpdateDate().ToInternalValue())+";"+
      std::to_string(c.SecureAttribute())+std::to_string(c.IsHttpOnly())+";"+
      std::to_string(static_cast<int>(c.SameSite()))+";"+std::to_string(c.Priority())+";"+
      std::to_string(static_cast<int>(c.SourceScheme()))+";"+std::to_string(c.SourcePort())+";"+
      std::to_string(static_cast<int>(c.SourceType()));
  if(c.PartitionKey()) {const auto serialized=Family::Key::Serialize(*c.PartitionKey());
    out+=";partition="+(serialized?serialized->GetDebugString():serialized.error());
    if(c.PartitionKey()->nonce())out+=";nonce="+c.PartitionKey()->nonce()->ToString();
  }return out;
}
template<class Family> class RecordingBackend final:public Family::Backend {
 public:
  using Cookie=typename Family::Cookie;using Loaded=typename Family::Backend::LoadedCallback;
  using Rows=std::vector<std::unique_ptr<Cookie>>;
  void Load(Loaded cb,const NetLogWithSource&) override {++loads;full=std::move(cb);trace.push_back("load-all");}
  void LoadCookiesForKey(const std::string& key,Loaded cb) override {
    ++key_loads;trace.push_back("load-key:"+key);CHECK(!keys.contains(key));keys.emplace(key,std::move(cb));
  }
  void AddCookie(const Cookie& c) override {trace.push_back("add:"+CookieDigest<Family>(c));}
  void UpdateCookieAccessTime(const Cookie& c) override {trace.push_back("access:"+CookieDigest<Family>(c));}
  void DeleteCookie(const Cookie& c) override {trace.push_back("delete:"+CookieDigest<Family>(c));}
  void SetForceKeepSessionState() override {trace.push_back("keep-session");}
  void SetBeforeCommitCallback(base::RepeatingClosure cb) override {before_commit=std::move(cb);}
  void Flush(base::OnceClosure cb) override {
    trace.push_back("flush");if(before_commit)before_commit.Run();
    if(cb)base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(FROM_HERE,std::move(cb));
  }
  void FinishAll(Rows rows={}) {CHECK(full);auto cb=std::move(full);std::move(cb).Run(std::move(rows));}
  void FinishKey(const std::string& key,Rows rows={}) {
    auto it=keys.find(key);CHECK(it!=keys.end());auto cb=std::move(it->second);keys.erase(it);std::move(cb).Run(std::move(rows));
  }
  size_t loads=0,key_loads=0;Loaded full;std::map<std::string,Loaded> keys;std::vector<std::string> trace;
 private:
  ~RecordingBackend() override=default;
  base::RepeatingClosure before_commit;
};

template<class Family> std::unique_ptr<typename Family::Cookie> Cookie(
    const Inputs& input,std::string name,std::string value,int index=0,bool secure=false,bool http=false,
    std::optional<typename Family::Key> partition=std::nullopt,int priority=1,
    std::optional<base::Time> expiry=std::nullopt,bool validate=true) {
  const auto creation=input.creation+base::Microseconds(index);
  auto c=Family::Cookie::CreateUnsafeCookieForTesting(name,value,"example.test","/",creation,
      expiry.value_or(input.creation+base::Hours(1)),creation,creation,secure,http,Family::SameSite::LAX_MODE,
      static_cast<typename Family::Priority>(priority),Family::SourceType::kOther,std::move(partition),
      static_cast<typename Family::SourceScheme>(secure?2:1),secure?443:80);
  CHECK(c);if(validate)CHECK(c->IsCanonical());return c;
}
template<class Family> Observations Run(const Case& scenario,const Inputs& input,base::test::TaskEnvironment& env) {
  using CookieType=typename Family::Cookie;using Key=typename Family::Key;
  const std::string id=scenario.id;Observations out;
  auto backend=base::MakeRefCounted<RecordingBackend<Family>>();
  {
    typename Family::Monster store(backend,nullptr);
    auto subscription=store.GetChangeDispatcher().AddCallbackForAllChanges(base::BindLambdaForTesting([&](const typename Family::Change& change){out.causes.push_back(static_cast<int>(change.cause));}));
    auto options=Family::Options::MakeAllInclusive();options.set_do_not_update_access_time();
    const GURL http("http://example.test/"),https("https://example.test/");
    auto set=[&](std::unique_ptr<CookieType> c,const GURL& url,const auto& used,const char* label,bool immediate=true) {
      const auto before=out.callbacks.size();
      // Pending callbacks never capture the local completion flag.
      store.SetCanonicalCookieAsync(std::move(c),url,used,
        base::BindLambdaForTesting([&,label](typename Family::Access access){out.callbacks.push_back(label);out.statuses.push_back(common::AccessDigest(access));
          if(std::string_view(label)=="rejected")CHECK(access.status.HasExclusionReason(
            static_cast<typename Family::Status::ExclusionReason>(id=="store_secure_overwrite_rejection"?12:13)));
          else CHECK(access.status.IsInclude());}),std::nullopt);
      if(immediate){env.RunUntilIdle();CHECK_EQ(out.callbacks.size(),before+1);}
    };
    auto all=[&](const char* label="all") {
      typename Family::List rows;bool done=false;
      store.GetAllCookiesAsync(base::BindLambdaForTesting([&](const typename Family::List& cookies){rows=cookies;done=true;out.callbacks.push_back(label);}));
      env.RunUntilIdle();CHECK(done);return rows;
    };
    auto query=[&](const typename Family::Collection& collection,const char* label) {
      bool done=false;std::string line;
      store.GetCookieListWithOptionsAsync(https,options,collection,base::BindLambdaForTesting([&](const typename Family::AccessList& included,const typename Family::AccessList& excluded){
          line=CookieType::BuildCookieLine(included);out.queries.push_back(std::string(label)+":"+line);
          for(const auto& row:included)out.statuses.push_back(common::AccessDigest(row.access_result));
          for(const auto& row:excluded)out.statuses.push_back(common::AccessDigest(row.access_result));done=true;}));
      env.RunUntilIdle();CHECK(done);return line;
    };
    auto initialize=[&]{store.GetAllCookiesAsync(base::BindLambdaForTesting([&](const typename Family::List& rows){CHECK(rows.empty());out.callbacks.push_back("initial");}));
      CHECK_EQ(backend->loads,1u);backend->FinishAll();env.RunUntilIdle();};
    auto key=[&](const char* top,bool nonce=false) {
      const auto token=base::UnguessableToken::Deserialize(0x123456789abcdef0ULL,0x0fedcba987654321ULL).value();
      return Key::FromURLForTesting(GURL(top),Key::AncestorChainBit::kSameSite,nonce?std::optional(token):std::nullopt);
    };
    if(id=="store_load_deduplicate"||id=="store_load_control_chars") {
      store.GetAllCookiesAsync(base::BindLambdaForTesting([&](const typename Family::List& rows){
        for(const auto& c:rows)out.queries.push_back(CookieDigest<Family>(c));out.callbacks.push_back("loaded");}));
      typename RecordingBackend<Family>::Rows rows;
      if(id=="store_load_deduplicate") {rows.push_back(Cookie<Family>(input,"n","old",0));rows.push_back(Cookie<Family>(input,"n","new",1));}
      else rows.push_back(Cookie<Family>(input,std::string("bad\x01",4),"value",0,false,false,std::nullopt,1,std::nullopt,false));
      backend->FinishAll(std::move(rows));env.RunUntilIdle();
    }else if(id=="store_key_load_queue") {
      set(Cookie<Family>(input,"n","value"),http,options,"set-key",false);
      store.GetCookieListWithOptionsAsync(http,options,typename Family::Collection(),base::BindLambdaForTesting([&](const typename Family::AccessList& rows,const typename Family::AccessList&){out.queries.push_back(CookieType::BuildCookieLine(rows));out.callbacks.push_back("get-key");}));
      CHECK(out.callbacks.empty());CHECK_EQ(backend->keys.size(),1u);
      typename RecordingBackend<Family>::Rows chained;
      chained.push_back(Family::Cookie::CreateUnsafeCookieForTesting("chain","value","chain.test","/",input.creation,
        input.creation+base::Hours(1),input.creation,input.creation,false,false,Family::SameSite::LAX_MODE,
        static_cast<typename Family::Priority>(1),Family::SourceType::kOther,std::nullopt,static_cast<typename Family::SourceScheme>(1),80));
      backend->FinishKey("example.test",std::move(chained));env.RunUntilIdle();
      CHECK(out.callbacks==(std::vector<std::string>{"set-key","get-key"}));backend->FinishAll();env.RunUntilIdle();
    }else if(id=="store_global_load_queue") {
      set(Cookie<Family>(input,"a","one"),http,options,"set-a",false);
      store.GetAllCookiesAsync(base::BindLambdaForTesting([&](const typename Family::List& rows){CHECK_EQ(rows.size(),1u);out.callbacks.push_back("get-all");}));
      set(Cookie<Family>(input,"b","two",1),http,options,"set-b",false);
      CHECK(out.callbacks.empty());backend->FinishAll();env.RunUntilIdle();
      CHECK(out.callbacks==(std::vector<std::string>{"set-a","get-all","set-b"}));
      // A late empty priority-load completion must not repeat queued callbacks.
      backend->FinishKey("example.test");env.RunUntilIdle();
    }else{
      initialize();
      if(id=="store_empty_queries") {CHECK(all().empty());CHECK(query(Family::Collection::ContainsAll(),"empty").empty());}
      else if(id=="store_set_get_delete") {
        set(Cookie<Family>(input,"n","value"),http,options,"set");auto rows=all();CHECK_EQ(rows.size(),1u);
        query(Family::Collection::ContainsAll(),"get");bool done=false;
        store.DeleteCanonicalCookieAsync(rows.front(),base::BindLambdaForTesting([&](uint32_t count){out.deleted=count;done=true;out.callbacks.push_back("delete");}));
        env.RunUntilIdle();CHECK(done);CHECK_EQ(out.deleted,1u);
      }else if(id=="store_overwrite_same_value") {
        set(Cookie<Family>(input,"n","v"),http,options,"first");
        auto expiry=input.creation+base::Hours(1)+base::Microseconds(1);
        set(Cookie<Family>(input,"n","v",1,false,false,std::nullopt,1,expiry),http,options,"expiry");
        set(Cookie<Family>(input,"n","v",2,false,false,std::nullopt,1,expiry),http,options,"same");
        CHECK(out.causes==(std::vector<int>{0,3,8,3,7}));
      }else if(id=="store_overwrite_changed_value") {
        set(Cookie<Family>(input,"n","old"),http,options,"first");set(Cookie<Family>(input,"n","new",1),http,options,"changed");
        CHECK(out.causes==(std::vector<int>{0,3,0}));
      }else if(id=="store_expired_overwrite") {
        set(Cookie<Family>(input,"n","old"),http,options,"first");
        set(Cookie<Family>(input,"n","new",1,false,false,std::nullopt,1,input.creation),http,options,"expired");
        CHECK(out.causes==(std::vector<int>{0,6}));
      }else if(id=="store_secure_overwrite_rejection"||id=="store_http_only_rejection") {
        const bool secure=id=="store_secure_overwrite_rejection";
        set(Cookie<Family>(input,"n","old",0,secure,!secure),secure?https:http,options,"protected");
        auto restricted=options;if(!secure)restricted.set_exclude_httponly();
        set(Cookie<Family>(input,"n","new",1),http,restricted,"rejected");CHECK(out.causes==(std::vector<int>{0}));
      }else if(id=="store_partitioned_isolation"||id=="store_nonced_partition") {
        const bool nonce=id=="store_nonced_partition";auto a=key("https://a.test",nonce);
        set(Cookie<Family>(input,"n","u",0,true),https,options,"unpartitioned");
        set(Cookie<Family>(input,"n","a",1,true,false,a),https,options,"a");
        const auto a_line=query(typename Family::Collection(a),"a");
        CHECK_EQ(a_line,nonce?"n=a":"n=u; n=a");
        if(!nonce){auto b=key("https://b.test");set(Cookie<Family>(input,"n","b",2,true,false,b),https,options,"b");
          CHECK_EQ(query(typename Family::Collection(b),"b"),"n=u; n=b");CHECK_EQ(query(typename Family::Collection(),"none"),"n=u");}
      }else if(id=="store_domain_priority_gc") {
        for(int i=0;i<181;++i)set(Cookie<Family>(input,"n"+std::to_string(i),"v",i,false,false,std::nullopt,i%3),http,options,"set");
      }else if(id=="store_partition_bytes_gc") {
        auto partition=key("https://a.test");for(int i=0;i<3;++i)set(Cookie<Family>(input,std::string(1,'a'+i),std::string(3500,'v'),i,true,false,partition),https,options,"set");
      }else{CHECK(false)<<id;}
    }
    auto final=all("final");for(const auto& c:final){out.stored.push_back(CookieDigest<Family>(c));out.creation.push_back(c.CreationDate().ToInternalValue());++out.priorities.at(c.Priority());}
    if(id=="store_overwrite_same_value")CHECK(out.creation==(std::vector<int64_t>{input.creation.ToInternalValue()}));
    if(id=="store_overwrite_changed_value")CHECK(out.creation==(std::vector<int64_t>{(input.creation+base::Microseconds(1)).ToInternalValue()}));
    if(id=="store_expired_overwrite"||id=="store_load_control_chars"||id=="store_set_get_delete")CHECK(final.empty());
    if(id=="store_domain_priority_gc"){CHECK_EQ(final.size(),150u);CHECK(out.priorities==(std::array<size_t,3>{30,60,60}));}
    if(id=="store_partition_bytes_gc"){CHECK_EQ(final.size(),2u);CHECK_EQ(final.front().Name(),"b");}
    if(id=="store_load_deduplicate"){CHECK_EQ(final.size(),1u);CHECK_EQ(final.front().Value(),"new");}
    store.SetForceKeepSessionState();bool flushed=false;
    store.FlushStore(base::BindLambdaForTesting([&]{out.callbacks.push_back("flush");flushed=true;}));env.RunUntilIdle();CHECK(flushed);
    // Observer, store, maps, native strings/crypto values and pending weak tasks
    // are destroyed inside this lifecycle. The backend is retained to observe it.
  }
  env.RunUntilIdle();out.persistence=backend->trace;out.load_all=backend->loads;out.load_key=backend->key_loads;
  return out;
}
}  // namespace net::cookie_family_store_test
#endif
