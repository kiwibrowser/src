// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_VULKAN_VULKAN_SURFACE_H_
#define GPU_VULKAN_VULKAN_SURFACE_H_

#include <vulkan/vulkan.h>

#include "gpu/vulkan/vulkan_device_queue.h"
#include "gpu/vulkan/vulkan_export.h"
#include "gpu/vulkan/vulkan_swap_chain.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/swap_result.h"

namespace gpu {

class VulkanDeviceQueue;
class VulkanSwapChain;

class VULKAN_EXPORT VulkanSurface {
 public:
  // Minimum bit depth of surface.
  enum Format {
    FORMAT_RGBA_32,
    FORMAT_RGB_16,

    NUM_SURFACE_FORMATS,
    DEFAULT_SURFACE_FORMAT = FORMAT_RGBA_32
  };

  VulkanSurface(VkInstance vk_instance, VkSurfaceKHR surface);

  ~VulkanSurface();

  bool Initialize(VulkanDeviceQueue* device_queue,
                  VulkanSurface::Format format);
  void Destroy();

  gfx::SwapResult SwapBuffers();

  VulkanSwapChain* GetSwapChain();

  void Finish();

 private:
  const VkInstance vk_instance_;
  gfx::Size size_;
  VkSurfaceKHR surface_ = VK_NULL_HANDLE;
  VkSurfaceFormatKHR surface_format_ = {};
  VulkanDeviceQueue* device_queue_ = nullptr;
  VulkanSwapChain swap_chain_;

  DISALLOW_COPY_AND_ASSIGN(VulkanSurface);
};

}  // namespace gpu

#endif  // GPU_VULKAN_VULKAN_SURFACE_H_
