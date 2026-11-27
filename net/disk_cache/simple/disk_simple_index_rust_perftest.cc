// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <chrono>
#include <string_view>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/compiler_specific.h"
#include "base/test/test_timeouts.h"
#include "net/disk_cache/simple/disk_simple_index_rust_test_support.h"
namespace disk_cache {
namespace {
volatile uint64_t sink;
template <class W>
uint64_t Work(unsigned kind,
              unsigned iterations,
              base::test::TaskEnvironment& tasks) {
  using M = typename W::Metadata;
  using I = typename W::Index;
  uint64_t sum = 0;
  for (unsigned iteration = 0; iteration < iterations; ++iteration) {
    if (kind < 3) {
      for (unsigned i = 0; i < 64; ++i) {
        M m(base::Time::UnixEpoch() + base::Seconds(i + 1), uint64_t{256} * i);
        m.SetInMemoryData(i & 3);
        if (kind == 0) {
          base::Pickle pickle;
          m.Serialize(net::DISK_CACHE, &pickle);
          base::PickleIterator reader(pickle);
          sum += m.Deserialize(net::DISK_CACHE, &reader, false);
        } else if (kind == 1) {
          base::Pickle pickle;
          pickle.WriteInt64(-1);
          if (i & 1) {
            pickle.WriteUInt64(uint64_t{1} << 38);
          }
          base::PickleIterator reader(pickle);
          sum += m.Deserialize(net::APP_CACHE, &reader, true);
        } else {
          constexpr std::array<uint64_t, 4> sizes = {0, 1, (uint64_t{1} << 38),
                                                     UINT64_MAX};
          sum += m.SetEntrySize(sizes[i & 3]);
          m.SetTrailerPrefetchSize(i & 1 ? 0 : UINT32_MAX);
          m.SetLastUsedTime(i & 1 ? base::Time() : base::Time::Min());
        }
        sum += m.GetEntrySize() + m.GetInMemoryData() + m.RawTimeForSorting();
      }
      continue;
    }
    auto type = kind == 5   ? net::APP_CACHE
                : kind == 9 ? net::GENERATED_BYTE_CODE_CACHE
                            : net::DISK_CACHE;
    W w(type);
    Populate(w, 64, kind == 6);
    if (kind == 6) {
      for (unsigned i = 0; i < 64; ++i) {
        if (i % 3 == 0) {
          w.index->Remove(i);
        } else {
          w.index->Insert(i);
          w.index->UpdateEntrySize(i, uint64_t{1024});
        }
      }
    }
    if (kind == 12) {
      w.index->ExecuteWhenReady(base::BindOnce(
          [](uint64_t* result, int status) {
            *result += status == net::ERR_ABORTED;
          },
          &sum));
      auto late = std::move(w.file->completion);
      w.index.reset();
      std::move(late).Run();
      continue;
    }
    if (kind == 11) {
      for (unsigned i = 0; i < 16; ++i) {
        w.index->ExecuteWhenReady(base::BindOnce(
            [](uint64_t* result, int status) { *result += status == net::OK; },
            &sum));
      }
    }
    w.file->Complete();
    for (unsigned i = 0; i < 64; ++i) {
      if (kind == 3) {
        sum +=
            w.index->Has(i) + w.index->UseIfExists(i) + w.index->Has(1000 + i);
        sum += w.index->GetEntryInMemoryData(i);
      } else if (kind == 4) {
        sum += w.index->UpdateEntrySize(i, uint64_t{i + 1} * 1024);
        w.index->SetEntryInMemoryData(i, (i + 1) & 3);
        w.index->Remove(i);
        w.index->Insert(i);
      } else if (kind == 5) {
        w.index->SetTrailerPrefetchSize(
            i, i & 1 ? 0 : static_cast<int32_t>(i * 31));
        sum += static_cast<uint32_t>(w.index->GetTrailerPrefetchSize(i));
        sum += w.index->UseIfExists(i);
      } else if (kind == 7) {
        auto lower = base::Time::UnixEpoch() + base::Seconds(i);
        auto upper = lower + base::Seconds(5);
        sum += w.index->GetEntriesBetween(lower, upper)->size();
        sum += w.index->GetCacheSizeBetween(lower, upper);
        sum += w.index->GetLastUsedTime(i).ToInternalValue();
      }
    }
    if (kind == 8 || kind == 9) {
      w.index->SetMaxSize(10000);
      sum += w.doomed.size();
    } else if (kind == 10) {
      w.index->Insert(100);
      sum += w.index->HasPendingWrite();
      w.index->WriteToDisk(I::INDEX_WRITE_REASON_IDLE);
      w.index->WriteToDisk(I::INDEX_WRITE_REASON_SHUTDOWN);
      sum += w.file->writes.size();
    } else if (kind == 11) {
      w.index->ExecuteWhenReady(base::BindOnce(
          [](uint64_t* result, int status) { *result += status == net::OK; },
          &sum));
      tasks.RunUntilIdle();
    }
    sum += w.index->GetAllHashes()->size() + w.index->GetEntryCount() +
           w.index->GetCacheSize() +
           static_cast<unsigned>(w.index->init_method());
  }
  return sum;
}
template <class W>
double Measure(unsigned kind, base::test::TaskEnvironment& tasks) {
  const auto start = std::chrono::steady_clock::now();
  const uint64_t result = Work<W>(kind, 200, tasks);
  const auto stop = std::chrono::steady_clock::now();
  sink = result;
  return std::chrono::duration<double, std::nano>(stop - start).count() / 200;
}
}  // namespace
int Run(int argc, char** argv) {
  // SAFETY: main forwards the C++ startup argument array unchanged. argc == 2
  // establishes that argv[1] is a valid NUL-terminated argument.
  const auto option = argc == 2 ? UNSAFE_BUFFERS(std::string_view(argv[1]))
                                : std::string_view();
  const bool timing = option == "--benchmark";
  CHECK(timing || option == "--verify-only");
  base::AtExitManager exit;
  base::CommandLine::Init(argc, argv);
  TestTimeouts::Initialize();
  base::test::TaskEnvironment tasks(
      base::test::TaskEnvironment::TimeSource::MOCK_TIME);
  constexpr std::array<const char*, 13> names = {
      "metadata_roundtrip",  "metadata_corrupt",  "metadata_mutation",
      "index_lookup",        "index_update",      "index_appcache",
      "index_merge",         "index_range",       "index_eviction_disk",
      "index_eviction_code", "index_persistence", "index_ready",
      "index_cancel"};
  uint64_t random = 0x5a961113;
  for (unsigned kind = 0; kind < names.size(); ++kind) {
    CHECK_EQ(Work<Original>(kind, 8, tasks), Work<Actual>(kind, 8, tasks));
    if (!timing) {
      continue;
    }  // No measurement clock calls on verification path.
    for (unsigned repeat = 0; repeat < 20; ++repeat) {
      const bool rust_first = (Random(random) & 1) != 0;
      double cpp, rust;
      if (rust_first) {
        rust = Measure<Actual>(kind, tasks);
        cpp = Measure<Original>(kind, tasks);
      } else {
        cpp = Measure<Original>(kind, tasks);
        rust = Measure<Actual>(kind, tasks);
      }
      // SAFETY: names and the boolean arguments are static NUL-terminated
      // literals; numeric arguments match the fixed format.
      UNSAFE_BUFFERS(std::printf(
          "{\"case\":\"%s\",\"repeat\":%u,\"baseline_ns\":%.9f,\"rust_ns\":%."
          "9f,\"iterations\":200,\"rust_first\":%s}\n",
          names[kind], repeat, cpp, rust, rust_first ? "true" : "false"));
    }
  }
  if (!timing) {
    std::puts("Verified all 13 paired workloads; no measurement clocks read");
  }
  return 0;
}
}  // namespace disk_cache
int main(int argc, char** argv) {
  return disk_cache::Run(argc, argv);
}
