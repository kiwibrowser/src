// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_VULKAN_VULKAN_IMPLEMENTATION_H_
#define GPU_VULKAN_VULKAN_IMPLEMENTATION_H_

#include <vulkan/vulkan.h>
#include <memory>

#include "base/macros.h"
#include "gpu/vulkan/vulkan_export.h"
#include "ui/gfx/native_widget_types.h"

namespace gpu {

class VulkanDeviceQueue;
class VulkanSurface;

// This object provides factory functions for creating vulkan objects that use
// platform-specific extensions (e.g. for creation of VkSurfaceKHR objects).
class VULKAN_EXPORT VulkanImplementation {
 public:
  VulkanImplementation();

  virtual ~VulkanImplementation();

  virtual bool InitializeVulkanInstance() = 0;

  virtual VkInstance GetVulkanInstance() = 0;

  virtual std::unique_ptr<VulkanSurface> CreateViewSurface(
      gfx::AcceleratedWidget window) = 0;

  virtual bool GetPhysicalDevicePresentationSupport(
      VkPhysicalDevice device,
      const std::vector<VkQueueFamilyProperties>& queue_family_properties,
      uint32_t queue_family_index) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(VulkanImplementation);
};

VULKAN_EXPORT
std::unique_ptr<VulkanDeviceQueue> CreateVulkanDeviceQueue(
    VulkanImplementation* vulkan_implementation,
    uint32_t option);

}  // namespace gpu

#endif  // GPU_VULKAN_VULKAN_IMPLEMENTATION_H_
