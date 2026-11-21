// Copyright 2026 The Chromium Authors
// BSD-style license: see LICENSE.
#include "net/spdy/http2_stream_family_io_fixture.h"
#include "net/test/test_with_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net::http2_family_test {
class Http2StreamFamilyIoTest : public TestWithTaskEnvironment {};

template <class Pool>
IoResult RunOrdinary(IoCase selected) {
  IoInputs inputs(selected);
  NativeIoEnvironment environment(inputs);
  auto key = MakeIoKey<Pool>(inputs.url);
  return RunIoPath<Pool>(environment, key);
}

TEST_F(Http2StreamFamilyIoTest, BufferedGetPublicPath) {
  EXPECT_EQ(RunOrdinary<SpdySessionPoolOriginal>(IoCase::kBufferedGet),
            RunOrdinary<SpdySessionPool>(IoCase::kBufferedGet));
}
TEST_F(Http2StreamFamilyIoTest, PendingReadDeliversOnce) {
  EXPECT_EQ(RunOrdinary<SpdySessionPoolOriginal>(IoCase::kPendingGet),
            RunOrdinary<SpdySessionPool>(IoCase::kPendingGet));
}
TEST_F(Http2StreamFamilyIoTest, ChunkedUploadAndEcho) {
  EXPECT_EQ(RunOrdinary<SpdySessionPoolOriginal>(IoCase::kChunkedUpload),
            RunOrdinary<SpdySessionPool>(IoCase::kChunkedUpload));
}
TEST_F(Http2StreamFamilyIoTest, WindowStallAndResumeBeforeFinalUpload) {
  EXPECT_EQ(RunOrdinary<SpdySessionPoolOriginal>(IoCase::kFlowControlledUpload),
            RunOrdinary<SpdySessionPool>(IoCase::kFlowControlledUpload));
}
TEST_F(Http2StreamFamilyIoTest, CancelThenDestroySuppressesPendingRead) {
  EXPECT_EQ(RunOrdinary<SpdySessionPoolOriginal>(IoCase::kCancelPendingRead),
            RunOrdinary<SpdySessionPool>(IoCase::kCancelPendingRead));
}
}  // namespace net::http2_family_test
