// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "base/functional/bind.h"
#include "base/location.h"
#include "base/memory/weak_ptr.h"
#include "base/rand_util.h"
#include "base/task/single_thread_task_runner.h"
#include "base/threading/thread_checker.h"
#include "chrome/browser/devtools/device/remote_device_manager.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/server/web_socket_encoder.h"
#include "net/server/web_socket_parse_result.h"
#include "net/socket/stream_socket.h"
#include "net/traffic_annotation/network_traffic_annotation.h"

using content::BrowserThread;
using net::WebSocket;

namespace {

const int kBufferSize = 16 * 1024;
const char kCloseResponse[] = "\x88\x80\x2D\x0E\x1E\xFA";

net::NetworkTrafficAnnotationTag kRemoteWebSocketTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("remote_web_socket", R"(
        semantics {
          sender: "Remote DevTools WebSocket"
          description: "Carries DevTools protocol messages to a remote debugging endpoint."
          trigger: "The user opens a remote debugging target."
          data: "DevTools protocol requests and responses."
          destination: OTHER
          destination_other: "The remote debugging endpoint selected by the user."
        }
        policy {
          cookies_allowed: NO
          setting: "Remote target discovery is configured by the user."
          policy_exception_justification: "Developer-requested remote debugging."
        })");
}  // namespace

class RemoteDeviceManager::RemoteWebSocket::WebSocketImpl {
 public:
  WebSocketImpl(
      scoped_refptr<base::SingleThreadTaskRunner> response_task_runner,
      base::WeakPtr<RemoteWebSocket> weak_socket,
      const std::string& extensions,
      const std::string& body_head,
      std::unique_ptr<net::StreamSocket> socket)
      : response_task_runner_(response_task_runner),
        weak_socket_(weak_socket),
        socket_(std::move(socket)),
        encoder_(net::WebSocketEncoder::CreateClient(extensions)),
        response_buffer_(body_head) {
    thread_checker_.DetachFromThread();
  }

  WebSocketImpl(const WebSocketImpl&) = delete;
  WebSocketImpl& operator=(const WebSocketImpl&) = delete;

  void StartListening() {
    CHECK(thread_checker_.CalledOnValidThread(), base::NotFatalUntil::M161);
    CHECK(socket_, base::NotFatalUntil::M161);

    auto buffer = base::MakeRefCounted<net::IOBufferWithSize>(kBufferSize);
    if (!response_buffer_.empty())
      ProcessResponseBuffer(buffer);
    else
      Read(buffer);
  }

  void SendFrame(const std::string& message) {
    CHECK(thread_checker_.CalledOnValidThread(), base::NotFatalUntil::M161);
    if (!socket_)
      return;
    int mask = base::RandIntInclusive(0, 0x7FFFFFFF);
    std::string encoded_frame;
    encoder_->EncodeTextFrame(message, mask, &encoded_frame);
    SendData(encoded_frame);
  }

  base::WeakPtr<WebSocketImpl> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

 private:
  void Read(scoped_refptr<net::IOBuffer> io_buffer) {
    int result =
        socket_->Read(io_buffer.get(), kBufferSize,
                      base::BindOnce(&WebSocketImpl::OnBytesRead,
                                     weak_factory_.GetWeakPtr(), io_buffer));
    if (result != net::ERR_IO_PENDING)
      OnBytesRead(io_buffer, result);
  }

  void OnBytesRead(scoped_refptr<net::IOBuffer> io_buffer, int result) {
    CHECK(thread_checker_.CalledOnValidThread(), base::NotFatalUntil::M161);
    if (result <= 0) {
      Disconnect();
      return;
    }
    response_buffer_.append(io_buffer->data(), result);

    ProcessResponseBuffer(io_buffer);
  }

  void ProcessResponseBuffer(scoped_refptr<net::IOBuffer> io_buffer) {
    int bytes_consumed;
    std::string output;
    net::WebSocketParseResult parse_result =
        encoder_->DecodeFrame(response_buffer_, &bytes_consumed, &output);

    while (parse_result == net::WebSocketParseResult::FRAME_OK_FINAL) {
      response_buffer_ = response_buffer_.substr(bytes_consumed);
      response_task_runner_->PostTask(
          FROM_HERE,
          base::BindOnce(&RemoteWebSocket::OnFrameRead, weak_socket_, output));
      parse_result = encoder_->DecodeFrame(
          response_buffer_, &bytes_consumed, &output);
    }
    if (parse_result == net::WebSocketParseResult::FRAME_CLOSE) {
      SendData(kCloseResponse);
    }

    if (parse_result == net::WebSocketParseResult::FRAME_ERROR) {
      Disconnect();
      return;
    }
    Read(io_buffer);
  }

