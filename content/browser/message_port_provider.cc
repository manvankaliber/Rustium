// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/message_port_provider.h"

#include <optional>
#include <utility>

#include "base/unguessable_token.h"
#include "build/build_config.h"
#include "build/chromecast_buildflags.h"
#include "content/browser/renderer_host/page_impl.h"
#include "content/browser/renderer_host/render_frame_host_impl.h"
#include "content/browser/renderer_host/render_process_host_impl.h"
#include "content/browser/renderer_host/render_view_host_impl.h"
#include "content/public/browser/browser_thread.h"
#include "third_party/blink/public/common/messaging/string_message_codec.h"


using blink::MessagePortChannel;

namespace content {
namespace {

void PostMessageToFrameInternal(
    Page& page,
    const url::Origin* source_origin,
    const url::Origin* target_origin,
    const blink::WebMessagePayload& data,
    std::vector<blink::MessagePortDescriptor> ports) {
  CHECK_CURRENTLY_ON(BrowserThread::UI, base::NotFatalUntil::M159);

  // TODO(chrisha): Kill off MessagePortChannel, as MessagePortDescriptor now
  // plays that role.
  std::vector<MessagePortChannel> channels;
  for (auto& port : ports) {
    channels.emplace_back(MessagePortChannel(std::move(port)));
  }

  // Note that this is reached by every embedder on the platform, not just
  // WebView -- Chrome Custom Tabs and Trusted Web Activities post through here
  // too, with a verified source origin -- so nothing here may assume a WebView
  // renderer on the other end. SharedArrayBuffer payloads are consequently not
  // deliverable from here at all: the regions backing them can only travel over
  // a WebView-specific interface, so WebView diverts those messages before they
  // reach this function.
  blink::TransferableMessage message = blink::EncodeWebMessagePayload(data);
  message.ports = std::move(channels);
  // As the message is posted from the embedder and not from another renderer,
  // set the agent cluster ID to the embedder's.
  message.sender_agent_cluster_id =
      blink::WebMessagePort::GetEmbedderAgentClusterID();

  RenderFrameHostImpl* rfh =
      static_cast<RenderFrameHostImpl*>(&page.GetMainDocument());
  rfh->PostMessageEvent(std::nullopt, source_origin, target_origin,
                        std::move(message));
}


}  // namespace

// static
void MessagePortProvider::PostMessageToFrame(
    Page& page,
    const url::Origin* source_origin,
    const url::Origin* target_origin,
    const blink::WebMessagePayload& data) {
  PostMessageToFrameInternal(page, source_origin, target_origin, data,
                             std::vector<blink::MessagePortDescriptor>());
}



}  // namespace content
