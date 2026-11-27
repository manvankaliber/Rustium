// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_QUIC_HTTP3_CLIENT_STREAM_RUST_TEST_SUPPORT_H_
#define NET_QUIC_HTTP3_CLIENT_STREAM_RUST_TEST_SUPPORT_H_

#include <stdint.h>

#include <algorithm>
#include <array>
#include <string>
#include <vector>

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/run_loop.h"
#include "base/test/task_environment.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/quic/quic_chromium_client_stream.h"
#include "net/third_party/quiche/src/quiche/quic/core/http/quic_spdy_client_session_base.h"
#include "net/third_party/quiche/src/quiche/quic/test_tools/quic_config_peer.h"
#include "net/third_party/quiche/src/quiche/quic/test_tools/quic_spdy_session_peer.h"
#include "net/third_party/quiche/src/quiche/quic/test_tools/quic_test_utils.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace net::http3_rust_test {
using ActionCounts = std::array<uint64_t, 19>;
// Only transport primitives are mocked. Both Stream implementations execute
// their actual production policy, QUICHE framing/sequencing and public
// adapters.
template <class Stream>
class Session : public quic::QuicSpdyClientSessionBase {
 public:
  explicit Session(quic::QuicConnection* connection)
      : quic::QuicSpdyClientSessionBase(connection,
                                        nullptr,
                                        quic::test::DefaultQuicConfig(),
                                        connection->supported_versions()),
        crypto_(this) {
    Initialize();
  }
  const quic::QuicCryptoStream* GetCryptoStream() const override {
    return &crypto_;
  }
  quic::QuicCryptoStream* GetMutableCryptoStream() override { return &crypto_; }
  Stream* CreateIncomingStream(quic::QuicStreamId) override { return nullptr; }
  Stream* CreateIncomingStream(quic::PendingStream*) { return nullptr; }
  Stream* CreateOutgoingBidirectionalStream() override { return nullptr; }
  Stream* CreateOutgoingUnidirectionalStream() { return nullptr; }
  bool ShouldCreateIncomingStream(quic::QuicStreamId) override { return false; }
  bool ShouldCreateOutgoingBidirectionalStream() override { return false; }
  bool ShouldCreateOutgoingUnidirectionalStream() { return false; }
  void OnProofValid(const quic::QuicCryptoClientConfig::CachedState&) override {
  }
  void OnProofVerifyDetailsAvailable(const quic::ProofVerifyDetails&) override {
  }
  bool OneRttKeysAvailable() const { return true; }
  quic::QuicConsumedData WritevData(quic::QuicStreamId,
                                    size_t length,
                                    quic::QuicStreamOffset,
                                    quic::StreamSendingState state,
                                    quic::TransmissionType,
                                    quic::EncryptionLevel) override {
    return {length, state != quic::NO_FIN};
  }
  bool WriteControlFrame(const quic::QuicFrame&,
                         quic::TransmissionType) override {
    return true;
  }
  using quic::QuicSession::ActivateStream;

 private:
  class Crypto : public quic::test::MockQuicCryptoStream {
   public:
    using quic::test::MockQuicCryptoStream::MockQuicCryptoStream;
    bool encryption_established() const override { return true; }
    bool one_rtt_keys_available() const override { return true; }
  };
  Crypto crypto_;
};

inline uint64_t Next(uint64_t& state) {
  state ^= state << 13;
  state ^= state >> 7;
  state ^= state << 17;
  return state;
}

template <class Stream>
class Fixture {
 public:
  class TransportStream : public Stream {
   public:
    using Stream::Stream;
    quic::DatagramStatus SendHttp3Datagram(std::string_view payload) override {
      datagram_payload.assign(payload);
      return datagram_status;
    }
    quic::DatagramStatus datagram_status =
        quic::DatagramStatus::DATAGRAM_STATUS_SUCCESS;
    std::string datagram_payload;
  };
  explicit Fixture(quic::ParsedQuicVersion version)
      : version_(version),
        session_(new testing::NiceMock<quic::test::MockQuicConnection>(
            &helper_,
            &alarms_,
            quic::Perspective::IS_CLIENT,
            quic::test::SupportedVersions(version))) {
    quic::test::QuicConfigPeer::SetReceivedInitialSessionFlowControlWindow(
        session_.config(), 16 * 1024 * 1024);
    quic::test::QuicConfigPeer::
        SetReceivedInitialMaxStreamDataBytesOutgoingBidirectional(
            session_.config(), 1024 * 1024);
    quic::test::QuicConfigPeer::SetReceivedMaxUnidirectionalStreams(
        session_.config(), 10);
    session_.OnConfigNegotiated();
    quic::test::QuicSpdySessionPeer::SetHttpDatagramSupport(
        &session_, quic::HttpDatagramSupport::kRfc);
    helper_.AdvanceTime(quic::QuicTime::Delta::FromSeconds(1));
    session_.connection()->SetEncrypter(
        quic::ENCRYPTION_FORWARD_SECURE,
        std::make_unique<quic::test::TaggingEncrypter>(
            quic::ENCRYPTION_FORWARD_SECURE));
  }

