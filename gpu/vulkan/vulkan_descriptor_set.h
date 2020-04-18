// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_VULKAN_VULKAN_DESCRIPTOR_SET_H_
#define GPU_VULKAN_VULKAN_DESCRIPTOR_SET_H_

#include <vulkan/vulkan.h>

#include <memory>
#include <vector>

#include "base/macros.h"
#include "gpu/vulkan/vulkan_export.h"

namespace gpu {

class VulkanDescriptorPool;
class VulkanDescriptorLayout;
class VulkanDeviceQueue;

class VULKAN_EXPORT VulkanDescriptorSet {
 public:
  ~VulkanDescriptorSet();

  bool Initialize(const VulkanDescriptorLayout* layout);
  void Destroy();

  void WriteToDescriptorSet(uint32_t dst_binding,
                            uint32_t dst_array_element,
                            uint32_t descriptor_count,
                            VkDescriptorType descriptor_type,
                            const VkDescriptorImageInfo* image_info,
                            const VkDescriptorBufferInfo* buffer_info,
                            const VkBufferView* texel_buffer_view);

  void CopyFromDescriptorSet(const VulkanDescriptorSet* source_set,
                             uint32_t src_binding,
                             uint32_t src_array_element,
                             uint32_t dst_binding,
                             uint32_t dst_array_element,
                             uint32_t descriptor_count);

  VkDescriptorSet handle() const { return handle_; }

 private:
  friend class VulkanDescriptorPool;
  VulkanDescriptorSet(VulkanDeviceQueue* device_queue,
                      VulkanDescriptorPool* descriptor_pool);

  VulkanDeviceQueue* device_queue_ = nullptr;
  VulkanDescriptorPool* descriptor_pool_ = nullptr;
  VkDescriptorSet handle_ = VK_NULL_HANDLE;

  DISALLOW_COPY_AND_ASSIGN(VulkanDescriptorSet);
};

}  // namespace gpu

#endif  // GPU_VULKAN_VULKAN_DESCRIPTOR_SET_H_
