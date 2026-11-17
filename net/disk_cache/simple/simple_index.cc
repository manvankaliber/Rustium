// Copyright 2013 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/disk_cache/simple/simple_index.h"

#include <limits>
#include <optional>
#include <string>
#include <utility>

#include "base/check_op.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/numerics/safe_conversions.h"
#include "base/pickle.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_tokenizer.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/task_runner.h"
#include "base/time/time.h"
#include "base/trace_event/memory_usage_estimator.h"
#include "base/trace_event/trace_event.h"
#include "build/build_config.h"
#include "net/base/features.h"
#include "net/base/net_errors.h"
#include "net/disk_cache/backend_cleanup_tracker.h"
#include "net/disk_cache/memory_entry_data_hints.h"
#include "net/disk_cache/simple/disk_simple_index_rust.h"
#include "net/disk_cache/simple/simple_entry_format.h"
#include "net/disk_cache/simple/simple_histogram_macros.h"
#include "net/disk_cache/simple/simple_index_delegate.h"
#include "net/disk_cache/simple/simple_index_file.h"
#include "net/disk_cache/simple/simple_synchronous_entry.h"
#include "net/disk_cache/simple/simple_util.h"

#if BUILDFLAG(IS_POSIX)
#include <sys/stat.h>
#include <sys/time.h>
#endif

namespace disk_cache {

EntryMetadata::EntryMetadata()
    : last_used_time_seconds_since_epoch_(0),
      entry_size_256b_chunks_(0),
      in_memory_data_(0) {}

EntryMetadata::EntryMetadata(base::Time last_used_time,
                             base::StrictNumeric<uint64_t> entry_size)
    : last_used_time_seconds_since_epoch_(0),
      entry_size_256b_chunks_(0),
      in_memory_data_(0) {
  CHECK(SetEntrySize(entry_size))
      << "Failed to create EntryMetadata due to too large entry_size: "
      << static_cast<uint64_t>(entry_size);

  SetLastUsedTime(last_used_time);
}

EntryMetadata::EntryMetadata(uint32_t trailer_prefetch_size,
                             base::StrictNumeric<uint64_t> entry_size)
    : trailer_prefetch_size_(0),
      entry_size_256b_chunks_(0),
      in_memory_data_(0) {
  CHECK(SetEntrySize(entry_size))
      << "Failed to create EntryMetadata due to too large entry_size: "
      << static_cast<uint64_t>(entry_size);

  SetTrailerPrefetchSize(trailer_prefetch_size);
}

base::Time EntryMetadata::GetLastUsedTime() const {
  // Preserve nullity.
  if (last_used_time_seconds_since_epoch_ == 0) {
    return base::Time();
  }

  return base::Time::UnixEpoch() +
         base::Seconds(last_used_time_seconds_since_epoch_);
}

void EntryMetadata::SetLastUsedTime(base::Time last_used_time) {
  last_used_time_seconds_since_epoch_ = rustium_simple_index_metadata(
      4, last_used_time.is_null(),
      (last_used_time - base::Time::UnixEpoch()).InSeconds(), 0);
}

uint32_t EntryMetadata::GetTrailerPrefetchSize() const {
  return trailer_prefetch_size_;
}

void EntryMetadata::SetTrailerPrefetchSize(uint32_t size) {
  trailer_prefetch_size_ =
      rustium_simple_index_metadata(5, trailer_prefetch_size_, size, 0);
}

uint64_t EntryMetadata::GetEntrySize() const {
  return static_cast<uint64_t>(entry_size_256b_chunks_) << 8;
}

bool EntryMetadata::SetEntrySize(base::StrictNumeric<uint64_t> entry_size) {
  const uint64_t chunks =
      rustium_simple_index_metadata(0, static_cast<uint64_t>(entry_size), 0, 0);
  if (chunks == std::numeric_limits<uint64_t>::max()) {
    return false;
  }
  entry_size_256b_chunks_ = chunks;
  return true;
}

uint8_t EntryMetadata::GetInMemoryData() const {
  return in_memory_data_;
}

void EntryMetadata::SetInMemoryData(uint8_t val) {
  // Memory data should only use 2 bits.
  CHECK_LE(val, 3);

  in_memory_data_ = val;
}

// Pickle and base::Time primitives. Rust decides read ordering, validation,
// APP_CACHE interpretation and mutation; failed reads never alter metadata.
struct DiskSimpleIndexMetadataServices {
  STACK_ALLOCATED();

