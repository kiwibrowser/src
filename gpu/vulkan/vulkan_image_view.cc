// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/vulkan/vulkan_image_view.h"

#include "base/logging.h"
#include "gpu/vulkan/vulkan_device_queue.h"

namespace gpu {

namespace {
const VkImageAspectFlags kAspectFlags[] = {
    // IMAGE_TYPE_COLOR,
    VK_IMAGE_ASPECT_COLOR_BIT,

    // IMAGE_TYPE_DEPTH,
    VK_IMAGE_ASPECT_DEPTH_BIT,

    // IMAGE_TYPE_STENCIL,
    VK_IMAGE_ASPECT_STENCIL_BIT,

    // IMAGE_TYPE_DEPTH_STENCIL,
    VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
};
static_assert(arraysize(kAspectFlags) == VulkanImageView::NUM_IMAGE_TYPES,
              "Array size for kAspectFlags must match image types.");
}  // namespace

VulkanImageView::VulkanImageView(VulkanDeviceQueue* device_queue)
    : device_queue_(device_queue) {}

VulkanImageView::~VulkanImageView() {
  DCHECK_EQ(static_cast<VkImageView>(VK_NULL_HANDLE), handle_);
  DCHECK_EQ(IMAGE_TYPE_INVALID, image_type_);
}

bool VulkanImageView::Initialize(VkImage image,
                                 VkImageViewType image_view_type,
                                 ImageType image_type,
                                 VkFormat format,
                                 uint32_t width,
                                 uint32_t height,
                                 uint32_t base_mip_level,
                                 uint32_t num_mips,
                                 uint32_t base_layer_level,
                                 uint32_t num_layers) {
  format_ = format;
  DCHECK_GT(image_type, IMAGE_TYPE_INVALID);
  DCHECK_LT(image_type, NUM_IMAGE_TYPES);
  VkImageSubresourceRange image_subresource_range = {};
  image_subresource_range.aspectMask = kAspectFlags[image_type];
  image_subresource_range.baseMipLevel = base_mip_level;
  image_subresource_range.levelCount = num_mips;
  image_subresource_range.baseArrayLayer = base_layer_level;
  image_subresource_range.layerCount = num_layers;

  VkImageViewCreateInfo image_view_create_info = {};
  image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  image_view_create_info.image = image;
  image_view_create_info.viewType = image_view_type;
  image_view_create_info.format = format;
  image_view_create_info.components = {
      VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
      VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
  image_view_create_info.subresourceRange = image_subresource_range;

  VkResult result =
      vkCreateImageView(device_queue_->GetVulkanDevice(),
                        &image_view_create_info, nullptr, &handle_);
  if (VK_SUCCESS != result) {
    DLOG(ERROR) << "vkCreateImageView() failed: " << result;
    return false;
  }

  image_type_ = image_type;
  width_ = width;
  height_ = height;
  mips_ = num_mips;
  layers_ = num_layers;
  return true;
}

void VulkanImageView::Destroy() {
  if (VK_NULL_HANDLE != handle_) {
    vkDestroyImageView(device_queue_->GetVulkanDevice(), handle_, nullptr);
    image_type_ = IMAGE_TYPE_INVALID;
    handle_ = VK_NULL_HANDLE;
  }
}

}  // namespace gpu
