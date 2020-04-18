// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_VULKAN_VULKAN_IMAGE_VIEW_H_
#define GPU_VULKAN_VULKAN_IMAGE_VIEW_H_

#include <vulkan/vulkan.h>

#include "base/macros.h"

namespace gpu {

class VulkanDeviceQueue;

class VulkanImageView {
 public:
  enum ImageType {
    IMAGE_TYPE_COLOR,
    IMAGE_TYPE_DEPTH,
    IMAGE_TYPE_STENCIL,
    IMAGE_TYPE_DEPTH_STENCIL,

    NUM_IMAGE_TYPES,
    IMAGE_TYPE_INVALID = -1,
  };

  explicit VulkanImageView(VulkanDeviceQueue* device_queue);
  ~VulkanImageView();

  bool Initialize(VkImage image,
                  VkImageViewType image_view_type,
                  ImageType image_type,
                  VkFormat format,
                  uint32_t width,
                  uint32_t height,
                  uint32_t base_mip_level,
                  uint32_t num_mips,
                  uint32_t base_layer_level,
                  uint32_t num_layers);
  void Destroy();

  ImageType image_type() const { return image_type_; }
  VkImageView handle() const { return handle_; }
  VkFormat format() const { return format_; }
  uint32_t width() const { return width_; }
  uint32_t height() const { return height_; }
  uint32_t mips() const { return mips_; }
  uint32_t layers() const { return layers_; }

 private:
  VulkanDeviceQueue* device_queue_ = nullptr;
  ImageType image_type_ = IMAGE_TYPE_INVALID;
  VkImageView handle_ = VK_NULL_HANDLE;
  VkFormat format_ = VK_FORMAT_UNDEFINED;
  uint32_t width_ = 0;
  uint32_t height_ = 0;
  uint32_t mips_ = 0;
  uint32_t layers_ = 0;

  DISALLOW_COPY_AND_ASSIGN(VulkanImageView);
};

}  // namespace gpu

#endif  // GPU_VULKAN_VULKAN_IMAGE_VIEW_H_
