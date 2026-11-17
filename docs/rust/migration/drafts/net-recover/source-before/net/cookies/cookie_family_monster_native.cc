// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
#include "base/containers/span.h"
#include "net/cookies/cookie_family_monster_native.h"
#include <algorithm>
#include <cstring>
#include <iterator>
#include "base/check.h"
#include "base/check_is_test.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/logging.h"
#include "base/metrics/histogram_functions.h"
#include "base/rand_util.h"
#include "base/strings/stringprintf.h"
#include "base/task/single_thread_task_runner.h"
#include "net/base/features.h"
#include "net/base/url_util.h"
#include "net/cookies/cookie_monster_netlog_params.h"
#include "net/http/http_util.h"
#include "net/log/net_log.h"
#include "net/log/net_log_values.h"
#include "url/url_constants.h"
namespace net {
namespace {
using cookie_family::View;
using cookie_family::StringView;
base::Time Time(int64_t value){return base::Time::FromInternalValue(value);}
void NoNative(void*,const CookieFamilyCall*,CookieFamilyNativeResult*){CHECK(false);}
bool Relation(uint32_t operation,const CanonicalCookie& a,const CanonicalCookie& b){
  CookieFamilyRequest request{};CookieFamilyResult output{};
  if(operation==2098){request.scalar[0]=a.Path().size();request.scalar[1]=b.Path().size();}
  else{request.scalar[0]=a.LastAccessDate().ToInternalValue();request.scalar[1]=b.LastAccessDate().ToInternalValue();}
  request.scalar[2]=a.CreationDate().ToInternalValue();request.scalar[3]=b.CreationDate().ToInternalValue();
  cookie_family_call(operation,&request,&output,nullptr,NoNative);return output.scalar;
}
template<class T>T Read(std::string_view input,size_t offset){T value{};CHECK_LE(offset,input.size());CHECK_GE(input.size()-offset,sizeof(value));base::byte_span_from_ref(value).copy_from(base::as_byte_span(input).subspan(offset,sizeof(value)));return value;}
CookieFamilyAccess Decode(std::string_view input){
  CHECK_EQ(input.size(),33u);CookieFamilyAccess out{};
  out.status={Read<uint64_t>(input,0),Read<uint64_t>(input,8),Read<int32_t>(input,16),0};
  out.effective_same_site=Read<int32_t>(input,20);out.access_semantics=Read<int32_t>(input,24);
  out.scope_semantics=Read<int32_t>(input,28);out.allowed_secure=input[32]!=0;return out;
}
}
CookieMonsterRustNative::CookieMonsterRustNative(CookieMonster* value):monster(value){
  previous = Active();
  Active() = this;
  static_assert(static_cast<int32_t>(CookieMonster::CookieChangeObservability::kNotWebObservable)==0);
  static_assert(static_cast<int32_t>(CookieMonster::CookieChangeObservability::kWebObservable)==1);
  static_assert(static_cast<int32_t>(CookieMonster::CookieChangeObservability::kWebObservableWithoutValueChange)==2);
  if(monster){DCHECK_CALLED_ON_VALID_THREAD(monster->thread_checker_);alive=monster->weak_ptr_factory_.GetWeakPtr();argument[0]=monster->last_access_threshold_.InMicroseconds();}
  argument[30]=base::Seconds(CookieMonster::kRecordStatisticsIntervalSeconds).InMicroseconds();
  canonical.cookie_lookup=[this](int64_t id)->const CanonicalCookie&{return *Get(id).cookie;};
  canonical.partition_lookup=[this](int64_t id)->const std::optional<CookiePartitionKey>&{return Partition(id);};
  canonical.common.threshold=[](const CookieBase* cookie){return CookieFamilyNativeAccess::AgeThreshold(*cookie);};
}
CookieMonsterRustNative::CookieMonsterRustNative(const CookieMonster* value)
    :CookieMonsterRustNative(static_cast<CookieMonster*>(nullptr)){
  // No WeakPtrFactory mutation on an actually const public object. This
  // context is restricted to storage reads and original delegate queries.
  monster=const_cast<CookieMonster*>(value);read_only=true;
  argument[0]=value->last_access_threshold_.InMicroseconds();
}
CookieMonsterRustNative*& CookieMonsterRustNative::Active() {
  static thread_local CookieMonsterRustNative* active = nullptr;
  return active;
}
CookieMonsterRustNative::~CookieMonsterRustNative() {
  CHECK(Active() == this);
  Active() = previous.get();
  ClearObservations();
}
void CookieMonsterRustNative::ClearObservations() {
  canceled = true;
  canonical = {};
  rows.clear();
  cursors.clear();
  monster = nullptr;
  url = nullptr;
  options = nullptr;
  collection = nullptr;
  input_partition = nullptr;
  deletion_info = nullptr;
  input_list = nullptr;
  loaded_list = nullptr;
  mutable_loaded_list = nullptr;
  input_pointers = nullptr;
  input_map = nullptr;
  input_vector = nullptr;
  input_iterator_list = nullptr;
  recent_output = nullptr;
  included_target = nullptr;
  excluded_target = nullptr;
  status_reference = nullptr;
}
void CookieMonsterRustNative::BeforeDestruction(const CookieMonster* owner) {
  for (auto* context = Active(); context; context = context->previous.get()) {
    if (context->monster == owner) {
      context->ClearObservations();
    }
  }
}
CookieMonsterRustNative::Row& CookieMonsterRustNative::Get(int64_t id){
  CHECK_GE(id,0);CHECK_LT(static_cast<size_t>(id),rows.size());CHECK(rows[id].cookie);return rows[id];
}
int64_t CookieMonsterRustNative::Add(CanonicalCookie* entry){
  CHECK(entry);auto [it,was_inserted]=row_ids.emplace(entry,rows.size());
  if(was_inserted){Row row;row.cookie=entry;rows.push_back(row);}return it->second;
}
int64_t CookieMonsterRustNative::Add(Iter iterator,Map* map,std::optional<PartIter> partition){
  const auto id=Add(iterator->second.get());auto& row=Get(id);row.map=map;row.iterator=iterator;row.partition=partition;return id;
}
int64_t CookieMonsterRustNative::Own(std::unique_ptr<CanonicalCookie> cookie){
  owned.push_back(std::move(cookie));auto id=Add(owned.back().get());Get(id).owner=&owned.back();return id;
}
int64_t CookieMonsterRustNative::Part(const CookiePartitionKey& partition_key){partitions.emplace_back(partition_key);return -static_cast<int64_t>(partitions.size());}
const std::optional<CookiePartitionKey>& CookieMonsterRustNative::Partition(int64_t id){
  if(id<0){CHECK_LE(static_cast<size_t>(-id),partitions.size());return partitions[-id-1];}
  CHECK_GT(id,0);return Get(id-1).cookie->PartitionKey();
}
void CookieMonsterRustNative::Callback(void* raw,const CookieFamilyCall* call,CookieFamilyNativeResult* out){
  auto& context=*static_cast<CookieMonsterRustNative*>(raw);
  // Public callbacks may destroy the store. No subsequent primitive touches
  // its old address. Previously applied native effects are never rolled back.
  if((context.canceled||(context.monster&&!context.read_only&&!context.alive))&&call->operation!=2073){context.canceled=true;out->failed=1;return;}
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
  try{context.Primitive(*call,*out);}catch(...){context.exception=std::current_exception();out->failed=1;}
#else
  context.Primitive(*call,*out);
#endif
  if (context.canceled && call->operation != 2073) out->failed = 1;
}
CookieFamilyResult CookieMonsterRustNative::Run(uint32_t operation,const CookieFamilyRequest& request){
  canonical.common.url=url;canonical.common.options=options;canonical.common.params=&params;
  CookieFamilyResult output{};cookie_family_call(operation,&request,&output,this,Callback);
  auto error=exception?exception:(canonical.exception?canonical.exception:canonical.common.exception);
  if(error){cookie_family_buffer_free(output.buffer);std::rethrow_exception(error);}return output;
}
void CookieMonsterRustNative::Primitive(const CookieFamilyCall& call,CookieFamilyNativeResult& out){
  const auto a=call.first,b=call.second;
  if(read_only){
    switch(call.operation){
      case 2002:case 2019:case 2021:case 2022:case 2023:case 2024:case 2026:case 2027:
      case 2028:case 2029:case 2030:case 2031:case 2032:case 2033:case 2034:case 2035:
      case 2036:case 2037:case 2038:case 2039:case 2042:case 2043:case 2062:case 2064:
      case 2065:case 2066:case 2071:case 2074:case 2079:case 2080:case 2086:case 2087:
      case 2092:case 2093:CHECK(false)<<"Store write requested by a const API";
    }
    CHECK(call.operation!=2041||a!=8)<<"Mutable partition-counter lookup requested by const API";
  }
  if(call.operation==1121&&cached_target==a&&b==1){out.bytes=View(cached_target_value);return;}
  if(call.operation==1120&&cached_target==a&&b==17){out.number=cached_target_value.size();return;}
  if(call.operation<2000){cookie_family::CanonicalNativeUnchecked(canonical,call,out);return;}
  if(call.operation==2078){schemes.emplace_back(StringView(call.bytes));return;}
  // Static URL classification helpers have no CookieMonster object.
  if(call.operation==2003&&a>=64&&a<=67){
    switch(a){case 64:out.number=IsLocalhost(*url);return;
      case 65:out.number=url->EffectiveIntPort();return;
      case 66:out.number=static_cast<int32_t>(GetSchemeNameEnum(*url));return;
      case 67:out.number=url::DefaultPortForScheme(url->scheme());return;}
  }
  if(call.operation==2000&&a!=0){if(a==4)CHECK(b);else DCHECK(b);return;}
  if(call.operation==2073){base::UmaHistogramCustomMicrosecondsTimes(a?"Cookie.DeleteCanonicalCookie.Duration":"Cookie.GetCookieListWithOptions.Duration.Subsampled",timer->Elapsed(),base::Microseconds(1),base::Milliseconds(128),100);return;}
  auto& m=*monster;
  DCHECK_CALLED_ON_VALID_THREAD(m.thread_checker_);
  switch(call.operation){
    case 2000:
      switch(a){case 0:return;
        case 4:CHECK(b)<<"Duplicate equivalent cookies found, cookie store is corrupted.";return;
        default:DCHECK(b);return;}
    case 2001:
      switch(a){case 0:out.number=m.initialized_;return;case 1:out.number=m.started_fetching_all_cookies_;return;
        case 2:out.number=m.finished_fetching_all_cookies_;return;case 3:out.number=m.seen_global_task_;return;
        case 4:out.number=m.store_!=nullptr;return;case 5:out.number=m.persist_session_cookies_;return;
        case 6:out.number=m.earliest_access_time_.ToInternalValue();return;case 7:out.number=m.last_statistic_record_time_.ToInternalValue();return;
        case 8:out.number=m.time_start_block_load_all_.value_or(base::TimeTicks()).ToInternalValue();return;case 9:out.number=m.time_start_block_load_all_.has_value();return;}CHECK(false);
    case 2002:
      switch(a){case 0:m.initialized_=b;return;case 1:m.started_fetching_all_cookies_=b;return;
        case 2:m.finished_fetching_all_cookies_=b;return;case 3:m.seen_global_task_=b;return;
        case 5:m.persist_session_cookies_=b;return;case 6:m.earliest_access_time_=Time(b);return;
        case 7:m.last_statistic_record_time_=Time(b);return;case 8:m.time_start_block_load_all_=base::TimeTicks::FromInternalValue(b);return;
        case 9:if(!b)m.time_start_block_load_all_.reset();return;}CHECK(false);
    case 2003:
      switch(a){case 1:out.number=options->return_excluded_cookies();return;case 2:out.number=options->update_access_time();return;
        case 3:out.number=options->exclude_httponly();return;
        case 4:out.number=Get(b).cookie->IsEffectivelySameSiteNone();return;
        case 5:out.number=inherited_creation.ToInternalValue();return;
        case 20:out.number=sorted_ids.at(b);return;case 50:out.number=prior_access.has_value();return;
        case 60:out.number=trusted;return;
        case 64:out.number=IsLocalhost(*url);return;case 65:out.number=url->EffectiveIntPort();return;
        case 66:out.number=static_cast<int32_t>(GetSchemeNameEnum(*url));return;
        case 67:out.number=url::DefaultPortForScheme(url->scheme());return;
        case 70:{const auto& cookie=*Get(b).cookie;out.number=recent_output->contains(cookie.LegacyUniqueKey());return;}
        case 71:{const auto& cookie=*Get(b).cookie;out.number=recent_output->find(cookie.LegacyUniqueKey())->second.first.ToInternalValue();return;}
        default:CHECK_GE(a,0);CHECK_LT(static_cast<size_t>(a),argument.size());out.number=argument[a];return;}
    case 2004:out.number=base::Time::Now().ToInternalValue();return;
    case 2005:out.number=base::TimeTicks::Now().ToInternalValue();return;
    case 2006:{Cursor cursor;
      switch(a){
        case 0:cursor.kind=1;cursor.map=&m.cookies_;cursor.current=cursor.map->begin();cursor.end=cursor.map->end();cursor.exhausted=*cursor.current==*cursor.end;break;
        case 1:cursor.kind=2;cursor.part_current=m.partitioned_cookies_.begin();cursor.part_end=m.partitioned_cookies_.end();cursor.exhausted=*cursor.part_current==*cursor.part_end;break;
        case 2:CHECK(input_range);cursor.kind=1;cursor.map=input_map?input_map.get():&m.cookies_;cursor.current=input_range->first;cursor.end=input_range->second;cursor.partition=selected_partition;cursor.exhausted=*cursor.current==*cursor.end;break;
        case 3:cursor.kind=3;
          if(mutable_loaded_list){for(auto& cookie:*mutable_loaded_list){auto id=Add(cookie.get());Get(id).owner=&cookie;cursor.ids.push_back(id);}}
          else if(input_list){for(const auto& cookie:*input_list)cursor.ids.push_back(Add(const_cast<CanonicalCookie*>(&cookie)));}
          cursor.exhausted=cursor.ids.empty();break;
        case 4:CHECK(selected_partition);cursor.kind=1;cursor.partition=selected_partition;cursor.map=(*selected_partition)->second.get();cursor.current=cursor.map->begin();cursor.end=cursor.map->end();cursor.exhausted=*cursor.current==*cursor.end;break;
        case 5:cursor.kind=4;cursor.task_current=m.tasks_pending_for_key_.begin();cursor.task_end=m.tasks_pending_for_key_.end();cursor.exhausted=*cursor.task_current==*cursor.task_end;break;
        case 8:cursor.kind=3;
          if(input_pointers){for(auto* cookie:*input_pointers)cursor.ids.push_back(Add(cookie));}
          else if(input_vector){for(auto it:*input_vector)cursor.ids.push_back(Add(it,it->second->IsPartitioned()?(*selected_partition)->second.get():&m.cookies_,it->second->IsPartitioned()?selected_partition:std::nullopt));}
          else if(input_iterator_list){for(auto it:*input_iterator_list)cursor.ids.push_back(Add(it,it->second->IsPartitioned()?(*selected_partition)->second.get():&m.cookies_,it->second->IsPartitioned()?selected_partition:std::nullopt));}
          else if(!iterator_input.empty()){for(auto it:iterator_input)cursor.ids.push_back(Add(it,&m.cookies_,std::nullopt));}
          else{for(auto* cookie:pointers)cursor.ids.push_back(Add(cookie));}cursor.exhausted=cursor.ids.empty();break;
        case 9:cursor.kind=3;for(const auto& partition:collection->PartitionKeys())cursor.ids.push_back(Part(partition));cursor.exhausted=cursor.ids.empty();break;
        case 10:cursor.kind=5;cursor.byte_current=m.bytes_per_cookie_partition_.begin();cursor.byte_end=m.bytes_per_cookie_partition_.end();cursor.exhausted=*cursor.byte_current==*cursor.byte_end;break;
        default:CHECK(false)<<a;
      }cursors.push_back(std::move(cursor));out.number=cursors.size()-1;return;}
    case 2007:out.number=cursors.at(a).exhausted;return;
    case 2008:{auto& cursor=cursors.at(a);CHECK(!cursor.exhausted);
      switch(cursor.kind){case 1:out.number=Add(*cursor.current,cursor.map,cursor.partition);return;
        case 2:out.number=Part((*cursor.part_current)->first);return;case 3:out.number=cursor.ids.at(cursor.position);return;
        case 4:scratch=(*cursor.task_current)->first;out.number=a;return;case 5:out.number=(*cursor.byte_current)->second;return;}CHECK(false);}
    case 2009:{auto& cursor=cursors.at(a);CHECK(!cursor.exhausted);
      switch(cursor.kind){case 1:++*cursor.current;cursor.exhausted=*cursor.current==*cursor.end;return;
        case 2:++*cursor.part_current;cursor.inner.reset();cursor.exhausted=*cursor.part_current==*cursor.part_end;return;
        case 3:cursor.exhausted=++cursor.position==cursor.ids.size();return;
        case 4:++*cursor.task_current;cursor.exhausted=*cursor.task_current==*cursor.task_end;return;
        case 5:++*cursor.byte_current;cursor.exhausted=*cursor.byte_current==*cursor.byte_end;return;}CHECK(false);}
    case 2010:{Cursor cursor;cursor.kind=1;
      switch(a){
        case 0:cursor.map=input_map?input_map.get():&m.cookies_;if(input_map&&input_map!=&m.cookies_)cursor.partition=selected_partition;break;
        case 1:CHECK(selected_partition);cursor.map=(*selected_partition)->second.get();cursor.partition=selected_partition;break;
        case 2:{const auto& cookie=*Get(b).cookie;if(cookie.IsPartitioned()){auto it=m.partitioned_cookies_.find(*cookie.PartitionKey());CHECK(it!=m.partitioned_cookies_.end());cursor.map=it->second.get();cursor.partition=it;}else cursor.map=&m.cookies_;break;}
        case 3:{auto& outer=cursors.at(b);
          if(outer.kind==1){cursor.map=outer.map;cursor.current=*outer.current;key=(*cursor.current)->first;cursor.end=cursor.map->upper_bound(key);outer.current=cursor.end;outer.exhausted=*outer.current==*outer.end;}
          else{CHECK_EQ(outer.kind,2);auto part=*outer.part_current;cursor.partition=part;cursor.map=part->second.get();cursor.current=outer.inner.value_or(cursor.map->begin());key=(*cursor.current)->first;cursor.end=cursor.map->upper_bound(key);outer.inner=cursor.end;
            if(*outer.inner==cursor.map->end()){++*outer.part_current;outer.inner.reset();outer.exhausted=*outer.part_current==*outer.part_end;}}
          cursor.exhausted=*cursor.current==*cursor.end;cursors.push_back(std::move(cursor));out.number=cursors.size()-1;return;}
        case 4:if(b==0)cursor.map=&m.cookies_;else{auto it=m.partitioned_cookies_.find(*Partition(b));CHECK(it!=m.partitioned_cookies_.end());cursor.map=it->second.get();cursor.partition=it;}break;
        default:CHECK(false)<<a;
      }auto pair=cursor.map->equal_range(key);cursor.current=pair.first;cursor.end=pair.second;cursor.exhausted=pair.first==pair.second;
      cursors.push_back(std::move(cursor));out.number=cursors.size()-1;return;}
    case 2011:
      switch(a){case 0:out.number=m.cookies_.count(key);return;case 1:out.number=(*selected_partition)->second->count(key);return;
        case 10:out.number=m.tasks_pending_for_key_.contains(key);return;case 11:out.number=m.tasks_pending_for_key_.size();return;
        case 20:{const auto* keys=CookieFamilyNativeAccess::Keys(*collection);out.number=keys?static_cast<int64_t>(keys->size()):-1;return;}
        case 30:out.number=m.cookieable_schemes_.size();return;case 31:{auto it=m.partitioned_cookies_.find(*Partition(b));out.number=it->second->count(key);return;}}
      CHECK(false)<<a;
    case 2012:selected_cookie=a;canonical.common.cookie=Get(a).cookie;return;
    case 2013:out.bytes=View(a==1?key:domain);return;
    case 2014:case 2015:CHECK(false)<<"Native key policy is not an adapter primitive";
    case 2016:
      switch(a){case 0:out.number=base::FeatureList::IsEnabled(features::kEnableSchemeBoundCookies);return;
        case 2:out.number=base::FeatureList::IsEnabled(features::kCookieMonsterPerKeyDelete);return;
        case 3:out.number=base::FeatureList::IsEnabled(features::kEnablePortBoundCookies);return;}CHECK(false)<<a;
    case 2017:out.number=base::ShouldRecordSubsampledMetric(kHistogramSampleProbability);return;
    case 2018:out.number=!!closure;return;
    case 2019:
      switch(a){case 0:std::move(closure).Run();return;case 1:std::move(active_task).Run();return;
        case 2:if(set_callback)std::move(set_callback).Run(access[0]);return;
        case 3:if(all_callback)std::move(all_callback).Run(cookies_output);return;
        case 4:if(list_callback)std::move(list_callback).Run(std::move(included_output),std::move(excluded_output));return;
        case 5:if(schemes_callback)std::move(schemes_callback).Run(b);return;
        case 6:if(semantics_callback)std::move(semantics_callback).Run(*input_list,std::move(semantics_output));return;}
      CHECK(false)<<a;
    case 2020:out.number=a?m.tasks_pending_for_key_.at(key).empty():m.tasks_pending_.empty();return;
    case 2021:
      switch(a){case 0:m.tasks_pending_.push_back(std::move(closure));return;case 1:m.tasks_pending_for_key_.at(key).push_back(std::move(closure));return;
        case 2:m.tasks_pending_for_key_.emplace(key,base::circular_deque<base::OnceClosure>());return;}CHECK(false);
    case 2022:if(a){auto& queue=m.tasks_pending_for_key_.at(key);active_task=std::move(queue.front());queue.pop_front();}
      else{active_task=std::move(m.tasks_pending_.front());m.tasks_pending_.pop_front();}return;
    case 2023:if(a==1)m.tasks_pending_for_key_.erase(key);else if(a==2)m.tasks_pending_for_key_.clear();else CHECK(false);return;
    case 2024:{auto& tasks=(*cursors.at(a).task_current)->second;m.tasks_pending_.insert(m.tasks_pending_.begin(),std::make_move_iterator(tasks.begin()),std::make_move_iterator(tasks.end()));return;}
    case 2025:out.number=m.keys_loaded_.contains(key);return;
    case 2026:if(a==0)m.keys_loaded_.insert(key);else if(a==1)m.domain_purged_keys_.insert(key);else CHECK(false);return;
    case 2027:m.keys_loaded_.clear();return;
    case 2028:m.store_->Load(base::BindOnce(&CookieMonster::OnLoaded,m.weak_ptr_factory_.GetWeakPtr(),base::TimeTicks::Now()),m.net_log_);return;
    case 2029:m.store_->LoadCookiesForKey(key,base::BindOnce(&CookieMonster::OnKeyLoaded,m.weak_ptr_factory_.GetWeakPtr(),key));return;
    case 2030:m.store_->Flush(std::move(closure));return;
    case 2031:base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(FROM_HERE,std::move(closure));return;
    case 2032:m.store_->SetForceKeepSessionState();return;
    case 2033:{auto& row=Get(a);CHECK(row.owner);auto* pointer=row.cookie.get();
      if(b){const auto& part=*pointer->PartitionKey();auto it=m.partitioned_cookies_.find(part);
        if(it==m.partitioned_cookies_.end())it=m.partitioned_cookies_.emplace(part,std::make_unique<Map>()).first;
        auto cookie_it=it->second->insert(Map::value_type(key,std::move(*row.owner)));
        row.iterator=cookie_it;row.map=it->second.get();row.partition=it;selected_partition=it;inserted.emplace(CookieMonster::PartitionedCookieMapIterators(it,cookie_it));}
      else{auto it=m.cookies_.insert(Map::value_type(key,std::move(*row.owner)));row.iterator=it;row.map=&m.cookies_;inserted.emplace(it);}
      row.owner=nullptr;out.number=a;return;}
    case 2034:{auto& row=Get(a);auto* map=row.map.get();auto* pointer=row.cookie.get();row.cookie=nullptr;row.map=nullptr;
      if(b){auto partition=*row.partition;map->erase(*row.iterator);if(map->empty()){
          for(auto& cursor:cursors)if(cursor.map==map){cursor.exhausted=true;cursor.map=nullptr;}m.partitioned_cookies_.erase(partition);}}
      else map->erase(*row.iterator);
      row_ids.erase(pointer);row.cookie=nullptr;row.iterator.reset();row.partition.reset();return;}
    case 2035:{auto* cookie=Get(selected_cookie).cookie.get();m.change_dispatcher_.DispatchChange(
        CookieChangeInfo(*cookie,access.at(argument[68]),static_cast<CookieChangeCause>(a)),b);return;}
    case 2036:m.store_->AddCookie(*Get(a).cookie);return;
    case 2037:m.store_->DeleteCookie(*Get(a).cookie);return;
    case 2038:m.store_->UpdateCookieAccessTime(*Get(a).cookie);return;
    case 2039:Log(a,b);return;
    case 2040:Metric(a,b);return;
    case 2041:
      switch(a){case 0:out.number=m.cookies_.size();return;case 1:out.number=m.num_partitioned_cookies_;return;case 2:out.number=m.num_nonced_partitioned_cookies_;return;
        case 3:out.number=m.num_partitioned_cookies_bytes_;return;case 4:out.number=m.num_nonced_partitioned_cookie_bytes_;return;
        case 5:out.number=m.num_keys_;return;case 6:out.number=m.domain_purged_keys_.size();return;case 7:out.number=m.partitioned_cookies_.size();return;
        case 8:out.number=m.bytes_per_cookie_partition_[*Get(selected_cookie).cookie->PartitionKey()];return;
        }
      CHECK(false)<<a;
    case 2042:
      switch(a){case 1:m.num_partitioned_cookies_=b;return;case 2:m.num_nonced_partitioned_cookies_=b;return;
        case 3:m.num_partitioned_cookies_bytes_=b;return;case 4:m.num_nonced_partitioned_cookie_bytes_=b;return;
        case 5:m.num_keys_=b;return;case 8:m.bytes_per_cookie_partition_[*Get(selected_cookie).cookie->PartitionKey()]=b;return;}CHECK(false)<<a;
    case 2044:{auto& row=Get(a);const auto it=*row.iterator;
      if(b<0)out.number=it!=row.map->begin()&&std::prev(it)->first==it->first;
      else out.number=std::next(it)!=row.map->end()&&std::next(it)->first==it->first;return;}
    case 2045:{const CookiePartitionKey* partition=input_partition;
      if(!partition&&a>=0&&static_cast<size_t>(a)<rows.size()&&rows[a].cookie&&rows[a].cookie->PartitionKey())partition=&*rows[a].cookie->PartitionKey();
      if(!partition&&selected_partition)partition=&(*selected_partition)->first;
      CHECK(partition);auto found=m.partitioned_cookies_.find(*partition);out.number=found!=m.partitioned_cookies_.end();
      if(out.number)selected_partition=found;else selected_partition.reset();return;}
    case 2046:{if(a==0&&status_reference)access[0].status=*status_reference;
      const auto value=cookie_family::Snapshot(access.at(a));out.number=value.status.exclusions;out.number2=value.status.warnings;
      out.number3=value.effective_same_site;out.number4=value.status.exemption;
      out.params={value.access_semantics,value.scope_semantics,value.allowed_secure};return;}
    case 2047:access.at(a)=cookie_family::Materialize(Decode(StringView(call.bytes)));if(a==0&&status_reference)*status_reference=access[0].status;return;
    case 2048:{auto* delegate=m.cookie_access_delegate();
      switch(a){case -1:out.number=delegate!=nullptr;return;
        case 0:trusted=delegate->ShouldTreatUrlAsTrustworthy(b?deletion_info->url.value():*url);out.number=trusted;return;
        case 1:out.number=static_cast<int32_t>(delegate->GetAccessSemantics(*Get(b).cookie));return;
        case 2:out.number=static_cast<int32_t>(delegate->GetScopeSemantics(b==-2?key:(b>=0&&static_cast<size_t>(b)<rows.size()?Get(b).cookie->Domain():domain)));return;}
      CHECK(false)<<a;}
    case 2051:
      switch(a){case 0:out.number=(Time(b)-Get(selected_cookie).cookie->LastAccessDate()).InMicroseconds();return;
        case 1:out.number=(Time(b)-base::Days(CookieMonster::kSafeFromGlobalPurgeDays)).ToInternalValue();return;
        case 2:out.number=(Time(b)-m.last_statistic_record_time_).InMicroseconds();return;
        case 3:out.number=static_cast<int>((Get(selected_cookie).cookie->ExpiryDate()-record_creation).InMinutes());return;
        case 4:out.number=(base::TimeTicks::FromInternalValue(b)-*m.time_start_block_load_all_).InMicroseconds();return;}CHECK(false)<<a;
    case 2052:{auto* cookie=Get(a).cookie.get();
      switch(b){case 0:pointers.push_back(cookie);if(input_vector)input_vector->push_back(*Get(a).iterator);iterator_output.push_back(*Get(a).iterator);return;
        case 1:(excluded_target?*excluded_target:excluded_output).emplace_back(*cookie,access[0]);return;
        case 2:(included_target?*included_target:included_output).emplace_back(*cookie,access[0]);return;
        case 3:sorted_ids.push_back(a);return;case 4:cookies_output.push_back(*cookie);return;
        case 5:pointers.push_back(cookie);return;case 6:kept_ids.push_back(a);return;
        case 7:recent_output->insert_or_assign(cookie->LegacyUniqueKey(),std::make_pair(cookie->CreationDate(),cookie->StrictlyUniqueKey()));return;}
      CHECK(false)<<b;}
    case 2053:
      switch(a){case 0:cookies_output.clear();cookies_output.reserve(b);return;
        case 1:included_output.clear();excluded_output.clear();pointers.clear();return;
        case 2:pointers.clear();pointers.reserve(b);included_output.reserve(b);return;
        case 4:sorted_ids.clear();return;case 5:kept_ids.clear();return;}CHECK(false)<<a;
    case 2054:out.number=predicate.Run(*Get(a).cookie);return;
    case 2055:out.number=deletion_info->Matches(*Get(a).cookie,params);return;
    case 2057:if(b==2){cached_target_value=Get(a).cookie->Value();cached_target=a;return;}out.number=Own(std::make_unique<CanonicalCookie>(*Get(a).cookie));return;
    case 2058:out.number=CookieFamilyNativeAccess::Keys(*collection)==nullptr;return;
    case 2059:out.number=a?m.pref_delegate_!=nullptr:m.pref_delegate_dict_!=nullptr;return;
    case 2060:out.number=m.pref_delegate_->IsPrefReady();return;
    case 2062:m.pref_delegate_dict_=a?std::make_unique<base::DictValue>(m.pref_delegate_->GetLegacyDomains().Clone()):std::make_unique<base::DictValue>();return;
    case 2063:out.number=m.pref_delegate_dict_->Find(key)!=nullptr;return;
    case 2064:m.pref_delegate_dict_->Set(key,base::Value(true));return;
    case 2065:m.pref_delegate_dict_->Remove(key);return;
    case 2066:m.pref_delegate_->SetLegacyDomains(m.pref_delegate_dict_->Clone());return;
    case 2067:{
      // Retain the original element kinds as well as std::sort: configured
      // standard libraries may select different algorithms by element type.
      if(a==0){std::vector<CanonicalCookie*> work;work.reserve(sorted_ids.size());
        for(auto id:sorted_ids)work.push_back(Get(id).cookie);
        std::sort(work.begin(),work.end(),[](const auto* left,const auto* right){return Relation(2098,*left,*right);});
        for(size_t i=0;i<work.size();++i)sorted_ids[i]=Add(work[i]);
      }else{std::vector<Iter> work;work.reserve(sorted_ids.size());for(auto id:sorted_ids)work.push_back(*Get(id).iterator);
        std::sort(work.begin(),work.end(),[](const auto& left,const auto& right){return Relation(2099,*left->second,*right->second);});
        for(size_t i=0;i<work.size();++i)sorted_ids[i]=Add(work[i]->second.get());
      }return;}
    case 2068:key=std::string(StringView(call.bytes));return;
    case 2070:switch(a){case 0:inherited_creation=Time(b);return;case 1:semantics_output.push_back(static_cast<CookieAccessSemantics>(b));return;
      case 2:earliest_output=Time(b);return;case 3:argument[61]=b;return;case 4:argument[62]=b;return;
      case 5:record_creation=Time(b);return;case 6:argument[63]=b;return;case 7:argument[68]=b;return;
      case 8:if(input_vector){auto found=std::find(input_vector->begin(),input_vector->end(),*Get(b).iterator);CHECK(found!=input_vector->end());vector_positions[b]=found-input_vector->begin();}
        if(input_iterator_list){auto found=std::find(input_iterator_list->begin(),input_iterator_list->end(),*Get(b).iterator);CHECK(found!=input_iterator_list->end());list_positions[b]=found;}return;
      case 9:if(input_vector){input_vector->erase(input_vector->begin()+vector_positions.at(b));vector_positions.erase(b);}
        if(input_iterator_list){input_iterator_list->erase(list_positions.at(b));list_positions.erase(b);}return;}CHECK(false)<<a;
    case 2071:{auto weak=m.weak_ptr_factory_.GetWeakPtr();closure=base::BindOnce([](base::WeakPtr<CookieMonster> store,CookieStore::DeleteCallback callback,uint32_t count){if(store&&callback)std::move(callback).Run(count);},weak,std::move(delete_callback),static_cast<uint32_t>(a));
      return;}
    case 2072:timer.emplace();return;
    case 2074:base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(FROM_HERE,base::BindOnce(&CookieMonster::DoCookieCallbackForHostOrDomain,m.weak_ptr_factory_.GetWeakPtr(),base::DoNothing(),std::string(url->host())));return;
    case 2076:out.bytes=View((*Get(a).iterator)->first);return;
    case 2077:{std::vector<Iter> work;work.reserve(sorted_ids.size());for(auto id:sorted_ids)work.push_back(*Get(id).iterator);
      std::partial_sort(work.begin(),work.begin()+a,work.end(),[](const auto& left,const auto& right){return Relation(2099,*left->second,*right->second);});
      for(size_t i=0;i<work.size();++i)sorted_ids[i]=Add(work[i]->second.get());return;}
    case 2079:m.cookieable_schemes_=std::move(schemes);return;
    case 2080:m.net_log_.AddEntryWithBoolParams(NetLogEventType::COOKIE_STORE_SESSION_PERSISTENCE,NetLogEventPhase::NONE,"persistence",a);m.persist_session_cookies_=a;return;
    case 2081:if(b==1){CHECK(input_partition);out.number=Part(*input_partition);}else out.number=Partition(a)->nonce().has_value();return;
    case 2082:CHECK(false)<<"Partition comparison must use direct shared Rust policy";
    case 2084:out.number=argument[79];return;
    case 2086:Get(a).cookie->SetCreationDate(Time(b));return;
    case 2087:Get(a).cookie->SetLastAccessDate(Time(b));return;
    case 2088:out.bytes=View(url->scheme());return;
    case 2089:out.bytes=View(m.cookieable_schemes_.at(a));return;
    case 2090:{const auto& partition=Partition(a);input_partition=&partition.value();auto found=m.partitioned_cookies_.find(*input_partition);if(found==m.partitioned_cookies_.end())selected_partition.reset();else selected_partition=found;return;}
    case 2091:params=CookieAccessParams(static_cast<CookieAccessSemantics>(a),static_cast<CookieScopeSemantics>(b),!StringView(call.bytes).empty()&&StringView(call.bytes)[0]!=0);return;
    case 2092:CookieFamilyNativeAccess::PostInclude(*Get(a).cookie,access[0],*options,static_cast<CookieOptions::SameSiteCookieContext::ContextType>(b));return;
    case 2093:CookieFamilyNativeAccess::PostSet(*Get(a).cookie,access[0],*options);return;
    case 2095:{const auto facts=cookie_family::CanonicalUrl(*url);if(a==0){out.bytes=facts.scheme;out.bytes2=facts.host;
        out.number=int64_t(facts.standard)|(int64_t(facts.is_ip)<<1)|(int64_t(facts.cryptographic)<<2)|(int64_t(facts.localhost)<<3)|(int64_t(facts.host_compliant)<<4);
        out.number2=facts.effective_port;out.number3=facts.identity;}else{out.bytes=facts.path;}return;}
  }
  CHECK(false)<<"Unknown native ownership primitive: "<<call.operation;
}
void CookieMonsterRustNative::Log(int64_t kind,int64_t id){
  auto& m=*monster;
  DCHECK_CALLED_ON_VALID_THREAD(m.thread_checker_);
  switch(kind){
    case 0:DVLOG(cookie_util::kVlogSetCookies)<<"InternalDeleteCookie()"<<", cause:"<<argument[62]<<", cc: "<<Get(id).cookie->DebugString();return;
    case 1:m.net_log_.AddEvent(NetLogEventType::COOKIE_STORE_COOKIE_DELETED,[&](NetLogCaptureMode capture){return NetLogCookieMonsterCookieDeleted(Get(id).cookie,static_cast<CookieChangeCause>(argument[63]),argument[61],capture);});return;
    case 2:m.net_log_.AddEvent(NetLogEventType::COOKIE_STORE_COOKIE_ADDED,[&](NetLogCaptureMode capture){return NetLogCookieMonsterCookieAdded(Get(id).cookie,argument[61],capture);});return;
    case 3:{const auto signature=Get(selected_cookie).cookie->UniqueKey();LOG(ERROR)<<base::StringPrintf("Found %d duplicate cookies for key='%s', with {name='%s', domain='%s', path='%s'}",static_cast<int>(id),key.c_str(),signature.name().c_str(),signature.domain().c_str(),signature.path().c_str());return;}
    case 4:skipped_secure=id;m.net_log_.AddEvent(NetLogEventType::COOKIE_STORE_COOKIE_REJECTED_SECURE,[&](NetLogCaptureMode capture){return NetLogCookieMonsterCookieRejectedSecure(Get(id).cookie,Get(argument[79]).cookie,capture);});return;
    case 5:m.net_log_.AddEvent(NetLogEventType::COOKIE_STORE_COOKIE_REJECTED_HTTPONLY,[&](NetLogCaptureMode capture){return NetLogCookieMonsterCookieRejectedHttponly(Get(id).cookie,Get(argument[79]).cookie,capture);});return;
    case 6:m.net_log_.AddEvent(NetLogEventType::COOKIE_STORE_COOKIE_PRESERVED_SKIPPED_SECURE,[&](NetLogCaptureMode capture){return NetLogCookieMonsterCookiePreservedSkippedSecure(Get(*skipped_secure).cookie,Get(id).cookie,Get(argument[79]).cookie,capture);});return;
    case 7:DVLOG(cookie_util::kVlogSetCookies)<<"SetCookie() not clobbering httponly cookie or secure cookie for insecure scheme";return;
    case 8:DVLOG(cookie_util::kVlogSetCookies)<<"SetCookie() key: "<<key<<" cc: "<<Get(id).cookie->DebugString();return;
    case 9:DVLOG(cookie_util::kVlogSetCookies)<<"SetCookie() not storing already expired cookie.";return;
    case 10:DVLOG(cookie_util::kVlogPerCookieMonster)<<"WARNING: Unsupported cookie scheme: "<<url->GetScheme();return;
    case 11:m.net_log_.EndEvent(NetLogEventType::COOKIE_STORE_ALIVE);return;
    case 12:DVLOG(cookie_util::kVlogGarbageCollection)<<"GarbageCollect() key: "<<key;return;
    case 13:DVLOG(cookie_util::kVlogGarbageCollection)<<"Deep Garbage Collect domain.";return;
    case 14:DVLOG(cookie_util::kVlogGarbageCollection)<<"GarbageCollect() everything";return;
    case 15:DVLOG(cookie_util::kVlogGarbageCollection)<<"Deep Garbage Collect everything.";return;
  }CHECK(false)<<kind;
}
void CookieMonsterRustNative::Metric(int64_t kind,int64_t value){
  switch(kind){
    case 0:base::UmaHistogramExactLinear("Cookie.Type2.Subsampled",value,16);return;
    case 2:base::UmaHistogramCounts10000("Cookie.SameSiteNoneSizeBytes.Subsampled",value);return;
    case 3:base::UmaHistogramBoolean("Cookie.IsPersistentWhenSet.Subsampled",value);return;
    case 4:base::UmaHistogramBoolean("Cookie.DomainSet.Subsampled",value);return;
    case 5:base::UmaHistogramEnumeration("Cookie.Port.Set.Localhost.Subsampled",ReducePortRangeForCookieHistogram(value));return;
    case 6:base::UmaHistogramEnumeration("Cookie.Port.Read.Localhost.Subsampled",ReducePortRangeForCookieHistogram(value));return;
    case 7:base::UmaHistogramEnumeration("Cookie.Port.Read.RemoteHost.Subsampled",ReducePortRangeForCookieHistogram(value));return;
    case 8:base::UmaHistogramEnumeration("Cookie.Port.ReadDiffersFromSet.Localhost.Subsampled",static_cast<CookieMonster::CookieSentToSamePort>(value));return;
    case 9:base::UmaHistogramEnumeration("Cookie.Port.ReadDiffersFromSet.RemoteHost.Subsampled",static_cast<CookieMonster::CookieSentToSamePort>(value));return;
    case 10:base::UmaHistogramCounts100000("Cookie.Count2",value);return;
    case 11:base::UmaHistogramCounts1000("Cookie.MaxSameSiteNoneCookiesPerKey",value);return;
    case 12:base::UmaHistogramCounts100000("Cookie.CookieJarSize",value);return;
    case 13:base::UmaHistogramCounts100000("Cookie.AvgCookieJarSizePerKey2",value);return;
    case 14:base::UmaHistogramCounts100000("Cookie.MaxCookieJarSizePerKey",value);return;
    case 15:base::UmaHistogramCounts1000("Cookie.PartitionCount",value);return;
    case 16:base::UmaHistogramCounts100000("Cookie.PartitionedCookieCount",value);return;
    case 17:base::UmaHistogramCounts100000("Cookie.PartitionedCookieCount.Nonced",value);return;
    case 18:base::UmaHistogramCounts100000("Cookie.PartitionedCookieCount.Unnonced",value);return;
    case 19:base::UmaHistogramCounts100000("Cookie.PartitionedCookieJarSizeKibibytes",value);return;
    case 20:base::UmaHistogramCounts100000("Cookie.PartitionedCookieJarSizeKibibytes.Nonced",value);return;
    case 21:base::UmaHistogramCounts100000("Cookie.PartitionedCookieJarSizeKibibytes.Unnonced",value);return;
    case 22:base::UmaHistogramCounts100000("Cookie.CookiePartitionSizeKibibytes",value);return;
    case 23:base::UmaHistogramEnumeration("Cookie.SourceType.Subsampled",static_cast<CookieSourceType>(value));return;
    case 24:base::UmaHistogramCounts10000("Cookie.SameSiteNoneSizeBytes.Partitioned.Subsampled",value);return;
    case 25:base::UmaHistogramCounts10000("Cookie.SameSiteNoneSizeBytes.Unpartitioned.Subsampled",value);return;
    case 26:base::UmaHistogramCustomCounts("Cookie.ExpirationDurationMinutesSecure.Subsampled2",value,1,60*24*400,100);return;
    case 27:base::UmaHistogramCustomCounts("Cookie.ExpirationDurationMinutesNonSecure.Subsampled2",value,1,60*24*400,100);return;
    case 28:base::UmaHistogramEnumeration("Cookie.Port.Set.RemoteHost.Subsampled",ReducePortRangeForCookieHistogram(value));return;
    case 29:base::UmaHistogramEnumeration("Cookie.CookieSourceSchemeName.Subsampled",static_cast<decltype(GetSchemeNameEnum(*url))>(value));return;
    case 30:base::UmaHistogramCustomTimes("Cookie.TimeBlockedOnLoad",base::TimeTicks::FromInternalValue(value)-load_begin,base::Milliseconds(1),base::Minutes(1),50);return;
    case 31:base::UmaHistogramCustomTimes("Cookie.TimeOpsBlockedDueToGlobalOp",base::Microseconds(value),base::Milliseconds(1),base::Minutes(1),50);return;
    case 32:base::UmaHistogramEnumeration("Cookie.Port.ReadDiffersFromSet.DomainSet.Subsampled",static_cast<CookieMonster::CookieSentToSamePort>(value));return;
    case 40:base::UmaHistogramBoolean("Cookie.OnPreconnect.LoadCookie",value);return;
  }CHECK(false)<<kind;
}
}  // namespace net
