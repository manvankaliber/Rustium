// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/widget/compositing/render_frame_metadata_observer_impl.h"

#include "base/run_loop.h"
#include "base/test/task_environment.h"
#include "build/build_config.h"
#include "cc/mojom/render_frame_metadata.mojom-blink.h"
#include "cc/trees/render_frame_metadata.h"
#include "components/viz/common/quads/compositor_frame_metadata.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/platform/testing/runtime_enabled_features_test_helpers.h"

namespace blink {
namespace {

ACTION_P(InvokeClosure, closure) {
  closure.Run();
}

}  // namespace

class MockRenderFrameMetadataObserverClient
    : public cc::mojom::blink::RenderFrameMetadataObserverClient {
 public:
  MockRenderFrameMetadataObserverClient(
      mojo::PendingReceiver<cc::mojom::blink::RenderFrameMetadataObserverClient>
          client_receiver,
      mojo::PendingRemote<cc::mojom::blink::RenderFrameMetadataObserver>
          observer)
      : render_frame_metadata_observer_client_receiver_(
            this,
            std::move(client_receiver)),
        render_frame_metadata_observer_remote_(std::move(observer)) {}
  MockRenderFrameMetadataObserverClient(
      const MockRenderFrameMetadataObserverClient&) = delete;
  MockRenderFrameMetadataObserverClient& operator=(
      const MockRenderFrameMetadataObserverClient&) = delete;

  MOCK_METHOD2(OnRenderFrameMetadataChanged,
               void(uint32_t frame_token,
                    const cc::RenderFrameMetadata& metadata));
  MOCK_METHOD1(OnFrameSubmissionForTesting, void(uint32_t frame_token));

 private:
  mojo::Receiver<cc::mojom::blink::RenderFrameMetadataObserverClient>
      render_frame_metadata_observer_client_receiver_;
  mojo::Remote<cc::mojom::blink::RenderFrameMetadataObserver>
      render_frame_metadata_observer_remote_;
};

class RenderFrameMetadataObserverImplTest : public testing::Test {
 public:
  RenderFrameMetadataObserverImplTest() = default;
  RenderFrameMetadataObserverImplTest(
      const RenderFrameMetadataObserverImplTest&) = delete;
  RenderFrameMetadataObserverImplTest& operator=(
      const RenderFrameMetadataObserverImplTest&) = delete;
  ~RenderFrameMetadataObserverImplTest() override = default;

  RenderFrameMetadataObserverImpl& observer_impl() { return *observer_impl_; }

  MockRenderFrameMetadataObserverClient& client() { return *client_; }

  // testing::Test:
  void SetUp() override {
    mojo::PendingRemote<cc::mojom::blink::RenderFrameMetadataObserver>
        observer_remote;
    mojo::PendingReceiver<cc::mojom::blink::RenderFrameMetadataObserver>
        receiver = observer_remote.InitWithNewPipeAndPassReceiver();
    mojo::PendingRemote<cc::mojom::blink::RenderFrameMetadataObserverClient>
        client_remote;

    client_ = std::make_unique<
        testing::NiceMock<MockRenderFrameMetadataObserverClient>>(
        client_remote.InitWithNewPipeAndPassReceiver(),
        std::move(observer_remote));
    observer_impl_ = std::make_unique<RenderFrameMetadataObserverImpl>(
        std::move(receiver), std::move(client_remote));
    observer_impl_->BindToCurrentSequence();
  }

  void TearDown() override {
    observer_impl_.reset();
    client_.reset();
    task_environment_.RunUntilIdle();
  }

 private:
  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<testing::NiceMock<MockRenderFrameMetadataObserverClient>>
      client_;
  std::unique_ptr<RenderFrameMetadataObserverImpl> observer_impl_;
};

// This test verifies that the RenderFrameMetadataObserverImpl picks up
// the frame token from CompositorFrameMetadata and passes it along to the
// client. This test also verifies that the RenderFrameMetadata object is
// passed along to the client.
TEST_F(RenderFrameMetadataObserverImplTest, ShouldSendFrameToken) {
  viz::CompositorFrameMetadata compositor_frame_metadata;
  compositor_frame_metadata.send_frame_token_to_embedder = false;
  compositor_frame_metadata.frame_token = 1337;
  cc::RenderFrameMetadata render_frame_metadata;
  render_frame_metadata.is_mobile_optimized = true;
  observer_impl().OnRenderFrameSubmission(render_frame_metadata,
                                          &compositor_frame_metadata,
                                          false /* force_send */);
  // |is_mobile_optimized| should be synchronized with frame activation so
  // RenderFrameMetadataObserverImpl should ask for the frame token from
  // Viz.
  EXPECT_TRUE(compositor_frame_metadata.send_frame_token_to_embedder);
  {
    base::RunLoop run_loop;
    EXPECT_CALL(client(),
                OnRenderFrameMetadataChanged(1337, render_frame_metadata))
        .WillOnce(InvokeClosure(run_loop.QuitClosure()));
    run_loop.Run();
  }
}


// This test verifies that a request to force send metadata is respected.
TEST_F(RenderFrameMetadataObserverImplTest, ForceSendMetadata) {
  const uint32_t expected_frame_token = 1337;
  viz::CompositorFrameMetadata compositor_frame_metadata;
  compositor_frame_metadata.send_frame_token_to_embedder = false;
  compositor_frame_metadata.frame_token = expected_frame_token;
  cc::RenderFrameMetadata render_frame_metadata;
  observer_impl().OnRenderFrameSubmission(render_frame_metadata,
                                          &compositor_frame_metadata,
                                          false /* force_send */);
  // The first RenderFrameMetadata will always get a corresponding frame token
  // from Viz because this is the first frame.
  EXPECT_TRUE(compositor_frame_metadata.send_frame_token_to_embedder);
  {
    base::RunLoop run_loop;
    EXPECT_CALL(client(), OnRenderFrameMetadataChanged(expected_frame_token,
                                                       render_frame_metadata))
        .WillOnce(InvokeClosure(run_loop.QuitClosure()));
    run_loop.Run();
  }

  // Submit twice with no changes, but once with |force_send|. We should get
  // exactly one call to OnRenderFrameMetadataChanged.
  observer_impl().OnRenderFrameSubmission(render_frame_metadata,
                                          &compositor_frame_metadata,
                                          false /* force_send */);
  observer_impl().OnRenderFrameSubmission(
      render_frame_metadata, &compositor_frame_metadata, true /* force_send */);
  // Force send does not trigger sending a frame token.
  EXPECT_FALSE(compositor_frame_metadata.send_frame_token_to_embedder);
  {
    base::RunLoop run_loop;
    // The 0u frame token indicates that the client should not expect
    // a corresponding frame token from Viz.
    EXPECT_CALL(client(),
                OnRenderFrameMetadataChanged(0u, render_frame_metadata))
        .WillOnce(InvokeClosure(run_loop.QuitClosure()));
    run_loop.Run();
  }

  // Update the metadata and send one more message to ensure that no spurious
  // OnRenderFrameMetadataChanged messages were generated.
  render_frame_metadata.is_scroll_offset_at_top =
      !render_frame_metadata.is_scroll_offset_at_top;
  observer_impl().OnRenderFrameSubmission(render_frame_metadata,
                                          &compositor_frame_metadata,
                                          false /* force_send */);
  {
    base::RunLoop run_loop;
    EXPECT_CALL(client(), OnRenderFrameMetadataChanged(expected_frame_token,
                                                       render_frame_metadata))
        .WillOnce(InvokeClosure(run_loop.QuitClosure()));
    run_loop.Run();
  }
}


}  // namespace blink
