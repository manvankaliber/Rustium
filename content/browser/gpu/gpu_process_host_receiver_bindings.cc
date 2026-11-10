// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file exposes services from the browser process to the GPU process.
#include "build/build_config.h"
#include "content/browser/gpu/gpu_process_host.h"
#include "content/browser/webnn/webnn_browser_host_impl.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/common/content_client.h"
#include "mojo/public/cpp/bindings/generic_pending_receiver.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "services/webnn/public/mojom/webnn_browser_host.mojom.h"



namespace content {

namespace {


}  // namespace

void GpuProcessHost::BindHostReceiver(
    mojo::GenericPendingReceiver generic_receiver) {
  if (auto r = generic_receiver.As<webnn::mojom::WebNNBrowserHost>()) {
    WebNNBrowserHostImpl::Create(std::move(r));
    return;
  }



  GetContentClient()->browser()->BindGpuHostReceiver(
      std::move(generic_receiver));
}

}  // namespace content