  // Entire lifecycle is included: native/Rust entry, header/body copies,
  // callbacks, ownership, stream/handle destruction and deterministic
  // transport. TaskEnvironment/session construction is amortized outside the
  // workload.
  std::vector<uint64_t> Scenario(uint64_t seed,
                                 uint32_t operations,
                                 ActionCounts* counts = nullptr) {
    std::vector<uint64_t> trace;
    auto* stream = new TransportStream(
        quic::test::GetNthClientInitiatedBidirectionalStreamId(
            version_.transport_version, next_stream_++),
        &session_, quic::QuicServerId("www.google.com", 443),
        quic::BIDIRECTIONAL, NetLogWithSource(), TRAFFIC_ANNOTATION_FOR_TESTS,
        base::Microseconds(seed % 10000));
    session_.ActivateStream(base::WrapUnique(stream));
    auto handle = stream->CreateHandle();
    class Visitor : public quic::QuicSpdyStream::Http3DatagramVisitor {
     public:
      void OnHttp3Datagram(quic::QuicStreamId, std::string_view) override {}
      void OnUnknownCapsule(quic::QuicStreamId,
                            const quiche::UnknownCapsule&) override {}
    } visitor;
    quiche::HttpHeaderBlock request;
    request[":method"] = "GET";
    request[":scheme"] = "https";
    request[":authority"] = "www.google.com";
    request[":path"] = "/";
    trace.push_back(handle->WriteHeaders(std::move(request), false, nullptr));
    handle->SetPriority(quic::QuicStreamPriority(
        quic::HttpStreamPriority{static_cast<int>(seed % 8), (seed & 8) != 0}));
    trace.push_back(handle->GetGuaranteedLargestMessagePayload());
    quiche::HttpHeaderBlock headers;
    const size_t hints = seed % 5;
    for (size_t i = 0; i < hints; ++i) {
      headers[":status"] = "103";
      headers["link"] = std::string(i + 1, 'x');
      auto list = quic::test::AsHeaderList(headers);
      stream->OnStreamHeaderList(false, list.uncompressed_header_bytes(), list);
      quiche::HttpHeaderBlock received;
      trace.push_back(handle->ReadInitialHeaders(&received, base::DoNothing()));
      trace.push_back(received == headers);
    }
    headers.clear();
    headers[":status"] = std::to_string(200 + seed % 400);
    headers["x-seed"] = std::to_string(seed);
    auto list = quic::test::AsHeaderList(headers);
    stream->OnStreamHeaderList(false, list.uncompressed_header_bytes(), list);
    quiche::HttpHeaderBlock received;
    trace.push_back(handle->ReadInitialHeaders(&received, base::DoNothing()));
    trace.push_back(received == headers);
    base::RunLoop().RunUntilIdle();
    const size_t size = 1 + seed % 256;
    std::string body(size, static_cast<char>(seed));
    trace.push_back(handle->WriteConnectUdpPayload(body));
    trace.push_back(stream->datagram_payload == std::string(1, '\0') + body);
    stream->datagram_status = quic::DatagramStatus::DATAGRAM_STATUS_BLOCKED;
    trace.push_back(handle->WriteConnectUdpPayload(body));
    trace.push_back(stream->datagram_payload == std::string(1, '\0') + body);
    auto framing = quic::HttpEncoder::SerializeDataFrameHeader(
        size, quiche::SimpleBufferAllocator::Get());
    stream->OnStreamFrame(quic::QuicStreamFrame(
        stream->id(), false, 0,
        std::string_view(framing.data(), framing.size())));
    stream->OnStreamFrame(
        quic::QuicStreamFrame(stream->id(), false, framing.size(), body));
    quiche::HttpHeaderBlock trailers;
    trailers["x-trailer"] = std::to_string(seed);
    auto trailing_list = quic::test::AsHeaderList(trailers);
    stream->OnStreamHeaderList(true, trailing_list.uncompressed_header_bytes(),
                               trailing_list);
    quiche::HttpHeaderBlock received_trailers;
    trace.push_back(
        handle->ReadTrailingHeaders(&received_trailers, base::DoNothing()));
    trace.push_back(trailers == received_trailers);
    auto buffer = base::MakeRefCounted<IOBufferWithSize>(512);
    std::vector<scoped_refptr<IOBuffer>> buffers{buffer, buffer};
    bool detached = false;
    for (uint32_t step = 0; step < operations; ++step) {
      const auto input = Next(seed);
      const auto method = input % 19;
      if (counts) {
        ++(*counts)[method];
      }
      trace.push_back(method);
      switch (method) {
        case 0:
        case 1: {
          std::fill(buffer->span().begin(), buffer->span().end(),
                    uint8_t{0x5a});
          const int length = 1 + (input >> 8) % 511;
          const int rv = method == 0 ? handle->ReadBody(buffer.get(), length,
                                                        base::DoNothing())
                                     : handle->Read(buffer.get(), length);
          trace.push_back(rv);
          uint64_t digest = 0;
          for (uint8_t byte : buffer->span()) {
            digest = digest * 131 + byte;
          }
          trace.push_back(digest);
          break;
        }
        case 2:
          trace.push_back(handle->WriteStreamData(
              std::string_view(body).substr(0, 1 + input % size), false,
              base::DoNothing()));
          break;
        case 3:
          trace.push_back(handle->WritevStreamData(
              buffers,
              {static_cast<int>(1 + input % 32),
               static_cast<int>(1 + (input >> 8) % 32)},
              false, base::DoNothing()));
          break;
        case 4:
          trace.push_back(handle->id());
          break;
        case 5:
          trace.push_back(handle->IsDoneReading());
          break;
        case 6:
          trace.push_back(handle->HasBytesToRead());
          break;
        case 7:
          trace.push_back(handle->stream_bytes_read());
          break;
        case 8:
          trace.push_back(handle->stream_bytes_written());
          break;
        case 9:
          trace.push_back(handle->NumBytesConsumed());
          break;
        case 10:
          handle->SetRequestIdempotency(input & 256 ? IDEMPOTENT
                                                    : NOT_IDEMPOTENT);
          trace.push_back(handle->GetRequestIdempotency());
          break;
        case 11:
          handle->DisableConnectionMigrationToCellularNetwork();
          trace.push_back(handle->can_migrate_to_cellular_network());
          break;
        case 12:
          trace.push_back(handle->IsFirstStream());
          break;
        case 13:
          trace.push_back(handle->IsOpen());
          break;
        case 14:
          if (step > operations / 2 && !detached) {
            stream->OnError(input & 512 ? ERR_ABORTED : ERR_CONNECTION_CLOSED);
            detached = true;
            base::RunLoop().RunUntilIdle();
          }
          trace.push_back(handle->connection_error());
          trace.push_back(handle->stream_error());
          break;
        case 15:
          trace.push_back(handle->fin_sent());
          trace.push_back(handle->fin_received());
          break;
        case 16:
          trace.push_back(handle->connection_wire_error());
          trace.push_back(handle->ietf_application_error());
          break;
        case 17:
          trace.push_back(
              handle->max_stream_limit_pending_delay().InMicroseconds());
          trace.push_back(handle->SupportsH3Datagram());
          break;
        case 18:
          if (handle->IsDoneReading()) {
            handle->OnFinRead();
          }
          trace.push_back(handle->fin_received());
          break;
      }
    }
    if (handle->IsOpen()) {
      handle->RegisterHttp3DatagramVisitor(&visitor);
    }
    handle->UnregisterHttp3DatagramVisitor();
    handle->UnregisterHttp3DatagramVisitor();
    // Exercise the Handle reset adapter while attached; detached handles leave
    // session-owned stream transport cleanup to the stream primitive.
    if (handle->IsOpen()) {
      handle->Reset(quic::QUIC_STREAM_CANCELLED);
    } else {
      stream->Reset(quic::QUIC_STREAM_CANCELLED);
    }
    handle.reset();
    base::RunLoop().RunUntilIdle();
    return trace;
  }

 private:
  base::test::TaskEnvironment tasks_;
  quic::ParsedQuicVersion version_;
  quic::test::MockQuicConnectionHelper helper_;
  quic::test::MockAlarmFactory alarms_;
  Session<Stream> session_;
  int next_stream_ = 0;
};
}  // namespace net::http3_rust_test
#endif  // NET_QUIC_HTTP3_CLIENT_STREAM_RUST_TEST_SUPPORT_H_
