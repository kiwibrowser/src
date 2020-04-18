// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_VULKAN_VULKAN_RENDER_PASS_H_
#define GPU_VULKAN_VULKAN_RENDER_PASS_H_

#include <vector>
#include <vulkan/vulkan.h>

#include "base/macros.h"
#include "gpu/vulkan/vulkan_export.h"

namespace gpu {

class CommandBufferRecorderBase;
class VulkanDeviceQueue;
class VulkanImageView;
class VulkanSwapChain;

class VULKAN_EXPORT VulkanRenderPass {
 public:
  enum class AttachmentType {
    // Use image view of the swap chain image.
    ATTACHMENT_TYPE_SWAP_IMAGE,

    // Use image view of the attachment data.
    ATTACHMENT_TYPE_ATTACHMENT_VIEW,
  };

  enum class ImageLayoutType {
    // Undefined image layout.
    IMAGE_LAYOUT_UNDEFINED,

    // Image layout whiches matches the image view.
    IMAGE_LAYOUT_TYPE_IMAGE_VIEW,

    // Image layout for presenting.
    IMAGE_LAYOUT_TYPE_PRESENT,
  };

  struct AttachmentData {
    AttachmentType attachment_type;
    VkSampleCountFlagBits sample_count;
    VkAttachmentLoadOp load_op;
    VkAttachmentStoreOp store_op;
    // The stencil ops are only used for IMAGE_TYPE_STENCIL and
    // IMAGE_TYPE_DEPTH_STENCIL image views.
    VkAttachmentLoadOp stencil_load_op;
    VkAttachmentStoreOp stencil_store_op;
    ImageLayoutType start_layout;
    ImageLayoutType end_layout;
    VulkanImageView* image_view;  // used for ATTACHMENT_TYPE_ATTACHMENT_VIEW.
    VkClearValue clear_value;     // used for VK_ATTACHMENT_LOAD_OP_CLEAR.

    bool ValidateData(const VulkanSwapChain* swap_chain) const;
  };

  struct SubpassAttachment {
    uint32_t attachment_index;
    ImageLayoutType subpass_layout;
  };

  struct SubpassData {
    SubpassData();
    SubpassData(const SubpassData& data);
    SubpassData(SubpassData&& data);
    ~SubpassData();

    std::vector<SubpassAttachment> subpass_attachments;

    bool ValidateData(uint32_t num_attachments) const;
  };

  struct RenderPassData {
    RenderPassData();
    RenderPassData(const RenderPassData& data);
    RenderPassData(RenderPassData&& data);
    ~RenderPassData();

    std::vector<AttachmentData> attachments;
    std::vector<SubpassData> subpass_datas;

    bool ValidateData(const VulkanSwapChain* swap_chain) const;
  };

  explicit VulkanRenderPass(VulkanDeviceQueue* device_queue);
  ~VulkanRenderPass();

  bool Initialize(const VulkanSwapChain* swap_chain,
                  const RenderPassData& render_pass_data);
  void Destroy();

  // Begins render pass to command_buffer. The variable exec_inline signifies
  // whether or not the subpass commands will be executed inline (within a
  // primary command buffer) or whether it will be executed through a secondary
  // command buffer.
  void BeginRenderPass(const CommandBufferRecorderBase& recorder,
                       bool exec_inline);

  // Begins the next subpass after BeginRenderPass has been called.
  void NextSubPass(const CommandBufferRecorderBase& recorder);

  // Ends the render passes.
  void EndRenderPass(const CommandBufferRecorderBase& recorder);

  void SetClearValue(uint32_t attachment_index, VkClearValue clear_value);

 private:
  VulkanDeviceQueue* device_queue_ = nullptr;
  const VulkanSwapChain* swap_chain_ = nullptr;
  uint32_t num_sub_passes_ = 0;
  uint32_t current_sub_pass_ = 0;
  bool executing_ = false;
  VkSubpassContents execution_type_ = VK_SUBPASS_CONTENTS_INLINE;
  VkRenderPass render_pass_ = VK_NULL_HANDLE;

  // There is 1 clear color for every attachment which needs a clear.
  std::vector<VkClearValue> attachment_clear_values_;

  // There is 1 clear index for every attachment which needs a clear. This is
  // kept in a separate array since it is only used setting clear values.
  std::vector<uint32_t> attachment_clear_indexes_;

  // There is 1 frame buffer for every swap chain image.
  std::vector<VkFramebuffer> frame_buffers_;

  DISALLOW_COPY_AND_ASSIGN(VulkanRenderPass);
};

}  // namespace gpu

#endif  // GPU_VULKAN_VULKAN_RENDER_PASS_H_
