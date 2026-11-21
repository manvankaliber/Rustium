// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <chrono>
#include <cstdio>
#include <iterator>
#include <random>
#include <ratio>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/task/single_thread_task_executor.h"
#include "base/test/scoped_feature_list.h"
#include "base/time/time_override.h"
#include "net/base/features.h"
#include "net/socket/tcp_client_socket_rust_test_support.h"
namespace net {
namespace {
volatile uint64_t sink = 0;
const char* const kCases[] = {
    "sync-read-64",       "sync-read-ready-64",    "sync-write-64",
    "async-read-64",      "async-write-64",        "cancel-ready",
    "disconnect-pending", "suspend-duplex",        "adopt-connected",
    "construct-normal",   "bind-local-disconnect", "forward-options",
    "fallback-connect",   "async-connect",         "before-error",
    "timeout-connect"};
#if BUILDFLAG(IS_WIN)
template <class Base>
uint64_t One(unsigned kind) {
  IPEndPoint ep(IPAddress::IPv4Localhost(), 18080);
  if (kind == 9) {
    Base socket(AddressList(ep), nullptr, nullptr, nullptr, NetLogSource(),
                handles::kInvalidNetworkHandle);
    return socket.WasEverUsed();
  }
  auto io = std::make_unique<
      typename TCPClientSocketRustNativeTraits<Base>::FakeIO>();
  auto* raw = io.get();
  if (kind >= 12) {
    AddressList addresses(std::vector<IPEndPoint>{ep, ep, ep});
    TCPClientSocketRustScripted<Base> socket(std::move(io), addresses);
    socket.results = kind == 12 ? std::vector<int>{ERR_CONNECTION_FAILED,
                                                   ERR_CONNECTION_RESET, OK}
                                : std::vector<int>{ERR_IO_PENDING};
    if (kind == 14) {
      socket.SetBeforeConnectCallback(
          base::BindRepeating([]() -> int { return ERR_FAILED; }));
    }
    uint64_t sum = 0;
    auto cb = base::BindOnce(
        [](uint64_t* s, int rv) { *s += static_cast<uint64_t>(rv + 1000); },
        &sum);
    int rv = socket.Connect(std::move(cb));
    if (rv == ERR_IO_PENDING) {
      TCPClientSocketRustTestPeer::Complete(&socket, OK);
    }
    return sum + static_cast<uint64_t>(rv + 1000) +
           socket.attempted_ports.size();
  }
  CHECK_EQ(io->Open(ADDRESS_FAMILY_IPV4), OK);
  Base socket(std::move(io), ep);
  if (kind == 8) {
    return socket.WasEverUsed() + socket.GetTotalReceivedBytes();
  }
  if (kind == 10) {
    socket.Disconnect();
    IPEndPoint out;
    int rv = socket.Bind(IPEndPoint(IPAddress::IPv4Localhost(), 0));
    return static_cast<uint64_t>(rv + 1000 + socket.GetLocalAddress(&out));
  }
  if (kind == 11) {
    return static_cast<uint64_t>(
        socket.SetKeepAlive(true, 45) + socket.SetNoDelay(true) +
        socket.SetReceiveBufferSize(4096) + socket.SetSendBufferSize(4096) +
        socket.GetSSLInfo(nullptr));
  }
  auto buffer = base::MakeRefCounted<IOBufferWithSize>(64);
  raw->result = 64;
  uint64_t sum = 0;
  auto callback = [&]() {
    return base::BindOnce(
        [](uint64_t* s, int rv) { *s += static_cast<uint64_t>(rv + 1000); },
        &sum);
  };
  int rv = 0;
  switch (kind) {
    case 0:
      rv = socket.Read(buffer.get(), 64, callback());
      break;
    case 1:
      rv = socket.ReadIfReady(buffer.get(), 64, callback());
      break;
    case 2:
      rv = socket.Write(buffer.get(), 64, callback(),
                        TRAFFIC_ANNOTATION_FOR_TESTS);
      break;
    case 3:
      raw->result = ERR_IO_PENDING;
      rv = socket.Read(buffer.get(), 64, callback());
      raw->FinishRead(64);
      break;
    case 4:
      raw->result = ERR_IO_PENDING;
      rv = socket.Write(buffer.get(), 64, callback(),
                        TRAFFIC_ANNOTATION_FOR_TESTS);
      raw->FinishWrite(64);
      break;
    case 5:
      raw->result = ERR_IO_PENDING;
      rv = socket.ReadIfReady(buffer.get(), 64, callback());
      rv += socket.CancelReadIfReady();
      break;
    case 6:
    case 7:
      raw->result = ERR_IO_PENDING;
      rv = socket.Read(buffer.get(), 64, callback());
      rv += socket.Write(buffer.get(), 64, callback(),
                         TRAFFIC_ANNOTATION_FOR_TESTS);
      if (kind == 6) {
        socket.Disconnect();
      } else {
        socket.OnSuspend();
      }
      break;
  }
  return sum +
         static_cast<uint64_t>(rv + 1000 + socket.GetTotalReceivedBytes()) +
         socket.WasEverUsed();
}
template <class Base>
uint64_t Batch(unsigned kind, unsigned n) {
  uint64_t sum = 0;
  for (unsigned i = 0; i < n; ++i) {
    sum += One<Base>(kind);
  }
  return sum;
}
template <class Base>
double Measure(unsigned kind) {
  const auto begin = std::chrono::steady_clock::now();
  sink = Batch<Base>(kind, 256);
  return std::chrono::duration<double, std::nano>(
             std::chrono::steady_clock::now() - begin)
             .count() /
         256;
}
#endif
}  // namespace
}  // namespace net
int main(int argc, char** argv) {
  base::AtExitManager at_exit;
  base::SingleThreadTaskExecutor executor(base::MessagePumpType::IO);
  base::CommandLine::Init(argc, argv);
  const auto* command = base::CommandLine::ForCurrentProcess();
  const bool timing = command->HasSwitch("timing");
  CHECK(timing || command->HasSwitch("validate-only"));
#if BUILDFLAG(IS_WIN)
  // Verification substitutes constant clock services. Neither measurement
  // clocks nor the production diagnostic clock callbacks read a live clock.
  std::unique_ptr<base::subtle::ScopedTimeClockOverrides> clocks;
  if (!timing) {
    clocks = std::make_unique<base::subtle::ScopedTimeClockOverrides>(
        []() {
          return base::Time::FromDeltaSinceWindowsEpoch(base::Seconds(1));
        },
        []() { return base::TimeTicks() + base::Seconds(1); },
        []() { return base::ThreadTicks() + base::Seconds(1); });
  }
  std::mt19937 random(20261113);
  for (unsigned kind = 0; kind < std::size(net::kCases); ++kind) {
    base::test::ScopedFeatureList feature;
    feature.InitWithFeatureState(net::features::kTimeoutTcpConnectAttempt,
                                 kind == 15);
    CHECK_EQ(net::Batch<net::TCPClientSocket>(kind, 32),
             net::Batch<net::TCPClientSocketRustReference>(kind, 32));
    if (!timing) {
      continue;
    }
    for (unsigned pair = 0; pair < 20; ++pair) {
      const bool rust_first = (random() & 1) != 0;
      double cpp, rust;
      if (rust_first) {
        rust = net::Measure<net::TCPClientSocket>(kind);
        cpp = net::Measure<net::TCPClientSocketRustReference>(kind);
      } else {
        cpp = net::Measure<net::TCPClientSocketRustReference>(kind);
        rust = net::Measure<net::TCPClientSocket>(kind);
      }
      // SAFETY: case names are static NUL-terminated literals.
      UNSAFE_BUFFERS(std::printf(
          "{\"case\":\"%s\",\"repeat\":%u,\"iterations\":256,\"rust_first\":%s,"
          "\"baseline_ns\":%.6f,\"rust_ns\":%.6f}\n",
          net::kCases[kind], pair, rust_first ? "true" : "false", cpp, rust));
    }
  }
  if (!timing) {
    std::puts(
        "PASS: 16 full-public-API paired workloads; verification only; no "
        "measurement clock reads");
  }
#else
  std::puts(
      "Windows fake-IO perftest requires Windows; POSIX tests are parent "
      "acceptance work");
  return 1;
#endif
}
