// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_VULKAN_VULKAN_DESCRIPTOR_POOL_H_
#define GPU_VULKAN_VULKAN_DESCRIPTOR_POOL_H_

#include <vulkan/vulkan.h>

#include <memory>
#include <vector>

#include "base/macros.h"

namespace gpu {

class VulkanDescriptorLayout;
class VulkanDescriptorSet;
class VulkanDeviceQueue;

class VulkanDescriptorPool {
 public:
  explicit VulkanDescriptorPool(VulkanDeviceQueue* device_queue);
  ~VulkanDescriptorPool();

  bool Initialize(uint32_t max_descriptor_sets,
                  const std::vector<VkDescriptorPoolSize>& pool_sizes);
  void Destroy();

  std::unique_ptr<VulkanDescriptorSet> CreateDescriptorSet(
      const VulkanDescriptorLayout* layout);
  VkDescriptorPool handle() { return handle_; }

 private:
  friend class VulkanDescriptorSet;

  void IncrementDescriptorSetCount();
  void DecrementDescriptorSetCount();

  VulkanDeviceQueue* device_queue_ = nullptr;
  VkDescriptorPool handle_ = VK_NULL_HANDLE;
  uint32_t max_descriptor_sets_ = 0;
  uint32_t descriptor_count_ = 0;

  DISALLOW_COPY_AND_ASSIGN(VulkanDescriptorPool);
};

}  // namespace gpu

#endif  // GPU_VULKAN_VULKAN_DESCRIPTOR_POOL_H_