 public:
  DiskSimpleIndexMetadataServices(EntryMetadata* entry,
                                  net::CacheType cache_type)
      : metadata(entry), type(cache_type) {}
  raw_ptr<EntryMetadata> metadata;
  net::CacheType type;
  raw_ptr<base::Pickle> writer = nullptr;
  raw_ptr<base::PickleIterator> reader = nullptr;
  bool has_prefetch = false;
  int64_t time = 0;
  uint64_t packed = 0;
  static uint64_t Dispatch(void* context, uint32_t op, uint64_t a, uint64_t b) {
    using namespace index_rust;
    auto& s = *static_cast<DiskSimpleIndexMetadataServices*>(context);
    switch (op) {
      case kMetaCacheType:
        return s.type;
      case kMetaHasPrefetch:
        return s.has_prefetch;
      case kMetaReadTime:
        return s.reader->ReadInt64(&s.time);
      case kMetaReadPacked:
        return s.reader->ReadUInt64(&s.packed);
      case kMetaTimeValue:
        return s.time;
      case kMetaPackedValue:
        return s.packed;
      case kMetaBits:
        return uint64_t{s.metadata->last_used_time_seconds_since_epoch_} |
               (uint64_t{s.metadata->entry_size_256b_chunks_} << 32) |
               (uint64_t{s.metadata->in_memory_data_} << 62);
      case kMetaSetBits:
        s.metadata->last_used_time_seconds_since_epoch_ = a;
        s.metadata->entry_size_256b_chunks_ = a >> 32;
        s.metadata->in_memory_data_ = a >> 62;
        return 0;
      case kMetaStoredFromInternal: {
        EntryMetadata m;
        m.SetLastUsedTime(base::Time::FromInternalValue(a));
        return m.RawTimeForSorting();
      }
      case kMetaInternalFromStored: {
        EntryMetadata m;
        m.last_used_time_seconds_since_epoch_ = a;
        return m.GetLastUsedTime().ToInternalValue();
      }
      case kMetaWriteTime:
        s.writer->WriteInt64(a);
        return 0;
      case kMetaWritePacked:
        s.writer->WriteUInt64(a);
        return 0;
      default:
        NOTREACHED();
    }
  }
};
void EntryMetadata::Serialize(net::CacheType cache_type,
                              base::Pickle* pickle) const {
  DCHECK(pickle);
  DiskSimpleIndexMetadataServices context{const_cast<EntryMetadata*>(this),
                                          cache_type};
  context.writer = pickle;
  rustium_simple_index_run(&context, &DiskSimpleIndexMetadataServices::Dispatch,
                           index_rust::kOpSerialize, 0, 0);
}
bool EntryMetadata::Deserialize(net::CacheType cache_type,
                                base::PickleIterator* it,
                                bool app_cache_has_trailer_prefetch_size) {
  DCHECK(it);
  DiskSimpleIndexMetadataServices context{this, cache_type};
  context.reader = it;
  context.has_prefetch = app_cache_has_trailer_prefetch_size;
  return rustium_simple_index_run(&context,
                                  &DiskSimpleIndexMetadataServices::Dispatch,
                                  index_rust::kOpDeserialize, 0, 0);
}

// Primitive services only. All decisions, iteration, ranking, merge and
// callback draining live in Rust. The stack context survives owner deletion. It
// never retains a map iterator across a service that can invoke user code.
struct DiskSimpleIndexRustServices {
  explicit DiskSimpleIndexRustServices(SimpleIndex* index)
      : owner(index), weak(index->weak_ptr_factory_.GetWeakPtr()) {}
  raw_ptr<SimpleIndex> owner;
  base::WeakPtr<SimpleIndex> weak;
  raw_ptr<SimpleIndexLoadResult> load = nullptr;
  raw_ptr<SimpleIndex::HashList> output = nullptr;
  net::CompletionOnceCallback ready;
  std::optional<SimpleIndex::EntrySet::const_iterator> cursor;
  std::optional<SimpleIndex::EntrySet::const_iterator> end;
  std::optional<absl::flat_hash_set<uint64_t>::const_iterator> removed_cursor;
  bool first = false;
  bool removed_first = false;
  std::vector<uint64_t> doomed;

