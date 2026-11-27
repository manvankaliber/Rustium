// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// STAGED DRAFT: not compiled or integrated.
#include "net/dns/host_resolver_cache.h"

#include <compare>
#include <limits>
#include <optional>
#include <string_view>
#include <utility>

#include "base/check_op.h"
#include "base/memory/raw_ptr_exclusion.h"
#include "base/notreached.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/tick_clock.h"
#include "net/dns/host_resolver_cache_rust_ffi.h"
#include "net/dns/host_resolver_internal_result.h"
#include "url/third_party/mozilla/url_parse.h"
#include "url/url_canon.h"
#include "url/url_canon_stdstring.h"

namespace net {
namespace {
using V = HostResolverCacheRustValues;
using Request = HostResolverCacheRustRequest;
using Op = HostResolverCacheRustOp;
extern "C" V HostResolverCacheRustService(void*, uint32_t, Request);
static_assert(static_cast<int>(DnsQueryType::UNSPECIFIED) == 0);
static_assert(static_cast<int>(HostResolverSource::ANY) == 0);
static_assert(static_cast<int>(HostResolverSource::SYSTEM) == 1);
static_assert(static_cast<int>(HostResolverSource::DNS) == 2);
static_assert(static_cast<int>(HostResolverSource::MULTICAST_DNS) == 3);
static_assert(static_cast<int>(HostResolverSource::LOCAL_ONLY) == 4);
static_assert(static_cast<int>(HostResolverSource::MAX) == 4);
static_assert(handles::kInvalidNetworkHandle == -1);
static_assert(static_cast<int>(url::CanonHostInfo::Family::NEUTRAL) == 0);

struct NakComparison {
  RAW_PTR_EXCLUSION const NetworkAnonymizationKey* left;
  RAW_PTR_EXCLUSION const NetworkAnonymizationKey* right;
};
extern "C" int32_t CompareNak(const void* opaque) {
  const auto& pair = *static_cast<const NakComparison*>(opaque);
  const auto order = *pair.left <=> *pair.right;
  return order < 0 ? -1 : order > 0 ? 1 : 0;
}
bool KeyLess(std::string_view left_domain,
             const NetworkAnonymizationKey& left_key,
             handles::NetworkHandle left_network,
             std::string_view right_domain,
             const NetworkAnonymizationKey& right_key,
             handles::NetworkHandle right_network) {
  const NakComparison pair{&left_key, &right_key};
  return host_resolver_cache_rust_key_less(
      left_domain.compare(right_domain), &pair, CompareNak, left_network,
      right_network);
}
}  // namespace

// Invocation-local ownership and primitive executor. Readonly invocations never
// receive a mutable cache pointer. There is no persistent Rust state or C++
// policy selector; Rust requests each traversal/filter/erase/Value operation.
struct HostResolverCacheRustContext {
  using Cache = HostResolverCache;
  using Iterator = Cache::EntryMap::const_iterator;
  RAW_PTR_EXCLUSION const Cache* cache = nullptr;
  RAW_PTR_EXCLUSION Cache* writable = nullptr;
  RAW_PTR_EXCLUSION const Cache::Entry* direct_entry = nullptr;
  std::string_view domain;
  RAW_PTR_EXCLUSION const NetworkAnonymizationKey* nak = nullptr;
  handles::NetworkHandle network = handles::kInvalidNetworkHandle;
  DnsQueryType query_type = DnsQueryType::UNSPECIFIED;
  HostResolverSource source = HostResolverSource::ANY;
  std::optional<bool> secure;
  std::string canonicalized;
  std::optional<url::StdStringCanonOutput> canonical_output;
  url::CanonHostInfo host_info;
  Iterator range_first;
  Iterator range_reverse;
  Iterator cursor;
  std::vector<Iterator> retained;
  std::optional<Iterator> best;
  std::unique_ptr<HostResolverInternalResult> pending;
  RAW_PTR_EXCLUSION const HostResolverInternalResult* selected = nullptr;
  base::ListValue list;
  base::DictValue dict;
  base::DictValue logging_dict;
  base::Value nak_value;
  base::Value result_value;
  RAW_PTR_EXCLUSION const base::Value* restore_input = nullptr;
  RAW_PTR_EXCLUSION const base::ListValue* restore_list = nullptr;
  RAW_PTR_EXCLUSION const base::DictValue* restore_dict = nullptr;
  RAW_PTR_EXCLUSION const base::Value* restore_nak_field = nullptr;
  std::optional<NetworkAnonymizationKey> restored_nak;
  std::unique_ptr<HostResolverInternalResult> restored_result;

