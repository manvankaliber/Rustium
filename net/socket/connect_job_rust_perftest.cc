// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/socket/connect_job_rust_test_support.h"

#include <chrono>
#include <cstdio>
#include <random>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/logging/logging_settings.h"
#include "base/test/test_timeouts.h"
#include "base/check_op.h"
#include "base/compiler_specific.h"

namespace net::connect_job_rust_test {
namespace {
volatile uint64_t sink = 0;
constexpr unsigned kIterations = 256;

template <class Base>
uint64_t Batch(Scenario kind, base::test::TaskEnvironment& environment,
               const Inputs<Base>& inputs) {
  uint64_t checksum = 0;
  for (unsigned i = 0; i < kIterations; ++i) {
    checksum = checksum * 131 + Run<Base>(kind, environment, inputs, false).checksum;
  }
  return checksum;
}
template <class Base>
std::pair<double, uint64_t> Measure(
    Scenario kind, base::test::TaskEnvironment& environment,
    const Inputs<Base>& inputs) {
  const auto begin = std::chrono::steady_clock::now();
  const uint64_t checksum = Batch<Base>(kind, environment, inputs);
  const auto end = std::chrono::steady_clock::now();
  sink = checksum;
  return {std::chrono::duration<double, std::nano>(end - begin).count() /
              kIterations,
          checksum};
}
}  // namespace
}  // namespace net::connect_job_rust_test

int main(int argc, char** argv) {
  base::AtExitManager at_exit;
  base::CommandLine::Init(argc, argv);
  if (!logging::InitLogging({.logging_dest = logging::LOG_TO_STDERR})) {
    return 1;
  }
  TestTimeouts::Initialize();
  const auto* command = base::CommandLine::ForCurrentProcess();
  const bool timing = command->HasSwitch("timing");
  const bool verify = command->HasSwitch("verify-only");
  CHECK(timing != verify) << "Choose exactly one of --verify-only or --timing";
  base::test::TaskEnvironment environment{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  using namespace net;
  using namespace net::connect_job_rust_test;
  // Separate preconstructed fixtures. ResponseInfo's independent rewrite and
  // CommonParams construction are excluded from both measured ConnectJob paths.
  const Inputs<ConnectJobRustOriginal> original_inputs;
  const Inputs<ConnectJob> rust_inputs;
  for (size_t i = 0; i < kWorkloadNames.size(); ++i) {
    const auto kind = static_cast<Scenario>(i);
    const auto original = Run<ConnectJobRustOriginal>(kind, environment, original_inputs);
    const auto rust = Run<ConnectJob>(kind, environment, rust_inputs);
    CHECK(original.events == rust.events);
    CHECK_EQ(original.checksum, rust.checksum);
    CHECK_EQ(Batch<ConnectJobRustOriginal>(kind, environment, original_inputs),
             Batch<ConnectJob>(kind, environment, rust_inputs));
  }
  // Verify-only never enters Measure or reads the measurement clock. Mock-time
  // lifecycle operations still read their normal virtual task clock.
  if (verify) {
    std::puts("Verified15 complete ConnectJob BASE lifecycle workloads; no timing");
    return 0;
  }
  std::mt19937 random(20261113);
  for (size_t i = 0; i < kWorkloadNames.size(); ++i) {
    const auto kind = static_cast<Scenario>(i);
    for (unsigned repeat = 0; repeat < 20; ++repeat) {
      const bool rust_first = (random() & 1) != 0;
      std::pair<double, uint64_t> cpp, rust;
      if (rust_first) {
        rust = Measure<ConnectJob>(kind, environment, rust_inputs);
        cpp = Measure<ConnectJobRustOriginal>(kind, environment, original_inputs);
      } else {
        cpp = Measure<ConnectJobRustOriginal>(kind, environment, original_inputs);
        rust = Measure<ConnectJob>(kind, environment, rust_inputs);
      }
      CHECK_EQ(cpp.second, rust.second);
      // SAFETY: case names are static NUL-terminated literals. All scalar
      // printf arguments exactly match their specifiers; no caller strings.
      UNSAFE_BUFFERS(std::printf(
          "{\"case\":\"%s\",\"repeat\":%u,\"iterations\":%u,\"rust_first\":%s,"
          "\"baseline_ns\":%.9f,\"rust_ns\":%.9f,"
          "\"baseline_checksum\":%llu,\"rust_checksum\":%llu}\n",
          kWorkloadNames.at(i), repeat, kIterations,
          rust_first ? "true" : "false", cpp.first, rust.first,
          static_cast<unsigned long long>(cpp.second),
          static_cast<unsigned long long>(rust.second)));
    }
  }
  return 0;
}
