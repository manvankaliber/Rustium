#include <algorithm>
#include <array>
#include <cstdio>
#include <memory>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/pickle.h"
#include "base/test/task_environment.h"
#include "net/disk_cache/backend_cleanup_tracker.h"
#include "net/disk_cache/simple/disk_simple_index_rust_original.h"
#include "net/disk_cache/simple/disk_simple_index_rust_original_file.h"
#include "net/disk_cache/simple/simple_index.h"
#include "net/disk_cache/simple/simple_index_delegate.h"
#include "net/disk_cache/simple/simple_index_file.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace disk_cache {
namespace {
uint64_t Random(uint64_t& x) {
  x ^= x << 13;
  x ^= x >> 7;
  x ^= x << 17;
  return x;
}
template <class Index, class File, class Result>
class Provider final : public File {
 public:
  explicit Provider(net::CacheType type)
      : File(nullptr,
             base::MakeRefCounted<TrivialFileOperationsFactory>(),
             type,
             base::FilePath()) {}
  void LoadIndexEntries(base::Time,
                        base::OnceClosure callback,
                        Result* result) override {
    completion = std::move(callback);
    loading = result;
  }
  void WriteToDisk(net::CacheType,
                   typename Index::IndexWriteToDiskReason reason,
                   const typename Index::EntrySet& entries,
                   uint64_t size,
                   base::OnceClosure callback) override {
    writes.push_back({static_cast<int>(reason), size});
    saved = entries;
    if (callback) {
      std::move(callback).Run();
    }
    // Move before invocation; this action may destroy the provider itself.
    auto action = std::move(on_write);
    if (action) {
      std::move(action).Run();
    }
  }
  void Complete() {
    loading = nullptr;
    auto cb = std::move(completion);
    std::move(cb).Run();
  }
  raw_ptr<Result> loading = nullptr;
  base::OnceClosure completion;
  base::OnceClosure on_write;
  typename Index::EntrySet saved;
  std::vector<std::pair<int, uint64_t>> writes;
};
template <class I, class M, class F, class R>
struct World final : SimpleIndexDelegate {
  using Index = I;
  using Metadata = M;
  using File = Provider<I, F, R>;
  explicit World(net::CacheType type) {
    auto owned_file = std::make_unique<File>(type);
    file = owned_file.get();
    index = std::make_unique<I>(base::SequencedTaskRunner::GetCurrentDefault(),
                                nullptr, this, type, std::move(owned_file));
    index->SetMaxSize(uint64_t{1} << 60);
    index->Initialize(base::Time());
  }
  void DoomEntries(std::vector<uint64_t>* hashes,
                   net::CompletionOnceCallback callback) override {
    doomed = *hashes;
    auto action = std::move(on_doom);
    if (action) {
      std::move(action).Run();
      return;
    }
    for (uint64_t hash : *hashes) {
      index->Remove(hash);
    }
    if (defer_doom) {
      doom_completion = std::move(callback);
    } else {
      std::move(callback).Run(net::OK);
    }
  }
  std::unique_ptr<I> index;
  raw_ptr<File> file;
  std::vector<uint64_t> doomed;
  bool defer_doom = false;
  net::CompletionOnceCallback doom_completion;
  base::OnceClosure on_doom;
};
using Actual =
    World<SimpleIndex, EntryMetadata, SimpleIndexFile, SimpleIndexLoadResult>;
using Original = World<OriginalSimpleIndex,
                       OriginalEntryMetadata,
                       OriginalSimpleIndexFile,
                       OriginalSimpleIndexLoadResult>;

template <class W>
void Populate(W& w, unsigned count, bool flush) {
  using M = typename W::Metadata;
  for (unsigned i = 0; i < count; ++i) {
    M m(base::Time::UnixEpoch() + base::Seconds(i + 1),
        uint64_t{256} * (i + 1));
    m.SetInMemoryData(i & 3);
    w.file->loading->entries.emplace(i, m);
  }
  w.file->loading->init_method = W::Index::INITIALIZE_METHOD_RECOVERED;
  w.file->loading->flush_required = flush;
}
void Compare(Actual& a, Original& b, net::CacheType type) {
  ASSERT_EQ(a.index->initialized(), b.index->initialized());
  ASSERT_EQ(a.index->GetEntryCount(), b.index->GetEntryCount());
  ASSERT_EQ(a.index->HasPendingWrite(), b.index->HasPendingWrite());
  ASSERT_EQ(a.index->max_size(), b.index->max_size());
  ASSERT_EQ(a.file->writes, b.file->writes);
  if (!a.index->initialized()) {
    return;
  }
  ASSERT_EQ(static_cast<int>(a.index->init_method()),
            static_cast<int>(b.index->init_method()));
  ASSERT_EQ(a.index->GetCacheSize(), b.index->GetCacheSize());
  auto ah = a.index->GetAllHashes();
  auto bh = b.index->GetAllHashes();
  std::sort(ah->begin(), ah->end());
  std::sort(bh->begin(), bh->end());
  ASSERT_EQ(*ah, *bh);
  for (uint64_t h : *ah) {
    ASSERT_EQ(a.index->GetEntryInMemoryData(h),
              b.index->GetEntryInMemoryData(h));
    if (type == net::APP_CACHE) {
      ASSERT_EQ(a.index->GetTrailerPrefetchSize(h),
                b.index->GetTrailerPrefetchSize(h));
    } else {
      ASSERT_EQ(a.index->GetLastUsedTime(h), b.index->GetLastUsedTime(h));
    }
  }
  const auto lower = base::Time::UnixEpoch() + base::Milliseconds(1500);
  const auto upper = base::Time::UnixEpoch() + base::Seconds(17);
  ASSERT_EQ(a.index->GetCacheSizeBetween(lower, upper),
            b.index->GetCacheSizeBetween(lower, upper));
  if (type != net::APP_CACHE) {
    ah = a.index->GetEntriesBetween(lower, upper);
    bh = b.index->GetEntriesBetween(lower, upper);
    std::sort(ah->begin(), ah->end());
    std::sort(bh->begin(), bh->end());
    ASSERT_EQ(*ah, *bh);
  }
}

TEST(DiskSimpleIndexRustDifferential, MillionStateOperations) {
  base::test::TaskEnvironment tasks(
      base::test::TaskEnvironment::TimeSource::MOCK_TIME);
  uint64_t random = 0x123456789abcdef0ULL;
  std::array<uint64_t, 13> counts{};
  for (unsigned scenario = 0; scenario < 1024; ++scenario) {
    constexpr std::array<net::CacheType, 6> types = {
        net::DISK_CACHE,
        net::APP_CACHE,
        net::SHADER_CACHE,
        net::GENERATED_BYTE_CODE_CACHE,
        net::GENERATED_NATIVE_CODE_CACHE,
        net::GENERATED_WEBUI_BYTE_CODE_CACHE};
    const net::CacheType type = types[scenario % types.size()];
    Actual a(type);
    Original b(type);
    Populate(a, 8, scenario & 1);
    Populate(b, 8, scenario & 1);
    std::vector<int> ac, bc;
    for (unsigned step = 0; step < 1024; ++step) {
      uint64_t r = Random(random);
      uint64_t key = (r >> 20) % 32;
      uint64_t bytes = (r & 0xffff) + 1;
      const unsigned op = r % counts.size();
      ++counts[op];
      switch (op) {
        case 0:
          a.index->Insert(key);
          b.index->Insert(key);
          break;
        case 1:
          a.index->Remove(key);
          b.index->Remove(key);
          break;
        case 2:
          ASSERT_EQ(a.index->Has(key), b.index->Has(key));
          break;
        case 3:
          ASSERT_EQ(a.index->UseIfExists(key), b.index->UseIfExists(key));
          break;
        case 4:
          ASSERT_EQ(a.index->UpdateEntrySize(key, bytes),
                    b.index->UpdateEntrySize(key, bytes));
          break;
        case 5:
          a.index->SetEntryInMemoryData(key, r & 3);
          b.index->SetEntryInMemoryData(key, r & 3);
          break;
        case 6:
          if (type == net::APP_CACHE) {
            a.index->SetTrailerPrefetchSize(key, static_cast<int32_t>(r >> 32));
            b.index->SetTrailerPrefetchSize(key, static_cast<int32_t>(r >> 32));
          }
          break;
        case 7:
          a.index->WriteToDisk(SimpleIndex::INDEX_WRITE_REASON_IDLE);
          b.index->WriteToDisk(OriginalSimpleIndex::INDEX_WRITE_REASON_IDLE);
          break;
        case 8:
          a.index->ExecuteWhenReady(base::BindOnce(
              [](std::vector<int>* v, int result) { v->push_back(result); },
              &ac));
          b.index->ExecuteWhenReady(base::BindOnce(
              [](std::vector<int>* v, int result) { v->push_back(result); },
              &bc));
          break;
        case 9:
          if (type != net::APP_CACHE && a.index->initialized() &&
              a.index->Has(key)) {
            auto time = base::Time::FromInternalValue(static_cast<int64_t>(r));
            a.index->SetLastUsedTimeForTest(key, time);
            b.index->SetLastUsedTimeForTest(key, time);
          }
          break;
        case 10:
          a.index->SetMaxSize((uint64_t{1} << 60) + bytes);
          b.index->SetMaxSize((uint64_t{1} << 60) + bytes);
          break;
        case 11:
          ASSERT_EQ(a.index->GetEntryInMemoryData(key),
                    b.index->GetEntryInMemoryData(key));
          break;
        case 12:
          ASSERT_EQ(a.index->GetEntryCount(), b.index->GetEntryCount());
          break;
      }
      if (step == 31) {
        a.file->Complete();
        b.file->Complete();
      }
      if ((step & 31) == 0) {
        Compare(a, b, type);
        ASSERT_FALSE(::testing::Test::HasFatalFailure());
      }
      if ((step & 127) == 0) {
        tasks.RunUntilIdle();
        ASSERT_EQ(ac, bc);
      }
    }
    Compare(a, b, type);
    tasks.RunUntilIdle();
    ASSERT_EQ(ac, bc);
    a.index.reset();
    b.index.reset();
  }
  for (unsigned i = 0; i < counts.size(); ++i) {
    std::printf("state_op_%u=%llu\n", i,
                static_cast<unsigned long long>(counts[i]));
  }
}

TEST(DiskSimpleIndexRustDifferential, MillionMetadataInputs) {
  uint64_t random = 0x5a96f8271ULL;
  for (unsigned i = 0; i < 1000000; ++i) {
    uint64_t r = Random(random);
    EntryMetadata a;
    OriginalEntryMetadata b;
    // Include valid sizes as well as arbitrary overflow and invalid sizes.
    uint64_t bytes = i & 1 ? r : r & ((uint64_t{1} << 38) - 1);
    ASSERT_EQ(a.SetEntrySize(bytes), b.SetEntrySize(bytes));
    auto time = base::Time::FromInternalValue(static_cast<int64_t>(r));
    a.SetLastUsedTime(time);
    b.SetLastUsedTime(time);
    a.SetInMemoryData(r & 3);
    b.SetInMemoryData(r & 3);
    a.SetTrailerPrefetchSize(r >> 32);
    b.SetTrailerPrefetchSize(r >> 32);
    net::CacheType type = i & 1 ? net::APP_CACHE : net::DISK_CACHE;
    base::Pickle pa, pb;
    a.Serialize(type, &pa);
    b.Serialize(type, &pb);
    ASSERT_EQ(pa.AsBytes(), pb.AsBytes());
    if (i % 3 == 0) {
      pa.WriteUInt64(r);
      pb.WriteUInt64(r);
    }
    base::Pickle corrupt;
    corrupt.WriteInt64(r);
    if (i % 5 != 0) {
      corrupt.WriteUInt64(i & 1 ? r : r & ((uint64_t{1} << 38) - 1));
    }
    base::PickleIterator ai(i % 3 ? pa : corrupt), bi(i % 3 ? pb : corrupt);
    ASSERT_EQ(a.Deserialize(type, &ai, i & 2), b.Deserialize(type, &bi, i & 2));
    ASSERT_EQ(a.GetEntrySize(), b.GetEntrySize());
    ASSERT_EQ(a.GetInMemoryData(), b.GetInMemoryData());
    ASSERT_EQ(a.GetTrailerPrefetchSize(), b.GetTrailerPrefetchSize());
    ASSERT_EQ(a.GetLastUsedTime(), b.GetLastUsedTime());
    int64_t tail_a = 0, tail_b = 0;
    ASSERT_EQ(ai.ReadInt64(&tail_a), bi.ReadInt64(&tail_b));
    ASSERT_EQ(tail_a, tail_b);
  }
}

TEST(DiskSimpleIndexRustDifferential, ReentryDeletionCancellationAndTimer) {
  base::test::TaskEnvironment tasks(
      base::test::TaskEnvironment::TimeSource::MOCK_TIME);
  Actual a(net::DISK_CACHE);
  Populate(a, 8, false);
  int ready = 0;
  a.index->ExecuteWhenReady(base::BindOnce(
      [](Actual* w, int* count, int result) {
        EXPECT_EQ(net::OK, result);
        ++*count;
        w->index->ExecuteWhenReady(base::BindOnce(
            [](int* c, int r) {
              EXPECT_EQ(net::OK, r);
              ++*c;
            },
            count));
      },
      &a, &ready));
  a.file->Complete();
  EXPECT_EQ(0, ready);
  tasks.RunUntilIdle();
  EXPECT_EQ(2, ready);
  a.index->Insert(99);
  EXPECT_TRUE(a.index->HasPendingWrite());
  tasks.FastForwardBy(base::Seconds(19));
  EXPECT_EQ(0u, a.file->writes.size());
  a.index->UseIfExists(99);
  tasks.FastForwardBy(base::Seconds(1));
  EXPECT_EQ(0u, a.file->writes.size());
  tasks.FastForwardBy(base::Seconds(19));
  EXPECT_EQ(1u, a.file->writes.size());
  a.defer_doom = true;
  a.index->SetMaxSize(1);
  auto pending = std::move(a.doom_completion);
  a.index.reset();
  std::move(pending).Run(net::OK);  // weak cancellation after owner destruction
  tasks.FastForwardBy(base::Seconds(30));
  Actual abort(net::DISK_CACHE);
  int aborted = 0;
  for (unsigned i = 0; i < 3; ++i) {
    abort.index->ExecuteWhenReady(base::BindOnce(
        [](int* c, int r) {
          EXPECT_EQ(net::ERR_ABORTED, r);
          ++*c;
        },
        &aborted));
  }
  abort.index.reset();
  EXPECT_EQ(3, aborted);
  Actual deletion(net::DISK_CACHE);
  Populate(deletion, 2, false);
  deletion.file->Complete();
  deletion.on_doom =
      base::BindOnce([](Actual* w) { w->index.reset(); }, &deletion);
  deletion.index->SetMaxSize(1);
  EXPECT_FALSE(deletion.index);
  Actual write_delete(net::DISK_CACHE);
  Populate(write_delete, 1, true);
  write_delete.file->on_write =
      base::BindOnce([](Actual* w) { w->index.reset(); }, &write_delete);
  write_delete.file->Complete();
  EXPECT_FALSE(write_delete.index);
}
TEST(DiskSimpleIndexRustDifferential, SizeTimeEvictionAndFailureEdges) {
  base::test::TaskEnvironment tasks(
      base::test::TaskEnvironment::TimeSource::MOCK_TIME);
  constexpr std::array<uint64_t, 8> sizes = {0,
                                             1,
                                             255,
                                             256,
                                             (uint64_t{1} << 38) - 256,
                                             (uint64_t{1} << 38) - 255,
                                             UINT64_MAX,
                                             UINT64_MAX - 255};
  for (uint64_t size : sizes) {
    EntryMetadata a(base::Time(), uint64_t{256});
    OriginalEntryMetadata b(base::Time(), uint64_t{256});
    ASSERT_EQ(a.SetEntrySize(size), b.SetEntrySize(size));
    ASSERT_EQ(a.GetEntrySize(), b.GetEntrySize());
    a.SetTrailerPrefetchSize(123);
    b.SetTrailerPrefetchSize(123);
    a.SetTrailerPrefetchSize(0);
    b.SetTrailerPrefetchSize(0);
    ASSERT_EQ(a.GetTrailerPrefetchSize(), b.GetTrailerPrefetchSize());
  }
  for (auto type : {net::DISK_CACHE, net::GENERATED_BYTE_CODE_CACHE,
                    net::GENERATED_WEBUI_BYTE_CODE_CACHE}) {
    Actual a(type);
    Original b(type);
    Populate(a, 32, true);
    Populate(b, 32, true);
    a.index->Remove(1);
    b.index->Remove(1);
    a.index->InsertEntryForTesting(
        100, EntryMetadata(base::Time::Min(), uint64_t{511}));
    b.index->InsertEntryForTesting(
        100, OriginalEntryMetadata(base::Time::Min(), uint64_t{511}));
    a.file->Complete();
    b.file->Complete();
    ASSERT_TRUE(a.index->UpdateEntrySize(100, uint64_t{1} << 38));
    ASSERT_TRUE(b.index->UpdateEntrySize(100, uint64_t{1} << 38));
    Compare(a, b, type);
    for (auto time : {base::Time(), base::Time::Min(), base::Time::Max()}) {
      a.index->SetLastUsedTimeForTest(100, time);
      b.index->SetLastUsedTimeForTest(100, time);
      ASSERT_EQ(
          a.index->GetCacheSizeBetween(base::Time::Min(), base::Time::Max()),
          b.index->GetCacheSizeBetween(base::Time::Min(), base::Time::Max()));
    }
    // Distinct ages avoid native-map-address ties between separate instances.
    a.index->SetMaxSize(10000);
    b.index->SetMaxSize(10000);
    ASSERT_EQ(a.doomed, b.doomed);
    Compare(a, b, type);
    a.index->Insert(1000);
    b.index->Insert(1000);
    a.index->UpdateEntrySize(1000, uint64_t{100000});
    b.index->UpdateEntrySize(1000, uint64_t{100000});
    ASSERT_EQ(a.doomed, b.doomed);
  }
  Actual app(net::APP_CACHE);
  Original ref(net::APP_CACHE);
  EXPECT_EQ(app.index->GetTrailerPrefetchSize(999),
            ref.index->GetTrailerPrefetchSize(999));
  int callbacks = 0;
  app.index->ExecuteWhenReady(base::BindOnce(
      [](SimpleIndex* index, int* count, int status) {
        EXPECT_EQ(net::ERR_ABORTED, status);
        ++*count;
        index->ExecuteWhenReady(base::BindOnce(
            [](int* c, int result) {
              EXPECT_EQ(net::ERR_ABORTED, result);
              ++*c;
            },
            count));
      },
      base::Unretained(app.index.get()), &callbacks));
  app.index.reset();
  EXPECT_EQ(2, callbacks);
}
}  // namespace
}  // namespace disk_cache
