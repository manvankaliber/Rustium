// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/google_benchmark/src/include/benchmark/benchmark.h"
#include "ui/base/resource/data_pack_parser_test_support.h"

namespace ui::data_pack_internal {
namespace {

// Both paths include the result object. The Rust path includes the actual cxx
// call used by DataPack. File mapping, decompression, and payload use are
// outside this CPU benchmark and require separate browser-level measurements.
void BM_DataPackParseLegacy(benchmark::State& state) {
  const auto resources = static_cast<uint16_t>(state.range(0));
  const auto data = test::MakePack(5, resources, resources / 10);
  for (auto _ : state) {
    benchmark::ClobberMemory();
    benchmark::DoNotOptimize(data.data());
    benchmark::DoNotOptimize(test::ParseLegacy(data));
  }
}

void BM_DataPackParseRust(benchmark::State& state) {
  const auto resources = static_cast<uint16_t>(state.range(0));
  const auto data = test::MakePack(5, resources, resources / 10);
  for (auto _ : state) {
    benchmark::ClobberMemory();
    benchmark::DoNotOptimize(data.data());
    benchmark::DoNotOptimize(parse_data_pack({data.data(), data.size()}));
  }
}

BENCHMARK(BM_DataPackParseLegacy)
    ->Arg(0)
    ->Arg(16)
    ->Arg(1024)
    ->Arg(16000)
    ->Arg(60000);
BENCHMARK(BM_DataPackParseRust)
    ->Arg(0)
    ->Arg(16)
    ->Arg(1024)
    ->Arg(16000)
    ->Arg(60000);

// The C++ reference includes guards needed by the differential fuzzer. This is
// a diagnostic microbenchmark; compare the actual DataPack public methods in
// native before/after builds for production performance acceptance.
template <bool use_rust>
void BM_DataPackLookup(benchmark::State& state) {
  const auto resources = static_cast<uint16_t>(state.range(0));
  const auto data = test::MakePack(5, resources, resources / 10);
  const auto layout = test::ParseLegacy(data);
  uint32_t random = 0x12345678;
  for (auto _ : state) {
    // Deterministic mixed access: direct hits, aliases, and misses. Input
    // choice and the optimization barrier are identical for the two
    // implementations.
    random = random * 1664525 + 1013904223;
    uint16_t id;
    switch ((random >> 16) & 3) {
      case 0:
      case 1:
        id = static_cast<uint16_t>(1 + random % resources);
        break;
      case 2:
        id = static_cast<uint16_t>(resources + 1 + random % (resources / 10));
        break;
      default:
        id = 65535;
    }
    benchmark::ClobberMemory();
    if constexpr (use_rust) {
      benchmark::DoNotOptimize(lookup_data_pack_resource(
          {data.data(), data.size()}, layout.resource_count, layout.alias_count,
          layout.resource_table_offset, id));
    } else {
      benchmark::DoNotOptimize(test::LookupLegacy(data, layout, id));
    }
  }
}

BENCHMARK(BM_DataPackLookup<false>)->Arg(16)->Arg(1024)->Arg(16000);
BENCHMARK(BM_DataPackLookup<true>)->Arg(16)->Arg(1024)->Arg(16000);

}  // namespace
}  // namespace ui::data_pack_internal
