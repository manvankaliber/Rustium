// Copyright 2026 The Chromium Authors
// BSD-style license: see LICENSE.
#ifndef NET_SPDY_HTTP2_STREAM_FAMILY_IO_FIXTURE_H_
#define NET_SPDY_HTTP2_STREAM_FAMILY_IO_FIXTURE_H_

#include <deque>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "base/check_op.h"
#include "base/functional/bind.h"
#include "base/memory/raw_ref.h"
#include "base/run_loop.h"
#include "base/strings/string_view_util.h"
#include "net/base/chunked_upload_data_stream.h"
#include "net/base/test_completion_callback.h"
#include "net/http/http_network_session.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_request_info.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_response_info.h"
#include "net/socket/socket_test_util.h"
#include "net/spdy/http2_session_family_factory.h"
#include "net/spdy/spdy_test_util_common.h"
#include "net/test/cert_test_util.h"
#include "net/test/test_data_directory.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"

namespace net::http2_family_test {

enum class IoCase {
  kBufferedGet,
  kPendingGet,
  kChunkedUpload,
  kFlowControlledUpload,
  kCancelPendingRead,
};

// Nonmovable because MockRead/MockWrite hold views of these native bytes.
struct IoInputs {
  explicit IoInputs(IoCase selected);
  ~IoInputs();
  IoInputs(const IoInputs&) = delete;
  IoInputs& operator=(const IoInputs&) = delete;
  const IoCase selected;
  const GURL url{kDefaultUrl};
  const std::string payload = "01234567abcdefgh";
  std::string initial_bytes;
  std::deque<spdy::SpdySerializedFrame> frames;
  std::vector<MockRead> reads;
  std::vector<MockWrite> writes;
};

struct IoResult {
  int status = ERR_FAILED;
  std::string body;
  size_t request_callbacks = 0;
  size_t body_callbacks = 0;
  bool consumed_all_reads = false;
  bool consumed_all_writes = false;
  bool operator==(const IoResult&) const = default;
  uint64_t Checksum() const;
};

// Caller-owned native services and TLS/data fixtures. Constructed outside
// operation timing for BOTH implementations. The native HttpNetworkSession is
// used only for SSLClientContext; its production SPDY pool is never inspected,
// opened or substituted for the separately typed family pool.
class NativeIoEnvironment {
 public:
  explicit NativeIoEnvironment(const IoInputs& inputs);
  ~NativeIoEnvironment();
  NativeIoEnvironment(const NativeIoEnvironment&) = delete;
  NativeIoEnvironment& operator=(const NativeIoEnvironment&) = delete;
  Http2SessionFamilyServices Services();
  std::unique_ptr<StreamSocket> ReleaseSocket();
  // Borrowed wire storage must outlive this environment and its socket data.
  const raw_ref<const IoInputs> inputs;
  SpdySessionDependencies dependencies;
  std::unique_ptr<HttpNetworkSession> provider_session;
  SequencedSocketData data;
  SSLSocketDataProvider ssl;
  std::unique_ptr<SSLClientSocket> socket;
  // Upload must outlive request's non-owning pointer, including teardown.
  std::unique_ptr<ChunkedUploadDataStream> upload;
  HttpRequestInfo request;
  HttpRequestHeaders headers;
  HttpResponseInfo response;
};

template <class Pool>
typename Http2SessionFamilyFactory<Pool>::Key MakeIoKey(const GURL& url) {
  if constexpr (std::is_same_v<Pool, SpdySessionPoolOriginal>) {
    return SpdySessionKeyOriginal(
        proxy_value_rust_reference::HostPortPair(
            url.host(), static_cast<uint16_t>(url.EffectiveIntPort())),
        PRIVACY_MODE_DISABLED, proxy_value_rust_reference::ProxyChain::Direct(),
        SessionUsage::kDestination, SocketTag(), NetworkAnonymizationKey(),
        SecureDnsPolicy::kAllow, false, handles::kInvalidNetworkHandle);
  } else {
    return SpdySessionKey(HostPortPair::FromURL(url), PRIVACY_MODE_DISABLED,
                          ProxyChain::Direct(), SessionUsage::kDestination,
                          SocketTag(), NetworkAnonymizationKey(),
                          SecureDnsPolicy::kAllow, false,
                          handles::kInvalidNetworkHandle);
  }
}

inline CompletionOnceCallback CountCallback(TestCompletionCallback& callback,
                                             size_t& count) {
  return base::BindOnce(
      [](size_t* count, CompletionOnceCallback next, int result) {
        ++*count;
        std::move(next).Run(result);
      }, &count, callback.callback());
}

// Complete bounded public HTTP2 operation including independent pool/session
// construction, client preface/settings, stream request, frame read/write,
// upload/window stall/resume or pending read cancellation and teardown. TLS
// socket connection, immutable wire bytes, caller headers/response storage and
// typed Key construction are prepared by the caller outside timing.
template <class Pool>
IoResult RunIoPath(
    NativeIoEnvironment& environment,
    const typename Http2SessionFamilyFactory<Pool>::Key& key) {
  using Factory = Http2SessionFamilyFactory<Pool>;
  IoResult result;
  {
    Factory factory(environment.Services());
    auto session_result = factory.OpenSession(key, environment.ReleaseSocket());
    CHECK(session_result.has_value());
    auto session = session_result.value();
    CHECK(session);
    session->SetTimeToBufferSmallWindowUpdates(base::TimeDelta::Max());

    // Both sides process exactly the scripted initial settings before creating
    // the stream. Flow-control case applies peer INITIAL_WINDOW_SIZE=8 here.
    environment.data.RunUntilPaused();
    CHECK(environment.data.IsPaused());
    auto http = std::make_unique<typename Factory::HttpStream>(
        session, NetLogSource(), base::flat_set<std::string>());
    http->RegisterRequest(&environment.request);
    TestCompletionCallback initialized;
    const int init = http->InitializeStream(
        true, DEFAULT_PRIORITY, NetLogWithSource(), initialized.callback());
    CHECK_EQ(initialized.GetResult(init), OK);

    TestCompletionCallback sent;
    const int send = http->SendRequest(
        environment.headers, &environment.response,
        CountCallback(sent, result.request_callbacks));
    CHECK_EQ(send, ERR_IO_PENDING);
    environment.data.Resume();
    if (environment.inputs->selected == IoCase::kFlowControlledUpload) {
      environment.data.RunUntilPaused();
      base::RunLoop().RunUntilIdle();
      CHECK(http->stream());
      CHECK(http->stream()->send_stalled_by_flow_control());
      CHECK(!sent.have_result());
      environment.data.Resume();
    }
    CHECK_EQ(sent.GetResult(send), OK);
    CHECK_EQ(result.request_callbacks, 1u);

    const bool pending =
        environment.inputs->selected == IoCase::kPendingGet ||
        environment.inputs->selected == IoCase::kCancelPendingRead;
    if (pending) {
      environment.data.RunUntilPaused();
      CHECK(environment.data.IsPaused());
    } else {
      base::RunLoop().RunUntilIdle();
    }
    TestCompletionCallback headers;
    CHECK_EQ(http->ReadResponseHeaders(headers.callback()), OK);
    CHECK(environment.response.headers);
    CHECK_EQ(environment.response.headers->response_code(), 200);

    auto buffer = base::MakeRefCounted<IOBufferWithSize>(
        static_cast<int>(environment.inputs->payload.size()));
    if (environment.inputs->selected == IoCase::kCancelPendingRead) {
      TestCompletionCallback cancelled;
      CHECK_EQ(http->ReadResponseBody(
                   buffer.get(), buffer->size(),
                   CountCallback(cancelled, result.body_callbacks)),
               ERR_IO_PENDING);
      http->Close(true);
      TestCompletionCallback after_close;
      result.status = http->ReadResponseHeaders(after_close.callback());
      CHECK_EQ(result.status, ERR_ABORTED);
      http.reset();
      // Later native completion sees a cancelled callback and a destroyed
      // public HTTP stream. The held caller IOBuffer stays alive independently.
      environment.data.Resume();
      base::RunLoop().RunUntilIdle();
      CHECK(!cancelled.have_result());
      CHECK_EQ(result.body_callbacks, 0u);
    } else {
      bool first = true;
      for (;;) {
        TestCompletionCallback body;
        const int read = http->ReadResponseBody(
            buffer.get(), buffer->size(),
            CountCallback(body, result.body_callbacks));
        if (pending && first) {
          CHECK_EQ(read, ERR_IO_PENDING);
          environment.data.Resume();
        }
        first = false;
        const int bytes = body.GetResult(read);
        CHECK_GE(bytes, 0);
        CHECK_LE(bytes, buffer->size());
        if (bytes == 0) break;
        result.body.append(base::as_string_view(
            buffer->span().first(static_cast<size_t>(bytes))));
      }
      CHECK_EQ(result.body, environment.inputs->payload);
      if (pending) CHECK_EQ(result.body_callbacks, 1u);
      CHECK(http->IsResponseBodyComplete());
      http->Close(false);
      http.reset();
      result.status = OK;
    }
    base::RunLoop().RunUntilIdle();
  }  // Include independently typed pool/session teardown in the operation.
  base::RunLoop().RunUntilIdle();
  result.consumed_all_reads = environment.data.AllReadDataConsumed();
  result.consumed_all_writes = environment.data.AllWriteDataConsumed();
  CHECK(result.consumed_all_reads);
  CHECK(result.consumed_all_writes);
  return result;
}

}  // namespace net::http2_family_test
#endif  // NET_SPDY_HTTP2_STREAM_FAMILY_IO_FIXTURE_H_
