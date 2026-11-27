// Copyright 2026 The Chromium Authors
// BSD-style license: see LICENSE.
#ifndef NET_SPDY_HTTP2_STREAM_FAMILY_RUST_TEST_SUPPORT_H_
#define NET_SPDY_HTTP2_STREAM_FAMILY_RUST_TEST_SUPPORT_H_

#include <array>
#include <limits>
#include <memory>
#include <type_traits>
#include <vector>

#include "base/check.h"
#include "base/check_op.h"
#include "net/base/net_errors.h"
#include "net/spdy/spdy_stream.h"
#include "net/spdy/spdy_stream_family_original.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"

namespace net::http2_family_test {
template <class Stream>
using BufferFor = std::conditional_t<std::is_same_v<Stream, SpdyStreamOriginal>,
                                     SpdyBufferOriginal, SpdyBuffer>;
template <class Stream>
using SessionFor = std::conditional_t<std::is_same_v<Stream, SpdyStreamOriginal>,
                                      SpdySessionOriginal, SpdySession>;

template <class Stream>
class Delegate final : public Stream::Delegate {
 public:
  void OnHeadersSent() override { events.push_back(1); }
  void OnEarlyHintsReceived(const quiche::HttpHeaderBlock&) override {
    events.push_back(2);
  }
  void OnHeadersReceived(const quiche::HttpHeaderBlock&) override {
    events.push_back(3);
  }
  void OnDataReceived(std::unique_ptr<BufferFor<Stream>>) override {
    events.push_back(4);
  }
  void OnDataSent() override { events.push_back(5); }
  void OnTrailers(const quiche::HttpHeaderBlock&) override {
    events.push_back(6);
  }
  void OnClose(int status) override {
    events.push_back(7);
    close_status = status;
  }
  bool CanGreaseFrameType() const override { return false; }
  NetLogSource source_dependency() const override { return {}; }
  std::vector<int> events;
  int close_status = ERR_FAILED;
};

struct Inputs {
  GURL url{"https://example.test/"};
  quiche::HttpHeaderBlock early;
  quiche::HttpHeaderBlock final;
  quiche::HttpHeaderBlock trailers;
  Inputs();
  ~Inputs();
  // Preserve the implicit move operations suppressed by declaring a destructor.
  Inputs(Inputs&&);
  Inputs& operator=(Inputs&&);
};

// Isolated public stream-control domain: no operation in these cases reaches
// a native session/socket primitive. An empty weak session deliberately makes
// that boundary explicit; these cases are not end-to-end HTTP/2 I/O evidence.
template <class Stream>
std::unique_ptr<Stream> Create(const Inputs& inputs, int32_t window = 1024) {
  if constexpr (std::is_same_v<Stream, SpdyStreamOriginal>) {
    return std::make_unique<Stream>(
        SPDY_REQUEST_RESPONSE_STREAM_ORIGINAL,
        base::WeakPtr<SessionFor<Stream>>(), inputs.url, LOWEST, window, 65535,
        NetLogWithSource(), TRAFFIC_ANNOTATION_FOR_TESTS, false);
  } else {
    return std::make_unique<Stream>(
        SPDY_REQUEST_RESPONSE_STREAM,
        base::WeakPtr<SessionFor<Stream>>(), inputs.url, LOWEST, window, 65535,
        NetLogWithSource(), TRAFFIC_ANNOTATION_FOR_TESTS, false);
  }
}

template <class Stream>
uint64_t RunCase(size_t kind, const Inputs& inputs, bool verify) {
  Delegate<Stream> delegate;
  auto stream = Create<Stream>(inputs);
  uint64_t checksum = stream->IsIdle() + stream->send_window_size();
  if (kind == 0) {
    stream->OnClose(OK);
  } else if (kind == 1 || kind == 2) {
    stream->SetDelegate(&delegate);
    stream->set_stream_id(1);
    CHECK_EQ(stream->OnHeadersSent(), OK);
    if (kind == 2) {
      stream->OnHeadersReceived(inputs.early, base::Time::Now(),
                                base::TimeTicks::Now());
    }
    stream->OnHeadersReceived(inputs.final, base::Time::Now(),
                              base::TimeTicks::Now());
    if (kind == 2) {
      stream->OnHeadersReceived(inputs.trailers, base::Time::Now(),
                                base::TimeTicks::Now());
    }
    stream->OnClose(OK);
    if (verify) {
      const std::vector<int> expected = kind == 2
          ? std::vector<int>{2, 3, 6, 7} : std::vector<int>{3, 7};
      CHECK(delegate.events == expected);
      CHECK_EQ(delegate.close_status, OK);
      CHECK_EQ(stream->stream_id(), 0u);
    }
    for (int event : delegate.events) {
      checksum = checksum * 131 + event;
    }
  } else if (kind == 3) {
    for (int delta : {1, 128, -64, 32, -97}) {
      CHECK(stream->AdjustSendWindowSize(delta));
      checksum = checksum * 131 + stream->send_window_size();
    }
    stream->OnClose(OK);
  } else if (kind == 4) {
    stream->OnClose(ERR_ABORTED);
    CHECK(stream->AdjustSendWindowSize(std::numeric_limits<int32_t>::max()));
    if (verify) {
      CHECK_EQ(stream->send_window_size(), 1024);
    }
  }
  CHECK(stream->IsClosed());
  return checksum;
}
}  // namespace net::http2_family_test
#endif  // NET_SPDY_HTTP2_STREAM_FAMILY_RUST_TEST_SUPPORT_H_
