// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_VULKAN_VULKAN_DESCRIPTOR_LAYOUT_H_
#define GPU_VULKAN_VULKAN_DESCRIPTOR_LAYOUT_H_

#include <vulkan/vulkan.h>

#include <memory>
#include <vector>

#include "base/macros.h"
#include "gpu/vulkan/vulkan_export.h"

namespace gpu {

class VulkanDeviceQueue;

class VULKAN_EXPORT VulkanDescriptorLayout {
 public:
  explicit VulkanDescriptorLayout(VulkanDeviceQueue* device_queue);
  ~VulkanDescriptorLayout();

  bool Initialize(const std::vector<VkDescriptorSetLayoutBinding>& layout);
  void Destroy();

  VkDescriptorSetLayout handle() const { return handle_; }

 private:
  VulkanDeviceQueue* device_queue_ = nullptr;
  VkDescriptorSetLayout handle_ = VK_NULL_HANDLE;

  DISALLOW_COPY_AND_ASSIGN(VulkanDescriptorLayout);
};

}  // namespace gpu

#endif  // GPU_VULKAN_VULKAN_DESCRIPTOR_LAYOUT_H_
