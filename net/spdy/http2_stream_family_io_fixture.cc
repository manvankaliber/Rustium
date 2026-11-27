// Copyright 2026 The Chromium Authors
// BSD-style license: see LICENSE.
#include "net/spdy/http2_stream_family_io_fixture.h"

#include "net/base/address_list.h"
#include "net/base/ip_address.h"
#include "net/socket/ssl_client_socket.h"
#include "net/ssl/ssl_config.h"

namespace net::http2_family_test {

IoInputs::IoInputs(IoCase selected) : selected(selected) {
  SpdyTestUtil wire(/*use_priority_header=*/true);
  // Exact peer-independent connection input. Default family settings differ
  // from protocol defaults only for ENABLE_PUSH and MAX_HEADER_LIST_SIZE.
  spdy::SettingsMap initial;
  initial[spdy::SETTINGS_ENABLE_PUSH] = 0;
  initial[spdy::SETTINGS_MAX_HEADER_LIST_SIZE] = 65536;
  initial_bytes = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";
  frames.push_back(wire.ConstructSpdySettings(initial));
  initial_bytes.append(std::string_view(frames.back()));
  writes.emplace_back(ASYNC, 0, initial_bytes);
  int sequence = 1;

  auto read_frame = [&](spdy::SpdySerializedFrame frame) {
    frames.push_back(std::move(frame));
    reads.push_back(CreateMockRead(frames.back(), sequence++));
  };
  auto write_frame = [&](spdy::SpdySerializedFrame frame) {
    frames.push_back(std::move(frame));
    writes.push_back(CreateMockWrite(frames.back(), sequence++));
  };
  const bool flow = selected == IoCase::kFlowControlledUpload;
  const bool uploading = selected == IoCase::kChunkedUpload || flow;
  if (flow) {
    spdy::SettingsMap peer;
    peer[spdy::SETTINGS_INITIAL_WINDOW_SIZE] = 8;
    read_frame(wire.ConstructSpdySettings(peer));
    write_frame(wire.ConstructSpdySettingsAck());
  }
  // Handshake/session policy is completed before the stream request starts.
  reads.emplace_back(ASYNC, ERR_IO_PENDING, sequence++);
  if (uploading) {
    write_frame(wire.ConstructChunkedSpdyPost(
        base::span<const std::string_view>()));
    if (flow) {
      write_frame(wire.ConstructSpdyDataFrame(
          1, std::string_view(payload).substr(0, 8), false));
      // Force the first frame's write callback to exhaust the stream window
      // before the peer grants credit; this exercises an actual stalled path.
      reads.emplace_back(ASYNC, ERR_IO_PENDING, sequence++);
      read_frame(wire.ConstructSpdyWindowUpdate(1, 8));
      write_frame(wire.ConstructSpdyDataFrame(
          1, std::string_view(payload).substr(8), true));
    } else {
      write_frame(wire.ConstructSpdyDataFrame(1, payload, true));
    }
    read_frame(wire.ConstructSpdyPostReply(
        base::span<const std::string_view>()));
  } else {
    write_frame(wire.ConstructSpdyGet(
        base::span<const std::string_view>(), 1, LOWEST));
    read_frame(wire.ConstructSpdyGetReply(
        base::span<const std::string_view>(), 1));
  }
  if (selected == IoCase::kPendingGet ||
      selected == IoCase::kCancelPendingRead) {
    reads.emplace_back(ASYNC, ERR_IO_PENDING, sequence++);
  }
  if (selected == IoCase::kCancelPendingRead) {
    write_frame(wire.ConstructSpdyRstStream(1, spdy::ERROR_CODE_CANCEL));
  } else {
    read_frame(wire.ConstructSpdyDataFrame(1, payload, true));
  }
  reads.emplace_back(ASYNC, 0, sequence++);
}

IoInputs::~IoInputs() = default;

uint64_t IoResult::Checksum() const {
  uint64_t value = static_cast<uint64_t>(status);
  for (unsigned char byte : body) value = value * 131 + byte;
  value = value * 131 + request_callbacks;
  value = value * 131 + body_callbacks;
  value = value * 131 + consumed_all_reads;
  return value * 131 + consumed_all_writes;
}

NativeIoEnvironment::NativeIoEnvironment(const IoInputs& source)
    : inputs(source),
      provider_session(SpdySessionDependencies::SpdyCreateSession(
          &dependencies)),
      data(MockConnect(SYNCHRONOUS, OK), inputs->reads, inputs->writes),
      ssl(SYNCHRONOUS, OK) {
  ssl.ssl_info.cert =
      ImportCertFromFile(GetTestCertsDirectory(), "spdy_pooling.pem");
  CHECK(ssl.ssl_info.cert);
  ssl.next_proto = NextProto::kProtoHTTP2;
  auto tcp = std::make_unique<MockTCPClientSocket>(
      AddressList::CreateFromIPAddress(IPAddress::IPv4Localhost(), 443),
      nullptr, &data);
  TestCompletionCallback connected;
  CHECK_EQ(connected.GetResult(tcp->Connect(connected.callback())), OK);
  socket = std::make_unique<MockSSLClientSocket>(
      std::move(tcp), HostPortPair::FromURL(inputs->url), SSLConfig(), &ssl);
  TestCompletionCallback secured;
  CHECK_EQ(secured.GetResult(socket->Connect(secured.callback())), OK);
  request.url = inputs->url;
  request.method = "GET";
  request.traffic_annotation =
      MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS);
  if (inputs->selected == IoCase::kChunkedUpload ||
      inputs->selected == IoCase::kFlowControlledUpload) {
    request.method = "POST";
    upload = std::make_unique<ChunkedUploadDataStream>(0);
    TestCompletionCallback initialized;
    CHECK_EQ(initialized.GetResult(
                 upload->Init(initialized.callback(), NetLogWithSource())),
             OK);
    upload->AppendData(base::as_byte_span(inputs->payload), true);
    request.upload_data_stream = upload.get();
  }
}

NativeIoEnvironment::~NativeIoEnvironment() = default;

Http2SessionFamilyServices NativeIoEnvironment::Services() {
  return {dependencies.GetHostResolver(), provider_session->ssl_client_context(),
          dependencies.http_server_properties.get(),
          dependencies.transport_security_state.get(), nullptr};
}

std::unique_ptr<StreamSocket> NativeIoEnvironment::ReleaseSocket() {
  return std::move(socket);
}

}  // namespace net::http2_family_test