  static uint64_t Encode(const EntryMetadata& m) {
    return uint64_t{m.last_used_time_seconds_since_epoch_} |
           (uint64_t{m.entry_size_256b_chunks_} << 32) |
           (uint64_t{m.in_memory_data_} << 62);
  }
  static EntryMetadata Decode(uint64_t bits) {
    EntryMetadata m;
    m.last_used_time_seconds_since_epoch_ = bits;
    m.entry_size_256b_chunks_ = bits >> 32;
    m.in_memory_data_ = bits >> 62;
    return m;
  }
  static uint64_t Dispatch(void* opaque, uint32_t op, uint64_t a, uint64_t b) {
    using namespace index_rust;
    auto& s = *static_cast<DiskSimpleIndexRustServices*>(opaque);
    if (op == kAlive) {
      return !!s.weak;
    }
    if (!s.weak) {
      return 0;
    }
    SimpleIndex* p = s.owner;
    switch (op) {
      case kCacheType:
        return p->cache_type_;
      case kInitialized:
        return p->initialized_;
      case kCacheSize:
        return p->cache_size_;
      case kMaxSize:
        return p->max_size_;
      case kHigh:
        return p->high_watermark_;
      case kLow:
        return p->low_watermark_;
      case kEvicting:
        return p->eviction_in_progress_;
      case kBackground:
        return p->app_on_background_;
      case kPrioritized:
        return p->prioritized_caching_enabled_;
      case kFactor:
        return p->caching_prioritization_factor_;
      case kPeriod:
        return p->caching_prioritization_period_in_seconds_;
      case kSetMax:
        p->max_size_ = a;
        break;
      case kSetHigh:
        p->high_watermark_ = a;
        break;
      case kSetLow:
        p->low_watermark_ = a;
        break;
      case kSetCacheSize:
        p->cache_size_ = a;
        break;
      case kSetInitialized:
        p->initialized_ = a;
        break;
      case kSetInitMethod:
        p->init_method_ = static_cast<SimpleIndex::IndexInitMethod>(a);
        break;
      case kContains:
        return p->entries_set_.contains(a);
      case kMeta: {
        auto it = p->entries_set_.find(a);
        return it == p->entries_set_.end() ? 0 : Encode(it->second);
      }
      case kSetMeta:
        p->entries_set_.find(a)->second = Decode(b);
        break;
      case kInsert:
        return p->entries_set_.emplace(a, Decode(b)).second;
      case kErase:
        p->entries_set_.erase(a);
        break;
      case kRemovedInsert:
        p->removed_entries_.insert(a);
        break;
      case kRemovedErase:
        p->removed_entries_.erase(a);
        break;
      case kRemovedBegin:
        s.removed_cursor = p->removed_entries_.begin();
        s.removed_first = true;
        break;
      case kRemovedNext:
        if (!s.removed_first) {
          ++*s.removed_cursor;
        }
        s.removed_first = false;
        return *s.removed_cursor != p->removed_entries_.end();
      case kRemovedKey:
        return **s.removed_cursor;
      case kRemovedClear:
        s.removed_cursor.reset();
        p->removed_entries_.clear();
        break;
      case kBegin:
        s.cursor = p->entries_set_.begin();
        s.end = p->entries_set_.end();
        s.first = true;
        break;
      case kNext:
      case kLoadedNext:
        if (!s.first) {
          ++*s.cursor;
        }
        s.first = false;
        return *s.cursor != *s.end;
      case kKey:
        return (**s.cursor).first;
      case kValue:
      case kLoadedValue:
        return Encode((**s.cursor).second);
      case kAddress:
        return reinterpret_cast<uintptr_t>(&**s.cursor);
      case kLoadedErase:
        s.load->entries.erase(a);
        break;
      case kLoadedPut:
        s.load->entries.insert_or_assign(a, Decode(b));
        break;
      case kLoadedBegin:
        s.cursor = s.load->entries.begin();
        s.end = s.load->entries.end();
        s.first = true;
        break;
      case kSwapLoaded:
        s.cursor.reset();
        s.end.reset();
        p->entries_set_.swap(s.load->entries);
        break;
      case kLoadMethod:
        return s.load->init_method;
      case kLoadFlush:
        return s.load->flush_required;
      case kNowSeconds:
        return static_cast<uint32_t>(
            (base::Time::Now() - base::Time::UnixEpoch()).InSeconds());
      case kNowStored: {
        EntryMetadata m;
        m.SetLastUsedTime(base::Time::Now());
        return m.RawTimeForSorting();
      }
      case kTimeInternal: {
        EntryMetadata m = Decode(a);
        return m.GetLastUsedTime().ToInternalValue();
      }
      case kRangeLower: {
        base::Time t = base::Time::FromInternalValue(a);
        return (t.is_null() ? t : t - base::Seconds(1)).ToInternalValue();
      }
      case kRangeUpper: {
        base::Time t = base::Time::FromInternalValue(a);
        return (t.is_null() ? base::Time::Max() : t).ToInternalValue();
      }
      case kOutputPush:
        s.output->push_back(a);
        break;
      case kTimerStart:
        p->write_to_disk_timer_.Start(FROM_HERE, base::Milliseconds(a),
                                      p->write_to_disk_cb_);
        break;
      case kTimerStop:
        p->write_to_disk_timer_.Stop();
        break;
      case kTimerRunning:
        return p->write_to_disk_timer_.IsRunning();
      case kLoad: {
        auto result = std::make_unique<SimpleIndexLoadResult>();
        auto* result_ptr = result.get();
        auto cb = base::BindOnce(&SimpleIndex::MergeInitializingSet, s.weak,
                                 std::move(result));
        p->index_file_->LoadIndexEntries(base::Time::FromInternalValue(a),
                                         std::move(cb), result_ptr);
        break;
      }
      case kWrite: {
        base::OnceClosure after_write;
        if (p->cleanup_tracker_) {
          after_write = base::DoNothingWithBoundArgs(p->cleanup_tracker_);
        }
        p->index_file_->WriteToDisk(
            p->cache_type_, static_cast<SimpleIndex::IndexWriteToDiskReason>(a),
            p->entries_set_, p->cache_size_, std::move(after_write));
        break;
      }
      case kEvictionStart:
        p->eviction_in_progress_ = true;
        p->eviction_start_time_ = base::TimeTicks::Now();
        break;
      case kDoomPush:
        s.doomed.push_back(a);
        break;
      case kDoomSubmit:
        p->delegate_->DoomEntries(
            &s.doomed, base::BindOnce(&SimpleIndex::EvictionDone, s.weak));
        break;
      case kEvictionTelemetry:
        SIMPLE_CACHE_UMA(COUNTS_1M, "Eviction.EntryCount", p->cache_type_,
                         s.doomed.size());
        SIMPLE_CACHE_UMA(TIMES, "Eviction.TimeToSelectEntries", p->cache_type_,
                         base::TimeTicks::Now() - p->eviction_start_time_);
        break;
      case kEvictionDoneTelemetry:
        p->eviction_in_progress_ = false;
        SIMPLE_CACHE_UMA(TIMES, "Eviction.TimeToDone", p->cache_type_,
                         base::TimeTicks::Now() - p->eviction_start_time_);
        break;
      case kInitTelemetry:
        SIMPLE_CACHE_UMA(CUSTOM_COUNTS, "IndexNumEntriesOnInit2",
                         p->cache_type_, p->entries_set_.size(), 0, 1000000,
                         50);
        SIMPLE_CACHE_UMA(MEMORY_MEDIUM_MB, "CacheSizeOnInit2", p->cache_type_,
                         static_cast<base::HistogramBase::Sample32>(
                             p->cache_size_ / (1024 * 1024)));
        SIMPLE_CACHE_UMA(MEMORY_MEDIUM_MB, "MaxCacheSizeOnInit2",
                         p->cache_type_,
                         static_cast<base::HistogramBase::Sample32>(
                             p->max_size_ / (1024 * 1024)));
        break;
      case kQueueReady:
        p->to_run_when_initialized_.push_back(std::move(s.ready));
        break;
      case kPostReady:
        p->task_runner_->PostTask(FROM_HERE,
                                  base::BindOnce(std::move(s.ready), net::OK));
        break;
      case kReadyCount:
        return p->to_run_when_initialized_.size();
      case kPostOne: {
        auto cb = std::move(p->to_run_when_initialized_.front());
        p->to_run_when_initialized_.pop_front();
        p->task_runner_->PostTask(FROM_HERE,
                                  base::BindOnce(std::move(cb), net::OK));
        break;
      }
      case kAbortOne: {
        auto cb = std::move(p->to_run_when_initialized_.front());
        p->to_run_when_initialized_.pop_front();
        std::move(cb).Run(net::ERR_ABORTED);
        break;
      }
      case kAssertAccounted:
        DCHECK_GE(p->cache_size_, a);
        break;
      case kInvalidSize:
        LOG(ERROR) << "Could not set the given entry size as it is too large: "
                   << a;
        break;
      default:
        NOTREACHED() << "Unknown SimpleIndex native service " << op;
    }
    return 0;
  }
  uint64_t Run(uint32_t op, uint64_t a = 0, uint64_t b = 0) {
    return rustium_simple_index_run(this, &Dispatch, op, a, b);
  }
};
SimpleIndex::SimpleIndex(
    const scoped_refptr<base::SequencedTaskRunner>& task_runner,
    scoped_refptr<BackendCleanupTracker> cleanup_tracker,
    SimpleIndexDelegate* delegate,
    net::CacheType cache_type,
    std::unique_ptr<SimpleIndexFile> index_file)
    : cleanup_tracker_(std::move(cleanup_tracker)),
      delegate_(delegate),
      cache_type_(cache_type),
      index_file_(std::move(index_file)),
      task_runner_(task_runner),
      prioritized_caching_enabled_(base::FeatureList::IsEnabled(
          net::features::kSimpleCachePrioritizedCaching)),
      caching_prioritization_factor_(
          net::features::kSimpleCachePrioritizedCachingPrioritizationFactor
              .Get()),
      caching_prioritization_period_in_seconds_(static_cast<uint64_t>(
          net::features::kSimpleCachePrioritizedCachingPrioritizationPeriod
              .Get()
              .InSeconds())) {
  // Creating the callback once so it is reused every time
  // write_to_disk_timer_.Start() is called.
  write_to_disk_cb_ = base::BindRepeating(&SimpleIndex::WriteToDisk,
                                          weak_ptr_factory_.GetWeakPtr(),
                                          INDEX_WRITE_REASON_IDLE);
}

SimpleIndex::~SimpleIndex() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DiskSimpleIndexRustServices(this).Run(index_rust::kOpShutdown);
}
void SimpleIndex::Initialize(base::Time cache_mtime) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DiskSimpleIndexRustServices s(const_cast<SimpleIndex*>(this));

  s.Run(index_rust::kOpInitialize, cache_mtime.ToInternalValue());
}
void SimpleIndex::SetMaxSize(uint64_t max_bytes) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DiskSimpleIndexRustServices s(const_cast<SimpleIndex*>(this));

  s.Run(index_rust::kOpSetMax, max_bytes);
}
void SimpleIndex::Insert(uint64_t entry_hash) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DiskSimpleIndexRustServices s(const_cast<SimpleIndex*>(this));

  s.Run(index_rust::kOpInsert, entry_hash);
}
void SimpleIndex::Remove(uint64_t entry_hash) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DiskSimpleIndexRustServices s(const_cast<SimpleIndex*>(this));

  s.Run(index_rust::kOpRemove, entry_hash);
}
bool SimpleIndex::Has(uint64_t entry_hash) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DiskSimpleIndexRustServices s(const_cast<SimpleIndex*>(this));

  return s.Run(index_rust::kOpHas, entry_hash);
}
bool SimpleIndex::UseIfExists(uint64_t entry_hash) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DiskSimpleIndexRustServices s(const_cast<SimpleIndex*>(this));

  return s.Run(index_rust::kOpUse, entry_hash);
}
uint8_t SimpleIndex::GetEntryInMemoryData(uint64_t entry_hash) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DiskSimpleIndexRustServices s(const_cast<SimpleIndex*>(this));

  return s.Run(index_rust::kOpGetMemory, entry_hash);
}
void SimpleIndex::SetEntryInMemoryData(uint64_t entry_hash, uint8_t value) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (entries_set_.contains(entry_hash)) {
    CHECK_LE(value, 3);
  }
  DiskSimpleIndexRustServices s(const_cast<SimpleIndex*>(this));

  s.Run(index_rust::kOpSetMemory, entry_hash, value);
}
int32_t SimpleIndex::GetTrailerPrefetchSize(uint64_t entry_hash) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK_EQ(cache_type_, net::APP_CACHE);
  DiskSimpleIndexRustServices s(const_cast<SimpleIndex*>(this));

  return s.Run(index_rust::kOpGetPrefetch, entry_hash);
}
void SimpleIndex::SetTrailerPrefetchSize(uint64_t entry_hash, int32_t size) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK_EQ(cache_type_, net::APP_CACHE);
  DiskSimpleIndexRustServices s(const_cast<SimpleIndex*>(this));

  s.Run(index_rust::kOpSetPrefetch, entry_hash, static_cast<uint32_t>(size));
}
bool SimpleIndex::UpdateEntrySize(uint64_t entry_hash,
                                  base::StrictNumeric<uint64_t> entry_size) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DiskSimpleIndexRustServices s(const_cast<SimpleIndex*>(this));

  return s.Run(index_rust::kOpUpdateSize, entry_hash,
               static_cast<uint64_t>(entry_size));
}
void SimpleIndex::InsertEntryForTesting(uint64_t entry_hash,
                                        const EntryMetadata& entry_metadata) {
  DCHECK(!entries_set_.contains(entry_hash));
  DiskSimpleIndexRustServices s(const_cast<SimpleIndex*>(this));

  s.Run(index_rust::kOpInsertTest, entry_hash,
        DiskSimpleIndexRustServices::Encode(entry_metadata));
}
void SimpleIndex::ExecuteWhenReady(net::CompletionOnceCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DiskSimpleIndexRustServices s(const_cast<SimpleIndex*>(this));
  s.ready = std::move(callback);
  s.Run(index_rust::kOpReady);
}
void SimpleIndex::PostponeWritingToDisk() {
  DiskSimpleIndexRustServices s(const_cast<SimpleIndex*>(this));

  s.Run(index_rust::kOpPostpone);
}
void SimpleIndex::StartEvictionIfNeeded() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DiskSimpleIndexRustServices s(const_cast<SimpleIndex*>(this));

  s.Run(index_rust::kOpEvict);
}
void SimpleIndex::EvictionDone(int result) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DiskSimpleIndexRustServices s(const_cast<SimpleIndex*>(this));

  s.Run(index_rust::kOpEvictionDone);
}
void SimpleIndex::WriteToDisk(IndexWriteToDiskReason reason) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DiskSimpleIndexRustServices s(const_cast<SimpleIndex*>(this));

  s.Run(index_rust::kOpWrite, reason);
}
bool SimpleIndex::HasPendingWrite() const {
  DiskSimpleIndexRustServices s(const_cast<SimpleIndex*>(this));

  return s.Run(index_rust::kOpPending);
}
void SimpleIndex::MergeInitializingSet(
    std::unique_ptr<SimpleIndexLoadResult> load_result) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  TRACE_EVENT("disk_cache", "SimpleIndex::MergeInitializingSet", "cache_type_",
              cache_type_);
  DiskSimpleIndexRustServices s(const_cast<SimpleIndex*>(this));
  s.load = load_result.get();
  s.Run(index_rust::kOpMerge);
}

