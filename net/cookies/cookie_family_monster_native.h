// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
#ifndef NET_COOKIES_COOKIE_FAMILY_MONSTER_NATIVE_H_
#define NET_COOKIES_COOKIE_FAMILY_MONSTER_NATIVE_H_
#include "base/memory/raw_ptr.h"
#include <array>
#include <deque>
#include <exception>
#include <list>
#include <map>
#include <optional>
#include <variant>
#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/timer/elapsed_timer.h"
#include "net/cookies/cookie_family_canonical_native.h"
#include "net/cookies/cookie_monster.h"
namespace net {
// Narrow native storage/effect owner. Rust never sees this layout. All native
// iterators are kept here and preadvanced at the point selected by Rust.
struct CookieMonsterRustNative {
  using Map=CookieMonster::CookieMap;
  using Parts=CookieMonster::PartitionedCookieMap;
  using Iter=Map::iterator;
  using PartIter=Parts::iterator;
  struct Row {
    raw_ptr<CanonicalCookie> cookie=nullptr;
    raw_ptr<std::unique_ptr<CanonicalCookie>> owner=nullptr;
    raw_ptr<Map> map=nullptr;
    std::optional<Iter> iterator;
    std::optional<PartIter> partition;
  };
  struct Cursor {
    int kind=0;
    bool exhausted=true;
    raw_ptr<Map> map=nullptr;
    std::optional<Iter> current,end,inner;
    std::optional<PartIter> part_current,part_end,partition;
    std::vector<int64_t> ids;
    size_t position=0;
    std::optional<decltype(CookieMonster::tasks_pending_for_key_)::iterator> task_current,task_end;
    std::optional<decltype(CookieMonster::bytes_per_cookie_partition_)::iterator> byte_current,byte_end;
  };
  raw_ptr<CookieMonster> monster=nullptr;
  bool read_only=false;
  base::WeakPtr<CookieMonster> alive;
  cookie_family::CanonicalContext canonical;
  std::vector<Row> rows;
  std::map<const CanonicalCookie*,int64_t> row_ids;
  std::deque<std::optional<CookiePartitionKey>> partitions;
  std::deque<Cursor> cursors;
  std::list<std::unique_ptr<CanonicalCookie>> owned;
  std::optional<PartIter> selected_partition;
  int64_t selected_cookie=0;
  std::string domain,key,scratch;
  std::string cached_target_value;
  std::optional<int64_t> cached_target;
  raw_ptr<const GURL> url=nullptr;
  raw_ptr<const CookieOptions> options=nullptr;
  raw_ptr<const CookiePartitionKeyCollection> collection=nullptr;
  raw_ptr<const CookiePartitionKey> input_partition=nullptr;
  raw_ptr<const CookieDeletionInfo> deletion_info=nullptr;
  raw_ptr<const CookieList> input_list=nullptr;
  raw_ptr<const std::vector<std::unique_ptr<CanonicalCookie>>> loaded_list=nullptr;
  raw_ptr<std::vector<std::unique_ptr<CanonicalCookie>>> mutable_loaded_list=nullptr;
  raw_ptr<std::vector<CanonicalCookie*>> input_pointers=nullptr;
  std::vector<CanonicalCookie*> pointers;
  std::vector<int64_t> sorted_ids,kept_ids;
  raw_ptr<Map> input_map=nullptr;
  std::optional<CookieMonster::CookieMapItPair> input_range;
  raw_ptr<CookieMonster::CookieItVector> input_vector=nullptr;
  raw_ptr<CookieMonster::CookieItList> input_iterator_list=nullptr;
  std::vector<Iter> iterator_input;
  std::vector<Iter> iterator_output;
  std::map<int64_t,size_t> vector_positions;
  std::map<int64_t,CookieMonster::CookieItList::iterator> list_positions;
  std::optional<std::variant<Iter,CookieMonster::PartitionedCookieMapIterators>> inserted;
  raw_ptr<std::map<UniqueCookieKey,std::pair<base::Time,UniqueCookieKey>>> recent_output=nullptr;
  CookieList cookies_output;
  CookieAccessResultList included_output,excluded_output;
  raw_ptr<CookieAccessResultList> included_target=nullptr;
  raw_ptr<CookieAccessResultList> excluded_target=nullptr;
  std::vector<CookieAccessSemantics> semantics_output;
  std::array<int64_t,80> argument{};
  std::array<CookieAccessResult,3> access;
  raw_ptr<CookieInclusionStatus> status_reference=nullptr;
  std::optional<CookieAccessResult> prior_access;
  CookieAccessParams params{CookieAccessSemantics::UNKNOWN,CookieScopeSemantics::UNKNOWN,false};
  bool trusted=false;
  bool canceled=false;
  std::exception_ptr exception;
  std::optional<base::ElapsedTimer> timer;
  base::TimeTicks load_begin;
  base::Time inherited_creation,earliest_output;
  base::Time record_creation;
  std::optional<int64_t> skipped_secure;
  std::vector<std::string> schemes;
  base::OnceClosure closure,active_task;
  CookieStore::SetCookiesCallback set_callback;
  CookieStore::GetAllCookiesCallback all_callback;
  CookieStore::GetAllCookiesWithAccessSemanticsCallback semantics_callback;
  CookieStore::GetCookieListCallback list_callback;
  CookieStore::DeleteCallback delete_callback;
  CookieStore::SetCookieableSchemesCallback schemes_callback;
  CookieStore::DeletePredicate predicate;
  explicit CookieMonsterRustNative(CookieMonster*);
  explicit CookieMonsterRustNative(const CookieMonster*);
  ~CookieMonsterRustNative();
  static void BeforeDestruction(const CookieMonster*);
  static CookieMonsterRustNative*& Active();
  void ClearObservations();
  raw_ptr<CookieMonsterRustNative> previous = nullptr;
  int64_t Add(CanonicalCookie*);
  int64_t Add(Iter,Map*,std::optional<PartIter>);
  int64_t Own(std::unique_ptr<CanonicalCookie>);
  int64_t Part(const CookiePartitionKey&);
  Row& Get(int64_t);
  const std::optional<CookiePartitionKey>& Partition(int64_t);
  CookieFamilyResult Run(uint32_t,const CookieFamilyRequest& = {});
  void Primitive(const CookieFamilyCall&,CookieFamilyNativeResult&);
  void Metric(int64_t,int64_t);
  void Log(int64_t,int64_t);
  static void Callback(void*,const CookieFamilyCall*,CookieFamilyNativeResult*);
};
}  // namespace net
#endif