  void SendData(const std::string& data) {
    request_buffer_ += data;
    if (request_buffer_.length() == data.length())
      SendPendingRequests(0);
  }

  void SendPendingRequests(int result) {
    CHECK(thread_checker_.CalledOnValidThread(), base::NotFatalUntil::M161);
    if (result < 0) {
      Disconnect();
      return;
    }
    request_buffer_ = request_buffer_.substr(result);
    if (request_buffer_.empty())
      return;

    scoped_refptr<net::StringIOBuffer> buffer =
        base::MakeRefCounted<net::StringIOBuffer>(request_buffer_);
    result = socket_->Write(buffer.get(), buffer->size(),
                            base::BindOnce(&WebSocketImpl::SendPendingRequests,
                                           weak_factory_.GetWeakPtr()),
                            kRemoteWebSocketTrafficAnnotation);
    if (result != net::ERR_IO_PENDING)
      SendPendingRequests(result);
  }

  void Disconnect() {
    CHECK(thread_checker_.CalledOnValidThread(), base::NotFatalUntil::M161);
    socket_.reset();
    response_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&RemoteWebSocket::OnSocketClosed, weak_socket_));
  }

  scoped_refptr<base::SingleThreadTaskRunner> response_task_runner_;
  base::WeakPtr<RemoteWebSocket> weak_socket_;
  std::unique_ptr<net::StreamSocket> socket_;
  std::unique_ptr<net::WebSocketEncoder> encoder_;
  std::string response_buffer_;
  std::string request_buffer_;
  base::ThreadChecker thread_checker_;

  base::WeakPtrFactory<WebSocketImpl> weak_factory_{this};
};

RemoteDeviceManager::RemoteWebSocket::RemoteWebSocket(
    scoped_refptr<Device> device,
    const std::string& socket_name,
    const std::string& path,
    Delegate* delegate)
    : device_(device),
      socket_impl_(nullptr, base::OnTaskRunnerDeleter(device->task_runner_)),
      delegate_(delegate) {
  CHECK_CURRENTLY_ON(BrowserThread::UI, base::NotFatalUntil::M161);
  CHECK(delegate_, base::NotFatalUntil::M161);
  CHECK(device_, base::NotFatalUntil::M161);
  device_->HttpUpgrade(
      socket_name, path, net::WebSocketEncoder::kClientExtensions,
      base::BindOnce(&RemoteWebSocket::Connected, weak_factory_.GetWeakPtr()));
}

RemoteDeviceManager::RemoteWebSocket::~RemoteWebSocket() = default;

void RemoteDeviceManager::RemoteWebSocket::SendFrame(
    const std::string& message) {
  CHECK_CURRENTLY_ON(BrowserThread::UI, base::NotFatalUntil::M161);
  CHECK(socket_impl_, base::NotFatalUntil::M161);
  CHECK(device_, base::NotFatalUntil::M161);
  device_->task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&WebSocketImpl::SendFrame,
                                socket_impl_->GetWeakPtr(), message));
}

void RemoteDeviceManager::RemoteWebSocket::Connected(
    int result,
    const std::string& extensions,
    const std::string& body_head,
    std::unique_ptr<net::StreamSocket> socket) {
  CHECK_CURRENTLY_ON(BrowserThread::UI, base::NotFatalUntil::M161);
  if (result != net::OK || !socket.get()) {
    OnSocketClosed();
    return;
  }
  socket_impl_.reset(new WebSocketImpl(
      base::SingleThreadTaskRunner::GetCurrentDefault(),
      weak_factory_.GetWeakPtr(), extensions, body_head, std::move(socket)));
  device_->task_runner_->PostTask(FROM_HERE,
                                  base::BindOnce(&WebSocketImpl::StartListening,
                                                 socket_impl_->GetWeakPtr()));
  delegate_->OnSocketOpened();
}

void RemoteDeviceManager::RemoteWebSocket::OnFrameRead(
    const std::string& message) {
  CHECK_CURRENTLY_ON(BrowserThread::UI, base::NotFatalUntil::M161);
  delegate_->OnFrameRead(message);
}

void RemoteDeviceManager::RemoteWebSocket::OnSocketClosed() {
  CHECK_CURRENTLY_ON(BrowserThread::UI, base::NotFatalUntil::M161);
  delegate_->OnSocketClosed();
}

RemoteDeviceManager::RemoteWebSocket*
RemoteDeviceManager::Device::CreateWebSocket(
    const std::string& socket_name,
    const std::string& path,
    RemoteWebSocket::Delegate* delegate) {
  return new RemoteWebSocket(this, socket_name, path, delegate);
}
