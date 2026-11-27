// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_VR_OPENXR_OPENXR_SWAPCHAIN_INFO_H_
#define DEVICE_VR_OPENXR_OPENXR_SWAPCHAIN_INFO_H_

#include "gpu/command_buffer/client/client_shared_image.h"
#include "ui/gfx/geometry/size.h"

#if BUILDFLAG(IS_WIN)
#include <d3d11_4.h>
#include <wrl.h>
#endif


#if BUILDFLAG(IS_LINUX)
#include <vulkan/vulkan_core.h>
#endif

namespace device {

// TODO(crbug.com/40909689): Refactor this class.
struct OpenXrSwapchainInfo {
 public:
#if BUILDFLAG(IS_WIN)
  explicit OpenXrSwapchainInfo(ID3D11Texture2D*);
#elif BUILDFLAG(IS_LINUX)
  explicit OpenXrSwapchainInfo(VkImage vk_image);
#endif
  OpenXrSwapchainInfo();
  virtual ~OpenXrSwapchainInfo();
  OpenXrSwapchainInfo(OpenXrSwapchainInfo&&);
  OpenXrSwapchainInfo& operator=(OpenXrSwapchainInfo&&);

  void Clear();

  scoped_refptr<gpu::ClientSharedImage> shared_image;
  gpu::SyncToken sync_token;

#if BUILDFLAG(IS_WIN)
  // When shared images are being used, there is a corresponding
  // ClientSharedImage and D3D11Fence for each D3D11 texture in the vector.
  raw_ptr<ID3D11Texture2D> d3d11_texture = nullptr;
  // If a shared handle cannot be created for the swap chain texture, a second
  // texture which is shareable will be created and passed to the renderer
  // process. When the frame is complete it will be copied to the swap chain
  // texture prior to submission.
  Microsoft::WRL::ComPtr<ID3D11Texture2D> d3d11_shared_texture = nullptr;
  Microsoft::WRL::ComPtr<ID3D11Fence> d3d11_fence;
#elif BUILDFLAG(IS_LINUX)
  // The Vulkan image handle from the OpenXR swapchain.
  VkImage vk_image = VK_NULL_HANDLE;
#endif
};

}  // namespace device

#endif  // DEVICE_VR_OPENXR_OPENXR_SWAPCHAIN_INFO_H_
