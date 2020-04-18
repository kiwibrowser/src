// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_VULKAN_VULKAN_IMPLEMENTATION_ANDROID_H_
#define GPU_VULKAN_VULKAN_IMPLEMENTATION_ANDROID_H_

#include <memory>

#include "gpu/vulkan/vulkan_export.h"
#include "gpu/vulkan/vulkan_implementation.h"
#include "gpu/vulkan/vulkan_instance.h"
#include "gpu/vulkan/vulkan_surface.h"

namespace gpu {

class VULKAN_EXPORT VulkanImplementationAndroid : public VulkanImplementation {
 public:
  VulkanImplementationAndroid();
  ~VulkanImplementationAndroid() override;

  // VulkanImplementation:
  bool InitializeVulkanInstance() override;
  VkInstance GetVulkanInstance() override;
  std::unique_ptr<VulkanSurface> CreateViewSurface(
      gfx::AcceleratedWidget window) override;
  bool GetPhysicalDevicePresentationSupport(
      VkPhysicalDevice device,
      const std::vector<VkQueueFamilyProperties>& queue_family_properties,
      uint32_t queue_family_index) override;

 private:
  VulkanInstance vulkan_instance_;

  DISALLOW_COPY_AND_ASSIGN(VulkanImplementationAndroid);
};

}  // namespace gpu

#endif  // GPU_VULKAN_VULKAN_IMPLEMENTATION_ANDROID_H_
