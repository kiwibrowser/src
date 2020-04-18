// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_VULKAN_VULKAN_SAMPLER_H_
#define GPU_VULKAN_VULKAN_SAMPLER_H_

#include <float.h>
#include <vulkan/vulkan.h>

#include "base/macros.h"
#include "gpu/vulkan/vulkan_export.h"

namespace gpu {

class VulkanDeviceQueue;

class VULKAN_EXPORT VulkanSampler {
 public:
  struct SamplerOptions {
    SamplerOptions();
    ~SamplerOptions();

    VkFilter mag_filter = VK_FILTER_NEAREST;
    VkFilter min_filter = VK_FILTER_NEAREST;
    VkSamplerMipmapMode mipmap_mode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    VkSamplerAddressMode address_mode_u = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    VkSamplerAddressMode address_mode_v = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    VkSamplerAddressMode address_mode_w = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    float mip_lod_bias = 0.0f;
    bool anisotropy_enable = false;
    float max_anisotropy = 1.0f;
    bool compare_enable = false;
    VkCompareOp compare_op = VK_COMPARE_OP_NEVER;
    float min_lod = 0.0f;
    float max_lod = FLT_MAX;
    VkBorderColor border_color = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    bool unnormalized_coordinates = false;
  };

  explicit VulkanSampler(VulkanDeviceQueue* device_queue);
  ~VulkanSampler();

  bool Initialize(const SamplerOptions& options);
  void Destroy();

  VkSampler handle() const { return handle_; }

 private:
  VulkanDeviceQueue* device_queue_ = nullptr;
  VkSampler handle_ = VK_NULL_HANDLE;

  DISALLOW_COPY_AND_ASSIGN(VulkanSampler);
};

}  // namespace gpu

#endif  // GPU_VULKAN_VULKAN_SAMPLER_H_