  explicit HostResolverCacheRustContext(const Cache& value) : cache(&value) {}
  explicit HostResolverCacheRustContext(Cache& value)
      : cache(&value), writable(&value) {}
  explicit HostResolverCacheRustContext(const Cache::Entry& value)
      : direct_entry(&value) {}

  void Configure(std::string_view name,
                 const NetworkAnonymizationKey& key,
                 handles::NetworkHandle target,
                 DnsQueryType query,
                 HostResolverSource from,
                 std::optional<bool> security) {
    domain = name;
    nak = &key;
    network = target;
    query_type = query;
    source = from;
    secure = security;
  }

  const Cache::Entry& EntryAt(uint64_t id) const {
    if (direct_entry) return *direct_entry;
    if (id == 0) return cursor->second;
    if (id == std::numeric_limits<uint64_t>::max()) return best.value()->second;
    return retained.at(base::checked_cast<size_t>(id - 1))->second;
  }
  const Cache::Key& CurrentKey() const { return cursor->first; }
  Cache& Mutable() const { CHECK(writable); return *writable; }

  V Run(uint32_t method, V args = {}) {
    return host_resolver_cache_rust_run(this, HostResolverCacheRustService, method, args);
  }

  static V Dispatch(void* opaque, uint32_t operation, Request request) {
    auto& s = *static_cast<HostResolverCacheRustContext*>(opaque);
    const V v = request.values;
    const std::string_view bytes = request.length == 0
        ? std::string_view()
        : std::string_view(reinterpret_cast<const char*>(request.data),
                           request.length);
    switch (static_cast<Op>(operation)) {
      case Op::kSize: return {.a = s.cache->entries_.size()};
      case Op::kCapacity: return {.a = s.cache->max_entries_};
      case Op::kGeneration: return {.x = s.cache->staleness_generation_};
      case Op::kSetGeneration:
        s.Mutable().staleness_generation_ = static_cast<int>(v.x); return {};
      case Op::kCanonicalize:
        s.canonical_output.emplace(&s.canonicalized);
        url::CanonicalizeHostVerbose(s.domain, url::Component(0, s.domain.size()),
                                    &*s.canonical_output, &s.host_info);
        return {.x = static_cast<int64_t>(s.host_info.family)};
      case Op::kCompleteCanonical: s.canonical_output->Complete(); return {};
      case Op::kOpenRange: {
        const std::string_view name = v.a != 0 ? s.canonicalized : s.domain;
        auto range = s.cache->entries_.equal_range(
            Cache::KeyRef{name, raw_ref(*s.nak), s.network});
        s.range_first = range.first; s.range_reverse = range.second; return {};
      }
      case Op::kStepBack:
        if (s.range_reverse == s.range_first) return {};
        --s.range_reverse; s.cursor = s.range_reverse; return {.a = 1};
      case Op::kQuery:
        return {.a = static_cast<uint64_t>(s.query_type),
                .b = s.secure ? (*s.secure ? 2u : 1u) : 0u,
                .x = static_cast<int64_t>(s.source)};
      case Op::kEntry: {
        const auto& e = s.EntryAt(v.a);
        return {.a = static_cast<uint64_t>(e.result->query_type()),
                .b = static_cast<uint64_t>(e.secure),
                .x = static_cast<int64_t>(e.source), .y = e.staleness_generation};
      }
      case Op::kRetain:
        s.retained.push_back(s.cursor); return {.a = s.retained.size()};
      case Op::kRetainedCount: return {.a = s.retained.size()};
      case Op::kCloseMatching:
        s.canonical_output.reset(); std::string().swap(s.canonicalized); return {};
      case Op::kTickNow: return {.x = s.cache->tick_clock_->NowTicks().ToInternalValue()};
      case Op::kWallNow: return {.x = s.cache->clock_->Now().ToInternalValue()};
      case Op::kAssertTimed:
        DCHECK(s.EntryAt(v.a).result->timed_expiration().has_value()); return {};
      case Op::kExpiration:
        return {.a = static_cast<uint64_t>(s.EntryAt(v.a).result->expiration().has_value())};
      case Op::kDifference: {
        const auto& result = *s.EntryAt(v.a).result;
        // Rust selects the time domain. Native base operators retain exact
        // sentinel/saturation/diagnostic behavior at this scalar boundary.
        const base::TimeDelta delta = v.b != 0
            ? result.expiration().value() - base::TimeTicks::FromInternalValue(v.x)
            : result.timed_expiration().value() - base::Time::FromInternalValue(v.y);
        return {.x = delta.ToInternalValue()};
      }
      case Op::kMagnitude:
        return {.x = base::TimeDelta::FromInternalValue(v.x).magnitude().ToInternalValue()};
      case Op::kSelectResult:
        s.selected = s.EntryAt(v.a).result.get(); return {};
      case Op::kPreparePending:
        DCHECK(s.pending); DCHECK(s.pending->timed_expiration().has_value());
        s.domain = s.pending->domain_name(); s.query_type = s.pending->query_type();
        return {};
      case Op::kDropPending: s.pending.reset(); return {};
      case Op::kEraseRetained:
        s.Mutable().entries_.erase(s.retained.at(base::checked_cast<size_t>(v.a - 1)));
        return {};
      case Op::kInsertPending: {
        std::string name = s.pending->domain_name();
        s.Mutable().entries_.emplace(
            Cache::Key(std::move(name), *s.nak, s.network),
            Cache::Entry(std::move(s.pending), s.source, s.secure.value(),
                         static_cast<int>(v.x)));
        return {};
      }
      case Op::kBeginEntries: s.cursor = s.cache->entries_.cbegin(); s.best.reset(); return {};
      case Op::kCursorValid: return {.a = static_cast<uint64_t>(s.cursor != s.cache->entries_.cend())};
      case Op::kCursorNext: ++s.cursor; return {};
      case Op::kEraseCursor:
        s.cursor = s.Mutable().entries_.erase(s.cursor); return {};
      case Op::kKeepBest: s.best = s.cursor; return {};
      case Op::kEraseBest: s.Mutable().entries_.erase(s.best.value()); return {};
      case Op::kCheckBest: CHECK(s.best.has_value()); return {};
      case Op::kCheckCapacity:
        CHECK_LE(s.cache->entries_.size(), s.cache->max_entries_); return {};
      case Op::kListReset: s.list = base::ListValue(); return {};
      case Op::kDictReset: s.dict = base::DictValue(); return {};
      case Op::kDictInt: s.dict.Set(bytes, static_cast<int>(v.x)); return {};
      case Op::kDictBool: s.dict.Set(bytes, v.a != 0); return {};
      case Op::kNakValue:
        s.nak_value = base::Value();
        return {.a = static_cast<uint64_t>(s.CurrentKey().network_anonymization_key.ToValue(&s.nak_value))};
      case Op::kNakDebugValue:
        s.nak_value = base::Value(std::string(bytes) +
            s.CurrentKey().network_anonymization_key.ToDebugString()); return {};
      case Op::kDictNak: s.dict.Set(bytes, std::move(s.nak_value)); return {};
      case Op::kResultValue: s.result_value = s.cursor->second.result->ToValue(); return {};
      case Op::kDictResult: s.dict.Set(bytes, std::move(s.result_value)); return {};
      case Op::kNetwork: return {.x = s.CurrentKey().target_network};
      case Op::kDictNetwork: s.dict.Set(bytes, base::NumberToString(v.x)); return {};
      case Op::kListAppend: s.list.Append(std::move(s.dict)); return {};
      case Op::kLoggingReset: s.logging_dict = base::DictValue(); return {};
      case Op::kCapacityInt: return {.x = base::checked_cast<int>(s.cache->max_entries_)};
      case Op::kLoggingInt: s.logging_dict.Set(bytes, static_cast<int>(v.x)); return {};
      case Op::kLoggingEntries:
        s.logging_dict.Set(bytes, base::Value(std::move(s.list))); return {};
      case Op::kRestoreList:
        s.restore_list = s.restore_input->GetIfList();
        return {.a = static_cast<uint64_t>(s.restore_list != nullptr),
                .b = s.restore_list ? s.restore_list->size() : 0};
      case Op::kRestoreDict:
        s.restore_dict = (*s.restore_list)[base::checked_cast<size_t>(v.a)].GetIfDict();
        return {.a = static_cast<uint64_t>(s.restore_dict != nullptr)};
      case Op::kRestoreNakField:
        s.restore_nak_field = s.restore_dict->Find(bytes);
        return {.a = static_cast<uint64_t>(s.restore_nak_field != nullptr)};
      case Op::kRestoreNak:
        s.restored_nak.emplace();
        return {.a = static_cast<uint64_t>(NetworkAnonymizationKey::FromValue(
            *s.restore_nak_field, &*s.restored_nak))};
      case Op::kRestoreSource: {
        const base::Value* value = s.restore_dict->Find(bytes);
        const std::optional<int> number = value ? value->GetIfInt() : std::nullopt;
        return {.a = static_cast<uint64_t>(number.has_value()), .x = number.value_or(0)};
      }
      case Op::kRestoreSecure: {
        const auto value = s.restore_dict->FindBool(bytes);
        return {.a = static_cast<uint64_t>(value.has_value()), .b = static_cast<uint64_t>(value.value_or(false))};
      }
      case Op::kRestoreResult: {
        const base::Value* value = s.restore_dict->Find(bytes);
        s.restored_result = value ? HostResolverInternalResult::FromValue(*value) : nullptr;
        return {.a = static_cast<uint64_t>(s.restored_result != nullptr),
                .b = static_cast<uint64_t>(s.restored_result && s.restored_result->timed_expiration().has_value())};
      }
      case Op::kRestorePending:
        s.pending = std::move(s.restored_result); s.nak = &*s.restored_nak;
        s.network = v.y; s.source = static_cast<HostResolverSource>(v.x);
        s.secure = v.a != 0; return {};
      case Op::kRestoreReset:
        s.restored_result.reset(); s.restored_nak.reset(); return {};
    }
    NOTREACHED();
  }
};

namespace {
extern "C" V HostResolverCacheRustService(void* context, uint32_t op, Request request) {
  return HostResolverCacheRustContext::Dispatch(context, op, request);
}
}  // namespace

HostResolverCache::Key::~Key() = default;
bool HostResolverCache::KeyComparator::operator()(const Key& a, const Key& b) const {
  return KeyLess(a.domain_name, a.network_anonymization_key, a.target_network,
                 b.domain_name, b.network_anonymization_key, b.target_network);
}
bool HostResolverCache::KeyComparator::operator()(const Key& a, const KeyRef& b) const {
  return KeyLess(a.domain_name, a.network_anonymization_key, a.target_network,
                 b.domain_name, *b.network_anonymization_key, b.target_network);
}
bool HostResolverCache::KeyComparator::operator()(const KeyRef& a, const Key& b) const {
  return KeyLess(a.domain_name, *a.network_anonymization_key, a.target_network,
                 b.domain_name, b.network_anonymization_key, b.target_network);
}
HostResolverCache::StaleLookupResult::StaleLookupResult(
    const HostResolverInternalResult& result, std::optional<base::TimeDelta> expired_by,
    bool stale_by_generation)
    : result(result), expired_by(expired_by), stale_by_generation(stale_by_generation) {}
HostResolverCache::HostResolverCache(size_t max_results, const base::Clock& clock,
                                   const base::TickClock& tick_clock)
    : max_entries_(max_results), clock_(clock), tick_clock_(tick_clock) {
  DCHECK(host_resolver_cache_rust_capacity_valid(max_entries_));
}
HostResolverCache::~HostResolverCache() = default;
HostResolverCache::HostResolverCache(HostResolverCache&&) = default;
HostResolverCache& HostResolverCache::operator=(HostResolverCache&&) = default;

const HostResolverInternalResult* HostResolverCache::Lookup(
    std::string_view name, const NetworkAnonymizationKey& key,
    handles::NetworkHandle target, DnsQueryType type, HostResolverSource source,
    std::optional<bool> secure) const {
  HostResolverCacheRustContext context(*this);
  context.Configure(name, key, target, type, source, secure);
  context.Run(2); return context.selected;
}
std::optional<HostResolverCache::StaleLookupResult> HostResolverCache::LookupStale(
    std::string_view name, const NetworkAnonymizationKey& key,
    handles::NetworkHandle target, DnsQueryType type, HostResolverSource source,
    std::optional<bool> secure) const {
  HostResolverCacheRustContext context(*this);
  context.Configure(name, key, target, type, source, secure);
  const V result = context.Run(3);
  if (result.a == 0) return std::nullopt;
  const std::optional<base::TimeDelta> expired = result.b != 0
      ? std::make_optional(base::TimeDelta::FromInternalValue(result.x)) : std::nullopt;
  return StaleLookupResult(*context.selected, expired, result.y != 0);
}
void HostResolverCache::Set(std::unique_ptr<HostResolverInternalResult> result,
    const NetworkAnonymizationKey& key, handles::NetworkHandle target,
    HostResolverSource source, bool secure) {
  Set(std::move(result), key, target, source, secure, true, staleness_generation_);
}
void HostResolverCache::Set(std::unique_ptr<HostResolverInternalResult> result,
    const NetworkAnonymizationKey& key, handles::NetworkHandle target,
    HostResolverSource source, bool secure, bool replace_existing, int generation) {
  HostResolverCacheRustContext context(*this);
  context.Configure({}, key, target, DnsQueryType::UNSPECIFIED, source, secure);
  context.pending = std::move(result);
  context.Run(4, {.a = static_cast<uint64_t>(replace_existing), .x = generation});
}
void HostResolverCache::MakeAllResultsStale() {
  HostResolverCacheRustContext context(*this); context.Run(9);
}
base::Value HostResolverCache::Serialize() const { return SerializeEntries(false); }
base::Value HostResolverCache::SerializeEntries(bool logging) const {
  HostResolverCacheRustContext context(*this);
  context.Run(6, {.a = static_cast<uint64_t>(logging)});
  return base::Value(std::move(context.list));
}
base::Value HostResolverCache::SerializeForLogging() const {
  HostResolverCacheRustContext context(*this); context.Run(7);
  return base::Value(std::move(context.logging_dict));
}
bool HostResolverCache::RestoreFromValue(const base::Value& value) {
  HostResolverCacheRustContext context(*this); context.restore_input = &value;
  return context.Run(8).a != 0;
}
void HostResolverCache::EvictEntries() {
  HostResolverCacheRustContext context(*this); context.Run(5);
}
std::vector<HostResolverCache::EntryMap::const_iterator>
HostResolverCache::LookupInternal(std::string_view name,
    const NetworkAnonymizationKey& key, handles::NetworkHandle target,
    DnsQueryType type, HostResolverSource source, std::optional<bool> secure) const {
  HostResolverCacheRustContext context(*this);
  context.Configure(name, key, target, type, source, secure);
  context.Run(1); return std::move(context.retained);
}
bool HostResolverCache::AtMaxSizeForTesting() const {
  return host_resolver_cache_rust_at_capacity(entries_.size(), max_entries_);
}
bool HostResolverCache::RustStaleFlags(bool generation, bool expired) {
  return host_resolver_cache_rust_stale_result(generation, expired);
}
HostResolverCache::Entry::Entry(std::unique_ptr<HostResolverInternalResult> value,
    HostResolverSource source, bool secure, int generation)
    : result(std::move(value)), source(source), secure(secure), staleness_generation(generation) {}
HostResolverCache::Entry::~Entry() = default;
HostResolverCache::Entry::Entry(Entry&&) = default;
HostResolverCache::Entry& HostResolverCache::Entry::operator=(Entry&&) = default;
bool HostResolverCache::Entry::IsStale(base::Time now, base::TimeTicks ticks,
                                    int generation) const {
  HostResolverCacheRustContext context(*this);
  return context.Run(11, {.a = static_cast<uint64_t>(ticks.ToInternalValue()),
                          .b = static_cast<uint64_t>(now.ToInternalValue()),
                          .x = generation}).a != 0;
}
base::TimeDelta HostResolverCache::Entry::TimeUntilExpiration(
    base::Time now, base::TimeTicks ticks) const {
  HostResolverCacheRustContext context(*this);
  return base::TimeDelta::FromInternalValue(context.Run(10,
      {.a = static_cast<uint64_t>(ticks.ToInternalValue()),
       .b = static_cast<uint64_t>(now.ToInternalValue())}).x);
}
}  // namespace net
