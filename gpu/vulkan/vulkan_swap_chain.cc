// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/vulkan/vulkan_swap_chain.h"

#include "gpu/vulkan/vulkan_command_buffer.h"
#include "gpu/vulkan/vulkan_command_pool.h"
#include "gpu/vulkan/vulkan_device_queue.h"
#include "gpu/vulkan/vulkan_image_view.h"

namespace gpu {

VulkanSwapChain::VulkanSwapChain() {}

VulkanSwapChain::~VulkanSwapChain() {
  DCHECK(images_.empty());
  DCHECK_EQ(static_cast<VkSwapchainKHR>(VK_NULL_HANDLE), swap_chain_);
  DCHECK_EQ(static_cast<VkSemaphore>(VK_NULL_HANDLE), next_present_semaphore_);
}

bool VulkanSwapChain::Initialize(VulkanDeviceQueue* device_queue,
                                 VkSurfaceKHR surface,
                                 const VkSurfaceCapabilitiesKHR& surface_caps,
                                 const VkSurfaceFormatKHR& surface_format) {
  DCHECK(device_queue);
  device_queue_ = device_queue;
  return InitializeSwapChain(surface, surface_caps, surface_format) &&
         InitializeSwapImages(surface_caps, surface_format);
}

void VulkanSwapChain::Destroy() {
  DestroySwapImages();
  DestroySwapChain();
}

gfx::SwapResult VulkanSwapChain::SwapBuffers() {
  VkResult result = VK_SUCCESS;

  VkDevice device = device_queue_->GetVulkanDevice();
  VkQueue queue = device_queue_->GetVulkanQueue();

  std::unique_ptr<ImageData>& current_image_data = images_[current_image_];

  // Submit our command buffer for the current buffer.
  if (!current_image_data->command_buffer->Submit(
          1, &current_image_data->present_semaphore, 1,
          &current_image_data->render_semaphore)) {
    return gfx::SwapResult::SWAP_FAILED;
  }

  // Queue the present.
  VkPresentInfoKHR present_info = {};
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores = &current_image_data->render_semaphore;
  present_info.swapchainCount = 1;
  present_info.pSwapchains = &swap_chain_;
  present_info.pImageIndices = &current_image_;

  result = vkQueuePresentKHR(queue, &present_info);
  if (VK_SUCCESS != result) {
    return gfx::SwapResult::SWAP_FAILED;
  }

  // Acquire then next image.
  result = vkAcquireNextImageKHR(device, swap_chain_, UINT64_MAX,
                                 next_present_semaphore_, VK_NULL_HANDLE,
                                 &current_image_);
  if (VK_SUCCESS != result) {
    DLOG(ERROR) << "vkAcquireNextImageKHR() failed: " << result;
    return gfx::SwapResult::SWAP_FAILED;
  }

  // Swap in the "next_present_semaphore" into the newly acquired image. The
  // old "present_semaphore" for the image becomes the place holder for the next
  // present semaphore for the next image.
  std::swap(images_[current_image_]->present_semaphore,
            next_present_semaphore_);

  return gfx::SwapResult::SWAP_ACK;
}

bool VulkanSwapChain::InitializeSwapChain(
    VkSurfaceKHR surface,
    const VkSurfaceCapabilitiesKHR& surface_caps,
    const VkSurfaceFormatKHR& surface_format) {
  VkDevice device = device_queue_->GetVulkanDevice();
  VkResult result = VK_SUCCESS;

  VkSwapchainCreateInfoKHR swap_chain_create_info = {};
  swap_chain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swap_chain_create_info.surface = surface;
  swap_chain_create_info.minImageCount =
      std::max(2u, surface_caps.minImageCount);
  swap_chain_create_info.imageFormat = surface_format.format;
  swap_chain_create_info.imageColorSpace = surface_format.colorSpace;
  swap_chain_create_info.imageExtent = surface_caps.currentExtent;
  swap_chain_create_info.imageArrayLayers = 1;
  swap_chain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  swap_chain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  swap_chain_create_info.preTransform = surface_caps.currentTransform;
  swap_chain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  swap_chain_create_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
  swap_chain_create_info.clipped = true;
  swap_chain_create_info.oldSwapchain = swap_chain_;

  VkSwapchainKHR new_swap_chain = VK_NULL_HANDLE;
  result = vkCreateSwapchainKHR(device, &swap_chain_create_info, nullptr,
                                &new_swap_chain);
  if (VK_SUCCESS != result) {
    DLOG(ERROR) << "vkCreateSwapchainKHR() failed: " << result;
    return false;
  }

  Destroy();

  swap_chain_ = new_swap_chain;
  size_ = gfx::Size(swap_chain_create_info.imageExtent.width,
                    swap_chain_create_info.imageExtent.height);

  return true;
}

void VulkanSwapChain::DestroySwapChain() {
  VkDevice device = device_queue_->GetVulkanDevice();

  if (swap_chain_ != VK_NULL_HANDLE) {
    vkDestroySwapchainKHR(device, swap_chain_, nullptr);
    swap_chain_ = VK_NULL_HANDLE;
  }
}

bool VulkanSwapChain::InitializeSwapImages(
    const VkSurfaceCapabilitiesKHR& surface_caps,
    const VkSurfaceFormatKHR& surface_format) {
  VkDevice device = device_queue_->GetVulkanDevice();
  VkResult result = VK_SUCCESS;

  uint32_t image_count = 0;
  result = vkGetSwapchainImagesKHR(device, swap_chain_, &image_count, nullptr);
  if (VK_SUCCESS != result) {
    DLOG(ERROR) << "vkGetSwapchainImagesKHR(NULL) failed: " << result;
    return false;
  }

  std::vector<VkImage> images(image_count);
  result =
      vkGetSwapchainImagesKHR(device, swap_chain_, &image_count, images.data());
  if (VK_SUCCESS != result) {
    DLOG(ERROR) << "vkGetSwapchainImagesKHR(images) failed: " << result;
    return false;
  }

  // Generic semaphore creation structure.
  VkSemaphoreCreateInfo semaphore_create_info = {};
  semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  // Default image subresource range.
  VkImageSubresourceRange image_subresource_range = {};
  image_subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  image_subresource_range.baseMipLevel = 0;
  image_subresource_range.levelCount = 1;
  image_subresource_range.baseArrayLayer = 0;
  image_subresource_range.layerCount = 1;

  // The image memory barrier is used to setup the image layout.
  VkImageMemoryBarrier image_memory_barrier = {};
  image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  image_memory_barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  image_memory_barrier.subresourceRange = image_subresource_range;

  command_pool_ = device_queue_->CreateCommandPool();
  if (!command_pool_)
    return false;

  images_.resize(image_count);
  for (uint32_t i = 0; i < image_count; ++i) {
    images_[i].reset(new ImageData);
    std::unique_ptr<ImageData>& image_data = images_[i];
    image_data->image = images[i];

    // Setup semaphores.
    result = vkCreateSemaphore(device, &semaphore_create_info, nullptr,
                               &image_data->render_semaphore);
    if (VK_SUCCESS != result) {
      DLOG(ERROR) << "vkCreateSemaphore(render) failed: " << result;
      return false;
    }

    result = vkCreateSemaphore(device, &semaphore_create_info, nullptr,
                               &image_data->present_semaphore);
    if (VK_SUCCESS != result) {
      DLOG(ERROR) << "vkCreateSemaphore(present) failed: " << result;
      return false;
    }

    // Initialize the command buffer for this buffer data.
    image_data->command_buffer = command_pool_->CreatePrimaryCommandBuffer();

    // Setup the Image Layout as the first command that gets issued in each
    // command buffer.
    ScopedSingleUseCommandBufferRecorder recorder(*image_data->command_buffer);
    image_memory_barrier.image = images[i];
    vkCmdPipelineBarrier(recorder.handle(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0,
                         nullptr, 1, &image_memory_barrier);

    // Create the image view.
    image_data->image_view.reset(new VulkanImageView(device_queue_));
    if (!image_data->image_view->Initialize(
            images[i], VK_IMAGE_VIEW_TYPE_2D, VulkanImageView::IMAGE_TYPE_COLOR,
            surface_format.format, size_.width(), size_.height(), 0, 1, 0, 1)) {
      return false;
    }
  }

  result = vkCreateSemaphore(device, &semaphore_create_info, nullptr,
                             &next_present_semaphore_);
  if (VK_SUCCESS != result) {
    DLOG(ERROR) << "vkCreateSemaphore(next_present) failed: " << result;
    return false;
  }

  // Acquire the initial buffer.
  result = vkAcquireNextImageKHR(device, swap_chain_, UINT64_MAX,
                                 next_present_semaphore_, VK_NULL_HANDLE,
                                 &current_image_);
  if (VK_SUCCESS != result) {
    DLOG(ERROR) << "vkAcquireNextImageKHR() failed: " << result;
    return false;
  }

  std::swap(images_[current_image_]->present_semaphore,
            next_present_semaphore_);

  return true;
}

void VulkanSwapChain::DestroySwapImages() {
  VkDevice device = device_queue_->GetVulkanDevice();

  if (VK_NULL_HANDLE != next_present_semaphore_) {
    vkDestroySemaphore(device, next_present_semaphore_, nullptr);
    next_present_semaphore_ = VK_NULL_HANDLE;
  }

  for (const std::unique_ptr<ImageData>& image_data : images_) {
    if (image_data->command_buffer) {
      // Make sure command buffer is done processing.
      image_data->command_buffer->Wait(UINT64_MAX);

      image_data->command_buffer->Destroy();
      image_data->command_buffer.reset();
    }

    // Destroy Image View.
    if (image_data->image_view) {
      image_data->image_view->Destroy();
      image_data->image_view.reset();
    }

    // Destroy Semaphores.
    if (VK_NULL_HANDLE != image_data->present_semaphore) {
      vkDestroySemaphore(device, image_data->present_semaphore, nullptr);
      image_data->present_semaphore = VK_NULL_HANDLE;
    }

    if (VK_NULL_HANDLE != image_data->render_semaphore) {
      vkDestroySemaphore(device, image_data->render_semaphore, nullptr);
      image_data->render_semaphore = VK_NULL_HANDLE;
    }

    image_data->image = VK_NULL_HANDLE;
  }
  images_.clear();

  if (command_pool_) {
    command_pool_->Destroy();
    command_pool_.reset();
  }
}

VulkanSwapChain::ImageData::ImageData() {}

VulkanSwapChain::ImageData::~ImageData() {}

}  // namespace gpu
