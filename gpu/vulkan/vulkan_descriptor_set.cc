// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/vulkan/vulkan_descriptor_set.h"

#include "base/logging.h"
#include "gpu/vulkan/vulkan_descriptor_layout.h"
#include "gpu/vulkan/vulkan_descriptor_pool.h"
#include "gpu/vulkan/vulkan_device_queue.h"

namespace gpu {

VulkanDescriptorSet::~VulkanDescriptorSet() {
  DCHECK_EQ(static_cast<VkDescriptorSet>(VK_NULL_HANDLE), handle_);
  descriptor_pool_->DecrementDescriptorSetCount();
}

bool VulkanDescriptorSet::Initialize(const VulkanDescriptorLayout* layout) {
  VkResult result = VK_SUCCESS;
  VkDevice device = device_queue_->GetVulkanDevice();

  VkDescriptorSetLayout layout_handle = layout->handle();

  VkDescriptorSetAllocateInfo set_allocate_info = {};
  set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  set_allocate_info.descriptorPool = descriptor_pool_->handle();
  set_allocate_info.descriptorSetCount = 1;
  set_allocate_info.pSetLayouts = &layout_handle;

  result = vkAllocateDescriptorSets(device, &set_allocate_info, &handle_);
  if (VK_SUCCESS != result) {
    DLOG(ERROR) << "vkAllocateDescriptorSets() failed: " << result;
    return false;
  }

  return true;
}

void VulkanDescriptorSet::Destroy() {
  VkDevice device = device_queue_->GetVulkanDevice();
  if (VK_NULL_HANDLE != handle_) {
    vkFreeDescriptorSets(device, descriptor_pool_->handle(), 1, &handle_);
    handle_ = VK_NULL_HANDLE;
  }
}

void VulkanDescriptorSet::WriteToDescriptorSet(
    uint32_t dst_binding,
    uint32_t dst_array_element,
    uint32_t descriptor_count,
    VkDescriptorType descriptor_type,
    const VkDescriptorImageInfo* image_info,
    const VkDescriptorBufferInfo* buffer_info,
    const VkBufferView* texel_buffer_view) {
  VkWriteDescriptorSet write_descriptor_set = {};
  write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write_descriptor_set.dstSet = handle_;
  write_descriptor_set.dstBinding = dst_binding;
  write_descriptor_set.dstArrayElement = dst_array_element;
  write_descriptor_set.descriptorCount = descriptor_count;
  write_descriptor_set.descriptorType = descriptor_type;
  write_descriptor_set.pImageInfo = image_info;
  write_descriptor_set.pBufferInfo = buffer_info;
  write_descriptor_set.pTexelBufferView = texel_buffer_view;

  vkUpdateDescriptorSets(device_queue_->GetVulkanDevice(), 1,
                         &write_descriptor_set, 0, nullptr);
}

void VulkanDescriptorSet::CopyFromDescriptorSet(
    const VulkanDescriptorSet* source_set,
    uint32_t src_binding,
    uint32_t src_array_element,
    uint32_t dst_binding,
    uint32_t dst_array_element,
    uint32_t descriptor_count) {
  VkCopyDescriptorSet copy_descriptor_set = {};
  copy_descriptor_set.sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET;
  copy_descriptor_set.srcSet = source_set->handle();
  copy_descriptor_set.srcBinding = src_binding;
  copy_descriptor_set.srcArrayElement = src_array_element;
  copy_descriptor_set.dstSet = handle_;
  copy_descriptor_set.dstBinding = dst_binding;
  copy_descriptor_set.dstArrayElement = dst_array_element;
  copy_descriptor_set.descriptorCount = descriptor_count;

  vkUpdateDescriptorSets(device_queue_->GetVulkanDevice(), 0, nullptr, 1,
                         &copy_descriptor_set);
}

VulkanDescriptorSet::VulkanDescriptorSet(VulkanDeviceQueue* device_queue,
                                         VulkanDescriptorPool* descriptor_pool)
    : device_queue_(device_queue), descriptor_pool_(descriptor_pool) {
  descriptor_pool_->IncrementDescriptorSetCount();
}

}  // namespace gpu
