// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/socket/connect_job_rust_test_support.h"

namespace net::connect_job_rust_test {
Trace::Trace(bool record) : record(record) {}

void Trace::Add(Mark mark, int64_t a, int64_t b,
           std::optional<State> state) {
    const auto mix = [this](uint64_t v) { checksum = checksum * 131 + v; };
    mix(static_cast<unsigned>(mark));
    mix(static_cast<uint64_t>(a));
    mix(static_cast<uint64_t>(b));
    if (state) {
      mix(state->delegate);
      mix(state->timer);
      mix(state->socket);
      mix(static_cast<unsigned>(state->priority));
      mix(static_cast<uint64_t>(state->error.value_or(12345)));
      mix(static_cast<uint64_t>(state->start));
      mix(static_cast<uint64_t>(state->end));
    }
    if (record) {
      events.push_back({mark, a, b, state});
    }
  }

Trace::~Trace() = default;

Trace::Trace(const Trace&) = default;

Trace::Trace(Trace&&) noexcept = default;

Trace& Trace::operator=(const Trace&) = default;

Trace& Trace::operator=(Trace&&) noexcept = default;

Observer::Observer(Trace& trace) : trace_(trace) {
    NetLog::Get()->AddObserver(this, NetLogCaptureMode::kDefault);
  }

Observer::~Observer()  { NetLog::Get()->RemoveObserver(this); }

void Observer::OnAddEntry(const NetLogEntry& entry)  {
    if (entry.type != NetLogEventType::CONNECT_JOB &&
        entry.type != NetLogEventType::TRANSPORT_CONNECT_JOB_CONNECT &&
        entry.type != NetLogEventType::CONNECT_JOB_SET_SOCKET &&
        entry.type != NetLogEventType::CONNECT_JOB_TIMED_OUT) {
      return;
    }
    trace_->Add(Mark::kLog, static_cast<int>(entry.type),
               static_cast<int>(entry.phase), probe ? probe() : std::nullopt);
    // IDs and absolute timestamps differ by construction, so compare dictionary
    // shape/type, not source IDs. Error code presence/value remains observable.
    trace_->Add(Mark::kLogParams, entry.params.FindInt("net_error").value_or(12345),
               entry.params.FindDict("source_dependency") != nullptr);
    if (const auto* dependency = entry.params.FindDict("source_dependency")) {
      trace_->Add(Mark::kLogParams,
                 dependency->FindInt("type").value_or(-1),
                 dependency->FindInt("id").value_or(0) ==
                     static_cast<int>(trace_->expected_source));
    }
  }

Socket::Socket(Trace& trace, int id)
      : trace_(trace), id_(id),
        log_(NetLogWithSource::Make(net::NetLog::Get(), NetLogSourceType::SOCKET)) {}

Socket::~Socket()  { trace_->Add(Mark::kSocketDestroy, id_); }

int Socket::Read(IOBuffer*, int, CompletionOnceCallback)  {
    return ERR_SOCKET_NOT_CONNECTED;
  }

int Socket::Write(IOBuffer*, int, CompletionOnceCallback,
            const NetworkTrafficAnnotationTag&)  {
    return ERR_SOCKET_NOT_CONNECTED;
  }

int Socket::SetReceiveBufferSize(int32_t)  { return OK; }

int Socket::SetSendBufferSize(int32_t)  { return OK; }

int Socket::Connect(CompletionOnceCallback)  { return OK; }

void Socket::Disconnect()  {}

bool Socket::IsConnected() const  { return true; }

bool Socket::IsConnectedAndIdle() const  { return true; }

int Socket::GetPeerAddress(IPEndPoint*) const  { return ERR_SOCKET_NOT_CONNECTED; }

int Socket::GetLocalAddress(IPEndPoint*) const  { return ERR_SOCKET_NOT_CONNECTED; }

const NetLogWithSource& Socket::NetLog() const  {
    trace_->Add(Mark::kSocketLog, id_);
    trace_->expected_source = log_.source().id;
    return log_;
  }

bool Socket::WasEverUsed() const  { return false; }

NextProto Socket::GetNegotiatedProtocol() const  { return NextProto::kProtoUnknown; }

bool Socket::GetSSLInfo(SSLInfo*)  { return false; }

int64_t Socket::GetTotalReceivedBytes() const  { return 0; }

void Socket::ApplySocketTag(const SocketTag&)  {}

void Socket::SetDnsAliases(base::flat_set<std::string> aliases)  {
    trace_->Add(Mark::kAliases, id_, aliases.size());
    dns_aliases_ = std::move(aliases);
    if (on_aliases) {
      on_aliases();
    }
  }
}  // namespace net::connect_job_rust_test
