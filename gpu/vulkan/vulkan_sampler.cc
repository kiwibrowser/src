// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/vulkan/vulkan_sampler.h"

#include "base/logging.h"
#include "gpu/vulkan/vulkan_device_queue.h"

namespace gpu {

VulkanSampler::SamplerOptions::SamplerOptions() = default;
VulkanSampler::SamplerOptions::~SamplerOptions() = default;

VulkanSampler::VulkanSampler(VulkanDeviceQueue* device_queue)
    : device_queue_(device_queue) {}

VulkanSampler::~VulkanSampler() {
  DCHECK_EQ(static_cast<VkSampler>(VK_NULL_HANDLE), handle_);
}

bool VulkanSampler::Initialize(const SamplerOptions& options) {
  DCHECK_EQ(static_cast<VkSampler>(VK_NULL_HANDLE), handle_);

  VkSamplerCreateInfo sampler_create_info = {};
  sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  sampler_create_info.magFilter = options.mag_filter;
  sampler_create_info.minFilter = options.min_filter;
  sampler_create_info.mipmapMode = options.mipmap_mode;
  sampler_create_info.addressModeU = options.address_mode_u;
  sampler_create_info.addressModeV = options.address_mode_v;
  sampler_create_info.addressModeW = options.address_mode_w;
  sampler_create_info.mipLodBias = options.mip_lod_bias;
  sampler_create_info.anisotropyEnable = options.anisotropy_enable;
  sampler_create_info.maxAnisotropy = options.max_anisotropy;
  sampler_create_info.compareOp = options.compare_op;
  sampler_create_info.minLod = options.min_lod;
  sampler_create_info.maxLod = options.max_lod;
  sampler_create_info.borderColor = options.border_color;
  sampler_create_info.unnormalizedCoordinates =
      options.unnormalized_coordinates;

  VkResult result = vkCreateSampler(device_queue_->GetVulkanDevice(),
                                    &sampler_create_info, nullptr, &handle_);
  if (VK_SUCCESS != result) {
    DLOG(ERROR) << "vkCreateSampler() failed: " << result;
    return false;
  }

  return true;
}

void VulkanSampler::Destroy() {
  if (VK_NULL_HANDLE != handle_) {
    vkDestroySampler(device_queue_->GetVulkanDevice(), handle_, nullptr);
    handle_ = VK_NULL_HANDLE;
  }
}

}  // namespace gpu
