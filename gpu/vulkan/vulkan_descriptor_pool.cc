// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/vulkan/vulkan_descriptor_pool.h"

#include "base/logging.h"
#include "gpu/vulkan/vulkan_descriptor_set.h"
#include "gpu/vulkan/vulkan_device_queue.h"

namespace gpu {

VulkanDescriptorPool::VulkanDescriptorPool(VulkanDeviceQueue* device_queue)
    : device_queue_(device_queue) {}

VulkanDescriptorPool::~VulkanDescriptorPool() {
  DCHECK_EQ(static_cast<VkDescriptorPool>(VK_NULL_HANDLE), handle_);
  DCHECK_EQ(0u, descriptor_count_);
}

bool VulkanDescriptorPool::Initialize(
    uint32_t max_descriptor_sets,
    const std::vector<VkDescriptorPoolSize>& pool_sizes) {
  DCHECK_EQ(static_cast<VkDescriptorPool>(VK_NULL_HANDLE), handle_);
  max_descriptor_sets_ = max_descriptor_sets;

  VkDescriptorPoolCreateInfo descriptor_pool_create_info = {};
  descriptor_pool_create_info.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  descriptor_pool_create_info.flags =
      VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  descriptor_pool_create_info.maxSets = max_descriptor_sets;
  descriptor_pool_create_info.poolSizeCount =
      static_cast<uint32_t>(pool_sizes.size());
  descriptor_pool_create_info.pPoolSizes = pool_sizes.data();

  VkResult result =
      vkCreateDescriptorPool(device_queue_->GetVulkanDevice(),
                             &descriptor_pool_create_info, nullptr, &handle_);
  if (VK_SUCCESS != result) {
    DLOG(ERROR) << "vkCreateDescriptorPool() failed: " << result;
    return false;
  }

  return true;
}

void VulkanDescriptorPool::Destroy() {
  DCHECK_EQ(0u, descriptor_count_);
  if (VK_NULL_HANDLE != handle_) {
    vkDestroyDescriptorPool(device_queue_->GetVulkanDevice(), handle_, nullptr);
    handle_ = VK_NULL_HANDLE;
  }

  max_descriptor_sets_ = 0;
}

std::unique_ptr<VulkanDescriptorSet> VulkanDescriptorPool::CreateDescriptorSet(
    const VulkanDescriptorLayout* layout) {
  std::unique_ptr<VulkanDescriptorSet> descriptor_set(
      new VulkanDescriptorSet(device_queue_, this));
  if (!descriptor_set->Initialize(layout)) {
    return nullptr;
  }
  return descriptor_set;
}

void VulkanDescriptorPool::IncrementDescriptorSetCount() {
  DCHECK_LT(descriptor_count_, max_descriptor_sets_);
  descriptor_count_++;
}

void VulkanDescriptorPool::DecrementDescriptorSetCount() {
  DCHECK_LT(0u, descriptor_count_);
  descriptor_count_--;
}

}  // namespace gpu
