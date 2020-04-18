// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/vulkan/vulkan_descriptor_layout.h"

#include "base/logging.h"
#include "gpu/vulkan/vulkan_descriptor_pool.h"
#include "gpu/vulkan/vulkan_device_queue.h"

namespace gpu {

VulkanDescriptorLayout::VulkanDescriptorLayout(VulkanDeviceQueue* device_queue)
    : device_queue_(device_queue) {}

VulkanDescriptorLayout::~VulkanDescriptorLayout() {
  DCHECK_EQ(static_cast<VkDescriptorSetLayout>(VK_NULL_HANDLE), handle_);
}

bool VulkanDescriptorLayout::Initialize(
    const std::vector<VkDescriptorSetLayoutBinding>& layout) {
  VkResult result = VK_SUCCESS;
  VkDevice device = device_queue_->GetVulkanDevice();

  VkDescriptorSetLayoutCreateInfo layout_create_info = {};
  layout_create_info.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layout_create_info.bindingCount = static_cast<uint32_t>(layout.size());
  layout_create_info.pBindings = layout.data();

  result = vkCreateDescriptorSetLayout(device, &layout_create_info, nullptr,
                                       &handle_);
  if (VK_SUCCESS != result) {
    DLOG(ERROR) << "vkCreateDescriptorSetLayout() failed: " << result;
    return false;
  }

  return true;
}

void VulkanDescriptorLayout::Destroy() {
  if (VK_NULL_HANDLE != handle_) {
    vkDestroyDescriptorSetLayout(
        device_queue_->GetVulkanDevice(), handle_, nullptr);
    handle_ = VK_NULL_HANDLE;
  }
}

}  // namespace gpu