std::unique_ptr<SimpleIndex::HashList> SimpleIndex::GetEntriesBetween(
    base::Time initial_time,
    base::Time end_time) {
  DCHECK(initialized_);
  DCHECK(cache_type_ != net::APP_CACHE ||
         (initial_time.is_null() && end_time.is_null()));
  auto result = std::make_unique<HashList>();
  DiskSimpleIndexRustServices s(this);
  s.output = result.get();
  DCHECK_GE(
      base::Time::FromInternalValue(s.Dispatch(&s, index_rust::kRangeUpper,
                                               end_time.ToInternalValue(), 0)),
      base::Time::FromInternalValue(s.Dispatch(
          &s, index_rust::kRangeLower, initial_time.ToInternalValue(), 0)));
  s.Run(index_rust::kOpRange, initial_time.ToInternalValue(),
        end_time.ToInternalValue());
  return result;
}
uint64_t SimpleIndex::GetCacheSizeBetween(base::Time initial_time,
                                          base::Time end_time) const {
  DCHECK(initialized_);
  DiskSimpleIndexRustServices s(const_cast<SimpleIndex*>(this));
  DCHECK_GE(
      base::Time::FromInternalValue(s.Dispatch(&s, index_rust::kRangeUpper,
                                               end_time.ToInternalValue(), 0)),
      base::Time::FromInternalValue(s.Dispatch(
          &s, index_rust::kRangeLower, initial_time.ToInternalValue(), 0)));
  return s.Run(index_rust::kOpSizeRange, initial_time.ToInternalValue(),
               end_time.ToInternalValue());
}
std::unique_ptr<SimpleIndex::HashList> SimpleIndex::GetAllHashes() {
  return GetEntriesBetween(base::Time(), base::Time());
}
int32_t SimpleIndex::GetEntryCount() const {
  return entries_set_.size();
}
uint64_t SimpleIndex::GetCacheSize() const {
  DCHECK(initialized_);
  return cache_size_;
}
base::Time SimpleIndex::GetLastUsedTime(uint64_t entry_hash) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK_NE(cache_type_, net::APP_CACHE);
  DiskSimpleIndexRustServices s(this);
  return DiskSimpleIndexRustServices::Decode(
             s.Run(index_rust::kOpLastUsed, entry_hash))
      .GetLastUsedTime();
}
void SimpleIndex::SetLastUsedTimeForTest(uint64_t entry_hash,
                                         base::Time last_used) {
  CHECK(entries_set_.contains(entry_hash));
  EntryMetadata m;
  m.SetLastUsedTime(last_used);
  DiskSimpleIndexRustServices(this).Run(index_rust::kOpSetLastUsed, entry_hash,
                                        m.RawTimeForSorting());
}
bool SimpleIndex::InsertInEntrySet(uint64_t entry_hash,
                                   const EntryMetadata& entry_metadata,
                                   EntrySet* entry_set) {
  DCHECK(entry_set);
  return entry_set->emplace(entry_hash, entry_metadata).second;
}
}  // namespace disk_cache
