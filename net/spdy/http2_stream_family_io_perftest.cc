// Copyright 2026 The Chromium Authors
// BSD-style license: see LICENSE.
#include <array>
#include <chrono>
#include <cstdio>
#include <random>
#include <string_view>

#include "base/compiler_specific.h"
#include "net/spdy/http2_stream_family_io_fixture.h"
#include "net/test/net_test_suite.h"
#include "net/test/test_with_task_environment.h"

namespace {
using namespace net;
using namespace net::http2_family_test;
constexpr std::array<const char*, 5> kCases = {
    "get_buffered", "get_pending_read", "chunked_upload_echo",
    "flow_controlled_upload", "cancel_pending_read"};
volatile uint64_t sink;

template <class Pool>
std::pair<double, IoResult> Measure(
    NativeIoEnvironment& environment,
    const typename Http2SessionFamilyFactory<Pool>::Key& key) {
  const auto start = std::chrono::steady_clock::now();
  IoResult result = RunIoPath<Pool>(environment, key);
  const auto end = std::chrono::steady_clock::now();
  sink = result.Checksum();
  return {std::chrono::duration<double, std::nano>(end - start).count(),
          std::move(result)};
}

void Verify(const IoInputs& inputs) {
  NativeIoEnvironment original_environment(inputs);
  NativeIoEnvironment rust_environment(inputs);
  const auto original_key = MakeIoKey<SpdySessionPoolOriginal>(inputs.url);
  const auto rust_key = MakeIoKey<SpdySessionPool>(inputs.url);
  base::RunLoop().RunUntilIdle();
  CHECK(RunIoPath<SpdySessionPoolOriginal>(original_environment, original_key) ==
        RunIoPath<SpdySessionPool>(rust_environment, rust_key));
}
}  // namespace

int RunBenchmark(int argc, char** argv) {
  const bool verify = argc == 2 &&
      UNSAFE_BUFFERS(std::string_view(argv[1])) == "--verify-only";
  const bool timing = argc == 2 &&
      UNSAFE_BUFFERS(std::string_view(argv[1])) == "--timing";
  if (!verify && !timing) return 2;
  std::array<std::unique_ptr<IoInputs>, 5> inputs;
  for (size_t kind = 0; kind != inputs.size(); ++kind) {
    inputs[kind] = std::make_unique<IoInputs>(static_cast<IoCase>(kind));
    Verify(*inputs[kind]);
    base::RunLoop().RunUntilIdle();
  }
  if (verify) {
    UNSAFE_BUFFERS(std::puts("Verified five complete scripted public HTTP2 paths; no timing"));
    return 0;
  }
  std::mt19937 random(20261113);
  for (size_t kind = 0; kind != inputs.size(); ++kind) {
    for (unsigned repeat = 0; repeat != 20; ++repeat) {
      const bool rust_first = (random() & 1) != 0;
      {
        // Fresh native sockets/caller storage/keys before BOTH clocks. Every
        // measured operation independently constructs and destroys its pool.
        NativeIoEnvironment original_environment(*inputs[kind]);
        NativeIoEnvironment rust_environment(*inputs[kind]);
        const auto original_key = MakeIoKey<SpdySessionPoolOriginal>(inputs[kind]->url);
        const auto rust_key = MakeIoKey<SpdySessionPool>(inputs[kind]->url);
        base::RunLoop().RunUntilIdle();
        std::pair<double, IoResult> baseline, rust;
        if (rust_first) {
          rust = Measure<SpdySessionPool>(rust_environment, rust_key);
          baseline = Measure<SpdySessionPoolOriginal>(original_environment, original_key);
        } else {
          baseline = Measure<SpdySessionPoolOriginal>(original_environment, original_key);
          rust = Measure<SpdySessionPool>(rust_environment, rust_key);
        }
        CHECK(baseline.second == rust.second);
        UNSAFE_BUFFERS(std::printf(
            "{\"case\":\"%s\",\"repeat\":%u,\"baseline_ns\":%.9f,"
            "\"rust_ns\":%.9f,\"iterations\":1,\"rust_first\":%s,"
            "\"baseline_checksum\":%llu,\"rust_checksum\":%llu}\n",
            kCases[kind], repeat, baseline.first, rust.first,
            rust_first ? "true" : "false",
            static_cast<unsigned long long>(baseline.second.Checksum()),
            static_cast<unsigned long long>(rust.second.Checksum())));
      }
      base::RunLoop().RunUntilIdle();
    }
  }
  return 0;
}

int main(int argc, char** argv) {
  NetTestSuite suite(argc, argv);
  suite.Initialize();
  int result;
  {
    net::NetTaskEnvironment tasks(
        base::test::TaskEnvironment::MainThreadType::IO);
    result = RunBenchmark(argc, argv);
  }
  suite.Shutdown();
  return result;
}
