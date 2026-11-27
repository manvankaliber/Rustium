// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <array>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <new>

#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "build/build_config.h"
#include "net/socket/tcp_client_socket_rust_test_support.h"
#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(IS_WIN)
#include <windows.h>
#endif
namespace net {
namespace {
using Original = TCPClientSocketRustReference;
using Peer = TCPClientSocketRustTestPeer;
class TCPClientSocketRustTest : public testing::Test {
 protected:
  base::test::TaskEnvironment tasks{
      base::test::TaskEnvironment::MainThreadType::IO};
  IPEndPoint ep{IPAddress::IPv4Localhost(), 18080};
};
TEST_F(TCPClientSocketRustTest, StableNativeLayout) {
  EXPECT_EQ(sizeof(TCPClientSocket), sizeof(Original));
  EXPECT_EQ(alignof(TCPClientSocket), alignof(Original));
}
#if BUILDFLAG(IS_WIN)
TEST_F(TCPClientSocketRustTest, LocalAddressDoesNotWriteConstSocketStorage) {
  EXPECT_EXIT(
      {
        ::SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
        for (int mode = 0; mode < 3; ++mode) {
          auto io = std::make_unique<TCPClientSocketRustFakeIO>();
          std::unique_ptr<IPEndPoint> bound;
          IPEndPoint expected;
          if (mode != 0) {
            if (io->Open(ADDRESS_FAMILY_IPV4) != OK ||
                io->Bind(IPEndPoint(IPAddress::IPv4Localhost(), 0)) != OK ||
                io->GetLocalAddress(&expected) != OK) {
              std::_Exit(2);
            }
            bound = std::make_unique<IPEndPoint>(expected);
            if (mode == 2) {
              io->Close();
            }
          }
          void* memory =
              ::VirtualAlloc(nullptr, sizeof(TCPClientSocket),
                             MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
          if (!memory) {
            std::_Exit(3);
          }
          const auto* socket = new (memory) const TCPClientSocket(
              std::move(io),
              AddressList(IPEndPoint(IPAddress::IPv4Localhost(), 18080)),
              std::move(bound), nullptr);
          DWORD previous;
          if (!::VirtualProtect(memory, sizeof(TCPClientSocket), PAGE_READONLY,
                                &previous)) {
            std::_Exit(4);
          }
          IPEndPoint actual;
          const int result = socket->GetLocalAddress(&actual);
          if (result != (mode == 0 ? ERR_SOCKET_NOT_CONNECTED : OK) ||
              (mode != 0 && actual != expected)) {
            std::_Exit(5);
          }
          if (!::VirtualProtect(memory, sizeof(TCPClientSocket), PAGE_READWRITE,
                                &previous)) {
            std::_Exit(6);
          }
          socket->~TCPClientSocket();
          if (!::VirtualFree(memory, 0, MEM_RELEASE)) {
            std::_Exit(7);
          }
        }
        std::_Exit(0);
      },
      testing::ExitedWithCode(0), "");
}

template <class Base>
auto Script(const AddressList& addresses, std::vector<int> results) {
  auto c = std::make_unique<TCPClientSocketRustScripted<Base>>(
      std::make_unique<
          typename TCPClientSocketRustNativeTraits<Base>::FakeIO>(),
      addresses);
  c->results = std::move(results);
  return c;
}
TEST_F(TCPClientSocketRustTest, FallbackAndCompletionDifferential) {
  for (int mode = 0; mode < 8; ++mode) {
    AddressList addresses(std::vector<IPEndPoint>{ep, ep, ep});
    std::vector<int> script{mode & 1 ? ERR_IO_PENDING : ERR_CONNECTION_FAILED,
                            mode & 2 ? ERR_IO_PENDING : ERR_CONNECTION_RESET,
                            mode & 4 ? ERR_CONNECTION_REFUSED : OK};
    auto a = Script<TCPClientSocket>(addresses, script);
    auto b = Script<Original>(addresses, script);
    std::vector<int> ca, cb;
    auto callback = [](std::vector<int>* v, int rv) { v->push_back(rv); };
    EXPECT_EQ(a->Connect(base::BindOnce(callback, &ca)),
              b->Connect(base::BindOnce(callback, &cb)));
    EXPECT_EQ(Peer::State(*a), Peer::State(*b));
    if (mode & 1) {
      Peer::Complete(a.get(), ERR_CONNECTION_FAILED);
      Peer::Complete(b.get(), ERR_CONNECTION_FAILED);
    }
    if (mode & 2) {
      Peer::Complete(a.get(), ERR_CONNECTION_RESET);
      Peer::Complete(b.get(), ERR_CONNECTION_RESET);
    }
    EXPECT_EQ(a->attempted_ports, b->attempted_ports);
    EXPECT_EQ(ca, cb);
    EXPECT_EQ(Peer::State(*a), Peer::State(*b));
    a->Disconnect();
    b->Disconnect();
    EXPECT_EQ(Peer::State(*a), Peer::State(*b));
  }
}
TEST_F(TCPClientSocketRustTest, OneMillionPublicIoDifferentialInputs) {
  auto ia = std::make_unique<TCPClientSocketRustFakeIO>();
  auto* ra = ia.get();
  auto ib = std::make_unique<TCPClientSocketRustReferenceFakeIO>();
  auto* rb = ib.get();
  ASSERT_EQ(OK, ia->Open(ADDRESS_FAMILY_IPV4));
  ASSERT_EQ(OK, ib->Open(ADDRESS_FAMILY_IPV4));
  TCPClientSocket a(std::move(ia), ep);
  Original b(std::move(ib), ep);
  auto buffer = base::MakeRefCounted<IOBufferWithSize>(256);
  uint64_t seed = 0x71c95ab23;
  std::array<uint64_t, 5> methods{};
  for (int i = 0; i < 1000000; ++i) {
    seed ^= seed << 13;
    seed ^= seed >> 7;
    seed ^= seed << 17;
    unsigned method = seed % 5;
    methods[method]++;
    int result = (seed >> 10) % 9 == 0 ? ERR_CONNECTION_RESET
                                       : static_cast<int>((seed >> 16) % 257);
    ra->result = rb->result = result;
    int va = 0, vb = 0, ca = 999, cb = 999;
    auto capture = [](int* output, int value) { *output = value; };
    if (method == 0) {
      va = a.Read(buffer.get(), 256, base::DoNothing());
      vb = b.Read(buffer.get(), 256, base::DoNothing());
    }
    if (method == 1) {
      va = a.ReadIfReady(buffer.get(), 256, base::DoNothing());
      vb = b.ReadIfReady(buffer.get(), 256, base::DoNothing());
    }
    if (method == 2) {
      va = a.Write(buffer.get(), 256, base::DoNothing(),
                   TRAFFIC_ANNOTATION_FOR_TESTS);
      vb = b.Write(buffer.get(), 256, base::DoNothing(),
                   TRAFFIC_ANNOTATION_FOR_TESTS);
    }
    if (method >= 3) {
      ra->result = rb->result = ERR_IO_PENDING;
      if (method == 3) {
        va = a.Read(buffer.get(), 256, base::BindOnce(capture, &ca));
        vb = b.Read(buffer.get(), 256, base::BindOnce(capture, &cb));
        ra->FinishRead(result);
        rb->FinishRead(result);
      } else {
        va = a.Write(buffer.get(), 256, base::BindOnce(capture, &ca),
                     TRAFFIC_ANNOTATION_FOR_TESTS);
        vb = b.Write(buffer.get(), 256, base::BindOnce(capture, &cb),
                     TRAFFIC_ANNOTATION_FOR_TESTS);
        ra->FinishWrite(result);
        rb->FinishWrite(result);
      }
    }
    ASSERT_EQ(va, vb);
    ASSERT_EQ(ca, cb);
    ASSERT_EQ(Peer::State(a), Peer::State(b));
  }
  for (unsigned m = 0; m < 5; ++m) {
    std::printf("differential IO method %u inputs %llu\n", m,
                static_cast<unsigned long long>(methods[m]));
  }
  for (auto n : methods) {
    EXPECT_GT(n, 190000u);
  }
}
TEST_F(TCPClientSocketRustTest, CancelSuspendAndReconnectDifferential) {
  auto ia = std::make_unique<TCPClientSocketRustFakeIO>();
  auto* ra = ia.get();
  auto ib = std::make_unique<TCPClientSocketRustReferenceFakeIO>();
  auto* rb = ib.get();
  ASSERT_EQ(OK, ia->Open(ADDRESS_FAMILY_IPV4));
  ASSERT_EQ(OK, ib->Open(ADDRESS_FAMILY_IPV4));
  TCPClientSocket a(std::move(ia), ep);
  Original b(std::move(ib), ep);
  auto buffer = base::MakeRefCounted<IOBufferWithSize>(64);
  ra->result = rb->result = ERR_IO_PENDING;
  EXPECT_EQ(a.ReadIfReady(buffer.get(), 64, base::DoNothing()),
            b.ReadIfReady(buffer.get(), 64, base::DoNothing()));
  EXPECT_EQ(a.CancelReadIfReady(), b.CancelReadIfReady());
  std::vector<int> ca, cb;
  auto capture = [](std::vector<int>* out, int n) { out->push_back(n); };
  a.Read(buffer.get(), 64, base::BindOnce(capture, &ca));
  b.Read(buffer.get(), 64, base::BindOnce(capture, &cb));
  a.Write(buffer.get(), 64, base::BindOnce(capture, &ca),
          TRAFFIC_ANNOTATION_FOR_TESTS);
  b.Write(buffer.get(), 64, base::BindOnce(capture, &cb),
          TRAFFIC_ANNOTATION_FOR_TESTS);
  a.OnSuspend();
  b.OnSuspend();
  EXPECT_EQ(ca, cb);
  EXPECT_EQ(ca, std::vector<int>(
                    {ERR_NETWORK_IO_SUSPENDED, ERR_NETWORK_IO_SUSPENDED}));
  EXPECT_EQ(Peer::State(a), Peer::State(b));
  EXPECT_EQ(a.Read(buffer.get(), 64, base::DoNothing()),
            b.Read(buffer.get(), 64, base::DoNothing()));
  EXPECT_EQ(a.Write(buffer.get(), 64, base::DoNothing(),
                    TRAFFIC_ANNOTATION_FOR_TESTS),
            b.Write(buffer.get(), 64, base::DoNothing(),
                    TRAFFIC_ANNOTATION_FOR_TESTS));
  a.Disconnect();
  b.Disconnect();
  EXPECT_EQ(Peer::State(a), Peer::State(b));
}
TEST_F(TCPClientSocketRustTest, SuspendReadDestroysOwnerAndSuppressesWrite) {
  auto io = std::make_unique<TCPClientSocketRustFakeIO>();
  auto* raw = io.get();
  ASSERT_EQ(OK, io->Open(ADDRESS_FAMILY_IPV4));
  auto a = std::make_unique<TCPClientSocket>(std::move(io), ep);
  auto buffer = base::MakeRefCounted<IOBufferWithSize>(64);
  raw->result = ERR_IO_PENDING;
  int reads = 0, writes = 0;
  a->Read(buffer.get(), 64,
          base::BindOnce(
              [](std::unique_ptr<TCPClientSocket>* owner, int* n, int rv) {
                EXPECT_EQ(rv, ERR_NETWORK_IO_SUSPENDED);
                ++*n;
                owner->reset();
              },
              &a, &reads));
  a->Write(buffer.get(), 64, base::BindOnce([](int* n, int) { ++*n; }, &writes),
           TRAFFIC_ANNOTATION_FOR_TESTS);
  a->OnSuspend();
  EXPECT_FALSE(a);
  EXPECT_EQ(reads, 1);
  EXPECT_EQ(writes, 0);
}
TEST_F(TCPClientSocketRustTest, AsyncReadWriteConnectCanDestroyOwner) {
  for (int method = 0; method < 3; ++method) {
    auto io = std::make_unique<TCPClientSocketRustFakeIO>();
    auto* raw = io.get();
    auto a = std::make_unique<TCPClientSocketRustScripted<TCPClientSocket>>(
        std::move(io), AddressList(ep));
    a->results = {ERR_IO_PENDING};
    int calls = 0;
    auto cb = base::BindOnce(
        [](decltype(a)* owner, int* n, int rv) {
          EXPECT_EQ(rv, OK);
          ++*n;
          owner->reset();
        },
        &a, &calls);
    if (method == 2) {
      EXPECT_EQ(a->Connect(std::move(cb)), ERR_IO_PENDING);
      Peer::Complete(a.get(), OK);
    } else {
      auto buffer = base::MakeRefCounted<IOBufferWithSize>(64);
      raw->result = ERR_IO_PENDING;
      if (method == 0) {
        a->Read(buffer.get(), 64, std::move(cb));
        raw->FinishRead(OK);
      } else {
        a->Write(buffer.get(), 64, std::move(cb), TRAFFIC_ANNOTATION_FOR_TESTS);
        raw->FinishWrite(OK);
      }
    }
    EXPECT_FALSE(a);
    EXPECT_EQ(calls, 1);
  }
}
TEST_F(TCPClientSocketRustTest, BeforeConnectCanDestroyOrDisconnectOwner) {
  for (int mode = 0; mode < 2; ++mode) {
    auto a = Script<TCPClientSocket>(AddressList(ep), {OK});
    a->SetBeforeConnectCallback(base::BindRepeating(
        [](decltype(a)* owner, int mode) -> int {
          if (mode) {
            owner->reset();
          } else {
            (*owner)->Disconnect();
          }
          return OK;
        },
        &a, mode));
    EXPECT_EQ(a->Connect(base::DoNothing()), ERR_ABORTED);
    if (!mode) {
      EXPECT_TRUE(a->attempted_ports.empty());
    } else {
      EXPECT_FALSE(a);
    }
  }
}
template <class Base>
void SuspendReconnectScenario(const IPEndPoint& ep) {
  auto io = std::make_unique<
      typename TCPClientSocketRustNativeTraits<Base>::FakeIO>();
  auto* raw = io.get();
  TCPClientSocketRustScripted<Base> socket(std::move(io), AddressList(ep));
  socket.results = {OK, OK};
  ASSERT_EQ(socket.Connect(base::DoNothing()), OK);
  auto buffer = base::MakeRefCounted<IOBufferWithSize>(64);
  raw->result = ERR_IO_PENDING;
  int old_writes = 0, new_writes = 0, reads = 0;
  socket.Read(buffer.get(), 64, base::BindLambdaForTesting([&](int rv) {
                EXPECT_EQ(rv, ERR_NETWORK_IO_SUSPENDED);
                ++reads;
                socket.Disconnect();
                EXPECT_EQ(socket.Connect(base::DoNothing()), OK);
                EXPECT_EQ(socket.Write(buffer.get(), 64,
                                       base::BindLambdaForTesting([&](int n) {
                                         EXPECT_EQ(n, 7);
                                         ++new_writes;
                                       }),
                                       TRAFFIC_ANNOTATION_FOR_TESTS),
                          ERR_IO_PENDING);
              }));
  socket.Write(buffer.get(), 64,
               base::BindLambdaForTesting([&](int) { ++old_writes; }),
               TRAFFIC_ANNOTATION_FOR_TESTS);
  socket.OnSuspend();
  EXPECT_EQ(reads, 1);
  EXPECT_EQ(old_writes, 0);
  EXPECT_EQ(new_writes, 0);
  raw->FinishWrite(7);
  EXPECT_EQ(new_writes, 1);
  EXPECT_TRUE(socket.WasEverUsed());
}
TEST_F(TCPClientSocketRustTest, SuspendReconnectPreservesNewWrite) {
  SuspendReconnectScenario<TCPClientSocket>(ep);
  SuspendReconnectScenario<Original>(ep);
}
TEST_F(TCPClientSocketRustTest, FailureAndBindFallbackDifferential) {
  for (int mode = 0; mode < 4; ++mode) {
    auto a = Script<TCPClientSocket>(AddressList(ep), {ERR_CONNECTION_FAILED});
    auto b = Script<Original>(AddressList(ep), {ERR_CONNECTION_FAILED});
    IPEndPoint bind(IPAddress::IPv4Localhost(), 0);
    if (mode == 0) {
      a->SetBeforeConnectCallback(
          base::BindRepeating([]() -> int { return ERR_FAILED; }));
      b->SetBeforeConnectCallback(
          base::BindRepeating([]() -> int { return ERR_FAILED; }));
    }
    if (mode == 1) {
      EXPECT_EQ(a->Bind(bind), b->Bind(bind));
    }
    if (mode == 2) {
      IPEndPoint invalid;
      EXPECT_EQ(a->Bind(invalid), b->Bind(invalid));
      continue;
    }
    if (mode == 3) {
      IPEndPoint nonlocal(IPAddress(192, 0, 2, 1), 0);
      EXPECT_EQ(a->Bind(nonlocal), b->Bind(nonlocal));
    }
    EXPECT_EQ(a->Connect(base::DoNothing()), b->Connect(base::DoNothing()));
    EXPECT_EQ(Peer::State(*a), Peer::State(*b));
    IPEndPoint pa, pb;
    EXPECT_EQ(a->GetLocalAddress(&pa), b->GetLocalAddress(&pb));
    EXPECT_EQ(pa, pb);
  }
}
TEST_F(TCPClientSocketRustTest, FrozenOriginalCancelledConnectSuspendBug) {
  EXPECT_DEATH(
      {
        auto socket = Script<Original>(AddressList(ep), {ERR_IO_PENDING});
        socket->Connect(base::DoNothing());
        socket->Disconnect();
        socket->OnSuspend();
      },
      "");
}
TEST_F(TCPClientSocketRustTest, CallbackCaptureReleaseCanDestroyOwner) {
  auto io = std::make_unique<TCPClientSocketRustFakeIO>();
  io->result = ERR_IO_PENDING;
  auto socket = std::make_unique<TCPClientSocket>(
      std::move(io), AddressList(ep), nullptr, nullptr);
  auto buffer = base::MakeRefCounted<IOBufferWithSize>(64);
  base::ScopedClosureRunner release(
      base::BindOnce([](decltype(socket)* owner) { owner->reset(); }, &socket));
  socket->Read(buffer.get(), 64,
               base::BindOnce([](base::ScopedClosureRunner, int) {},
                              std::move(release)));
  socket->Disconnect();
  EXPECT_FALSE(socket);
}
TEST_F(TCPClientSocketRustTest, CancelledConnectCannotCompleteOnSuspend) {
  auto socket = Script<TCPClientSocket>(AddressList(ep), {ERR_IO_PENDING});
  int calls = 0;
  EXPECT_EQ(socket->Connect(base::BindLambdaForTesting([&](int) { ++calls; })),
            ERR_IO_PENDING);
  socket->Disconnect();
  socket->OnSuspend();
  EXPECT_EQ(calls, 0);
}
template <class Base>
void RepeatedConnectScenario(const IPEndPoint& ep) {
  auto socket = Script<Base>(AddressList(ep), {ERR_IO_PENDING});
  int first = 0, second = 0;
  EXPECT_EQ(socket->Connect(base::BindLambdaForTesting([&](int rv) {
    EXPECT_EQ(rv, OK);
    ++first;
  })),
            ERR_IO_PENDING);
  EXPECT_EQ(socket->Connect(base::BindLambdaForTesting([&](int) { ++second; })),
            OK);
  Peer::Complete(socket.get(), OK);
  EXPECT_EQ(socket->Connect(base::BindLambdaForTesting([&](int) { ++second; })),
            OK);
  EXPECT_EQ(first, 1);
  EXPECT_EQ(second, 0);
  EXPECT_EQ(socket->attempted_ports.size(), 1u);
}
TEST_F(TCPClientSocketRustTest, RepeatedConnectKeepsExactlyOneCallback) {
  RepeatedConnectScenario<TCPClientSocket>(ep);
  RepeatedConnectScenario<Original>(ep);
}
TEST_F(TCPClientSocketRustTest, NetworkBindingErrorDifferential) {
  TCPClientSocket a(AddressList(ep), nullptr, nullptr, nullptr, NetLogSource(),
                    123);
  Original b(AddressList(ep), nullptr, nullptr, nullptr, NetLogSource(), 123);
  EXPECT_EQ(a.Connect(base::DoNothing()), b.Connect(base::DoNothing()));
  EXPECT_EQ(Peer::State(a), Peer::State(b));
}
TEST_F(TCPClientSocketRustTest, BoundAdoptionAndPendingDisconnect) {
  auto io = std::make_unique<TCPClientSocketRustFakeIO>();
  auto* raw = io.get();
  ASSERT_EQ(OK, io->Open(ADDRESS_FAMILY_IPV4));
  IPEndPoint bound(IPAddress::IPv4Localhost(), 0);
  ASSERT_EQ(io->Bind(bound), OK);
  auto a = TCPClientSocket::CreateFromBoundSocket(
      std::move(io), AddressList(ep), bound, nullptr);
  EXPECT_EQ(a->GetTotalReceivedBytes(), 0);
  EXPECT_FALSE(a->WasEverUsed());
  auto buffer = base::MakeRefCounted<IOBufferWithSize>(64);
  raw->result = ERR_IO_PENDING;
  int calls = 0;
  a->ReadIfReady(buffer.get(), 64,
                 base::BindLambdaForTesting([&](int) { ++calls; }));
  a->Write(buffer.get(), 64, base::BindLambdaForTesting([&](int) { ++calls; }),
           TRAFFIC_ANNOTATION_FOR_TESTS);
  a->Disconnect();
  EXPECT_FALSE(raw->read);
  EXPECT_FALSE(raw->write);
  EXPECT_EQ(calls, 0);
}
TEST_F(TCPClientSocketRustTest, ForwardingAndBindDifferential) {
  TCPClientSocket a(AddressList(ep), nullptr, nullptr, nullptr, NetLogSource(),
                    handles::kInvalidNetworkHandle);
  Original b(AddressList(ep), nullptr, nullptr, nullptr, NetLogSource(),
             handles::kInvalidNetworkHandle);
  EXPECT_EQ(a.SetKeepAlive(true, 1), b.SetKeepAlive(true, 1));
  EXPECT_EQ(a.SetNoDelay(true), b.SetNoDelay(true));
  EXPECT_EQ(a.SetReceiveBufferSize(1024), b.SetReceiveBufferSize(1024));
  EXPECT_EQ(a.SetSendBufferSize(1024), b.SetSendBufferSize(1024));
  IPEndPoint pa, pb;
  EXPECT_EQ(a.GetLocalAddress(&pa), b.GetLocalAddress(&pb));
  EXPECT_EQ(a.GetPeerAddress(&pa), b.GetPeerAddress(&pb));
  EXPECT_EQ(a.IsConnected(), b.IsConnected());
  EXPECT_EQ(a.IsConnectedAndIdle(), b.IsConnectedAndIdle());
  EXPECT_EQ(a.GetSSLInfo(nullptr), b.GetSSLInfo(nullptr));
  EXPECT_EQ(a.GetNegotiatedProtocol(), b.GetNegotiatedProtocol());
  IPEndPoint bound(IPAddress::IPv4Localhost(), 0);
  EXPECT_EQ(a.Bind(bound), b.Bind(bound));
  EXPECT_EQ(Peer::State(a), Peer::State(b));
  EXPECT_EQ(a.GetLocalAddress(&pa), b.GetLocalAddress(&pb));
  EXPECT_EQ(pa.address(), pb.address());
  EXPECT_TRUE(a.SetKeepAlive(true, 1));
  EXPECT_TRUE(a.SetNoDelay(true));
  EXPECT_EQ(a.SetReceiveBufferSize(2048), b.SetReceiveBufferSize(2048));
  EXPECT_EQ(a.SetSendBufferSize(2048), b.SetSendBufferSize(2048));
  a.Disconnect();
  b.Disconnect();
  EXPECT_EQ(Peer::State(a), Peer::State(b));
}

class TCPClientSocketLazySlotTest : public testing::Test {
 protected:
  base::test::TaskEnvironment tasks{
      base::test::TaskEnvironment::MainThreadType::IO};
  IPEndPoint endpoint{IPAddress::IPv4Localhost(), 18080};
};

TEST_F(TCPClientSocketLazySlotTest, SuspendWithoutPendingWriteMatchesOriginal) {
  auto original_io = std::make_unique<TCPClientSocketRustReferenceFakeIO>();
  auto production_io = std::make_unique<TCPClientSocketRustFakeIO>();
  ASSERT_EQ(original_io->Open(ADDRESS_FAMILY_IPV4), OK);
  ASSERT_EQ(production_io->Open(ADDRESS_FAMILY_IPV4), OK);
  TCPClientSocketRustReference original(std::move(original_io), endpoint);
  TCPClientSocket production(std::move(production_io), endpoint);
  production.OnSuspend();
  original.OnSuspend();
  EXPECT_EQ(TCPClientSocketRustTestPeer::State(production),
            TCPClientSocketRustTestPeer::State(original));
  production.OnSuspend();
  original.OnSuspend();
  EXPECT_EQ(TCPClientSocketRustTestPeer::State(production),
            TCPClientSocketRustTestPeer::State(original));
}

TEST_F(TCPClientSocketLazySlotTest,
       DetachedWriteCaptureReleasesAfterReadDeletesOwner) {
  auto io = std::make_unique<TCPClientSocketRustFakeIO>();
  auto* native = io.get();
  ASSERT_EQ(io->Open(ADDRESS_FAMILY_IPV4), OK);
  auto owner = std::make_unique<TCPClientSocket>(std::move(io), endpoint);
  auto buffer = base::MakeRefCounted<IOBufferWithSize>(64);
  native->result = ERR_IO_PENDING;
  int reads = 0;
  int writes = 0;
  int released = 0;
  EXPECT_EQ(
      owner->Read(buffer.get(), 64, base::BindLambdaForTesting([&](int result) {
                    EXPECT_EQ(result, ERR_NETWORK_IO_SUSPENDED);
                    EXPECT_EQ(released, 0);
                    ++reads;
                    owner.reset();
                    EXPECT_EQ(released, 0);
                  })),
      ERR_IO_PENDING);
  base::ScopedClosureRunner capture(base::BindLambdaForTesting([&] {
    EXPECT_FALSE(owner);
    EXPECT_EQ(reads, 1);
    ++released;
  }));
  EXPECT_EQ(owner->Write(buffer.get(), 64,
                         base::BindOnce([](base::ScopedClosureRunner,
                                           int* calls, int) { ++*calls; },
                                        std::move(capture), &writes),
                         TRAFFIC_ANNOTATION_FOR_TESTS),
            ERR_IO_PENDING);
  owner->OnSuspend();
  EXPECT_FALSE(owner);
  EXPECT_EQ(reads, 1);
  EXPECT_EQ(writes, 0);
  EXPECT_EQ(released, 1);
}

TEST_F(TCPClientSocketLazySlotTest,
       SameAddressReplacementDoesNotReviveOuterConnect) {
  using Script = TCPClientSocketRustScripted<TCPClientSocket>;
  alignas(Script) std::array<std::byte, sizeof(Script)> storage;
  Script* owner = new (storage.data()) Script(
      std::make_unique<TCPClientSocketRustFakeIO>(), AddressList(endpoint));
  owner->results = {OK};
  const auto initial = TCPClientSocketRustTestPeer::State(*owner);
  bool replaced = false;
  owner->SetBeforeConnectCallback(base::BindLambdaForTesting([&]() -> int {
    owner->~Script();
    owner = new (storage.data()) Script(
        std::make_unique<TCPClientSocketRustFakeIO>(), AddressList(endpoint));
    owner->results = {ERR_IO_PENDING};
    replaced = true;
    return OK;
  }));
  EXPECT_EQ(owner->Connect(base::DoNothing()), ERR_ABORTED);
  EXPECT_TRUE(replaced);
  EXPECT_TRUE(owner->attempted_ports.empty());
  EXPECT_EQ(TCPClientSocketRustTestPeer::State(*owner), initial);
  owner->~Script();
}

#endif
}  // namespace
}  // namespace net
