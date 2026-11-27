// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_SOCKET_TCP_CLIENT_SOCKET_RUST_TEST_SUPPORT_H_
#define NET_SOCKET_TCP_CLIENT_SOCKET_RUST_TEST_SUPPORT_H_
#include <tuple>
#include <vector>

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/log/net_log_source.h"
#include "net/socket/tcp_client_socket.h"
#include "net/socket/tcp_client_socket_rust_reference.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
namespace net {
// Test peer exposes completions only; both production and frozen original
// policy execute. No fake state transitions or simulated C++ policy model.
class TCPClientSocketRustTestPeer {
 public:
  template <class T>
  static void Complete(T* s, int rv) {
    s->DidCompleteConnect(rv);
  }
  template <class T>
  static auto State(const T& s) {
    return std::tuple(
        s.current_address_index_, static_cast<int>(s.next_connect_state_),
        s.previously_disconnected_, s.total_received_bytes_, s.was_ever_used_,
        s.was_disconnected_on_suspend_, !!s.bind_address_,
        !!s.connect_callback_, !!s.read_callback_, !!s.write_callback_,
        !!s.start_connect_attempt_, s.connect_attempt_timer_.IsRunning());
  }
};
#if BUILDFLAG(IS_WIN)
// Each side owns its real native descriptor, options, logging and error policy.
// Only IO is deterministic; Close invokes OnClosed and cancels fake callbacks
// exactly as the native primitive's contract requires.
template <class Native>
class TCPClientSocketRustFakeIOBase : public Native {
 public:
  TCPClientSocketRustFakeIOBase();
  ~TCPClientSocketRustFakeIOBase() override;
  int result = OK;
  CompletionOnceCallback read, write;
  int Read(IOBuffer*, int, CompletionOnceCallback cb) override;
  int ReadIfReady(IOBuffer* b, int n, CompletionOnceCallback cb) override;
  int CancelReadIfReady() override;
  int Write(IOBuffer*,
            int,
            CompletionOnceCallback cb,
            const NetworkTrafficAnnotationTag&) override;
  void FinishRead(int rv) { std::move(read).Run(rv); }
  void FinishWrite(int rv) { std::move(write).Run(rv); }

 protected:
  scoped_refptr<typename Native::Core> CreateCore() override;
  bool HasPendingRead() const override;
  void OnClosed() override;
};
template <class Native>
TCPClientSocketRustFakeIOBase<Native>::TCPClientSocketRustFakeIOBase()
    : Native(nullptr, nullptr, NetLogSource()) {}
template <class Native>
TCPClientSocketRustFakeIOBase<Native>::~TCPClientSocketRustFakeIOBase() {
  this->Close();
}
template <class Native>
int TCPClientSocketRustFakeIOBase<Native>::Read(IOBuffer*,
                                                int,
                                                CompletionOnceCallback cb) {
  if (result == ERR_IO_PENDING) {
    read = std::move(cb);
  }
  return result;
}
template <class Native>
int TCPClientSocketRustFakeIOBase<Native>::ReadIfReady(
    IOBuffer* b,
    int n,
    CompletionOnceCallback cb) {
  return Read(b, n, std::move(cb));
}
template <class Native>
int TCPClientSocketRustFakeIOBase<Native>::CancelReadIfReady() {
  read.Reset();
  return OK;
}
template <class Native>
int TCPClientSocketRustFakeIOBase<Native>::Write(
    IOBuffer*,
    int,
    CompletionOnceCallback cb,
    const NetworkTrafficAnnotationTag&) {
  if (result == ERR_IO_PENDING) {
    write = std::move(cb);
  }
  return result;
}
template <class Native>
scoped_refptr<typename Native::Core>
TCPClientSocketRustFakeIOBase<Native>::CreateCore() {
  return nullptr;
}
template <class Native>
bool TCPClientSocketRustFakeIOBase<Native>::HasPendingRead() const {
  return !!read;
}
template <class Native>
void TCPClientSocketRustFakeIOBase<Native>::OnClosed() {
  read.Reset();
  write.Reset();
}
// Keep constructors/destructors and virtual IO bodies out of the workload TU,
// as in the original separately compiled fixture.
extern template class TCPClientSocketRustFakeIOBase<TCPSocketWin>;
extern template class TCPClientSocketRustFakeIOBase<
    TCPClientSocketRustNativeReferenceSocket>;
using TCPClientSocketRustFakeIO = TCPClientSocketRustFakeIOBase<TCPSocketWin>;
using TCPClientSocketRustReferenceFakeIO =
    TCPClientSocketRustFakeIOBase<TCPClientSocketRustNativeReferenceSocket>;
template <class Base>
struct TCPClientSocketRustNativeTraits {
  using Socket = TCPSocket;
  using FakeIO = TCPClientSocketRustFakeIO;
};
template <>
struct TCPClientSocketRustNativeTraits<TCPClientSocketRustReference> {
  using Socket = TCPClientSocketRustNativeReferenceSocket;
  using FakeIO = TCPClientSocketRustReferenceFakeIO;
};
template <class Base>
class TCPClientSocketRustScripted : public Base {
 public:
  TCPClientSocketRustScripted(
      std::unique_ptr<typename TCPClientSocketRustNativeTraits<Base>::Socket>
          io,
      const AddressList& addresses)
      : Base(std::move(io), addresses, nullptr, nullptr) {}
  std::vector<int> results;
  std::vector<uint16_t> attempted_ports;

 private:
  int ConnectInternal(const IPEndPoint& ep) override {
    attempted_ports.push_back(ep.port());
    CHECK_LE(attempted_ports.size(), results.size());
    return results[attempted_ports.size() - 1];
  }
};
#endif
}  // namespace net
#endif
