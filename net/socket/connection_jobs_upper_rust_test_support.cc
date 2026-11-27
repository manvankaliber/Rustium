// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/socket/connection_jobs_upper_rust_test_support.h"

namespace net::connection_jobs_upper_test {
FakeSocket::FakeSocket(Trace&trace,int id):trace(trace),id(id),log(NetLogWithSource::Make(net::NetLog::Get(),NetLogSourceType::SOCKET)) {}

FakeSocket::~FakeSocket() {trace->Add(Mark::kSocketDestroy,id);}

int FakeSocket::Read(IOBuffer*,int,CompletionOnceCallback) {return ERR_SOCKET_NOT_CONNECTED;}

int FakeSocket::Write(IOBuffer*,int,CompletionOnceCallback,const NetworkTrafficAnnotationTag&) {return ERR_SOCKET_NOT_CONNECTED;}

int FakeSocket::SetReceiveBufferSize(int32_t) {return OK;}

int FakeSocket::SetSendBufferSize(int32_t) {return OK;}

int FakeSocket::Connect(CompletionOnceCallback) {return OK;}

void FakeSocket::Disconnect() {trace->Add(Mark::kReturn,107,id);}

bool FakeSocket::IsConnected()const  {return true;}

bool FakeSocket::IsConnectedAndIdle()const  {return true;}

int FakeSocket::GetPeerAddress(IPEndPoint*)const  {return ERR_SOCKET_NOT_CONNECTED;}

int FakeSocket::GetLocalAddress(IPEndPoint*)const  {return ERR_SOCKET_NOT_CONNECTED;}

const NetLogWithSource& FakeSocket::NetLog()const  {trace->Add(Mark::kSocketLog,id);return log;}

bool FakeSocket::WasEverUsed()const  {return false;}

NextProto FakeSocket::GetNegotiatedProtocol()const  {return NextProto::kProtoUnknown;}

bool FakeSocket::GetSSLInfo(SSLInfo*) {return false;}

int64_t FakeSocket::GetTotalReceivedBytes()const  {return 0;}

void FakeSocket::ApplySocketTag(const SocketTag&) {}
}  // namespace net::connection_jobs_upper_test
