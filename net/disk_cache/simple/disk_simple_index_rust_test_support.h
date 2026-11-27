// Test-only deterministic IO provider; real production adapters execute.
#ifndef DISK_SIMPLE_INDEX_RUST_TEST_SUPPORT_H_
#define DISK_SIMPLE_INDEX_RUST_TEST_SUPPORT_H_
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

}  // namespace
}  // namespace disk_cache
#endif
