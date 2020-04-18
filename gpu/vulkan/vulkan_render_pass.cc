// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/vulkan/vulkan_render_pass.h"

#include "base/logging.h"
#include "gpu/vulkan/vulkan_command_buffer.h"
#include "gpu/vulkan/vulkan_device_queue.h"
#include "gpu/vulkan/vulkan_image_view.h"
#include "gpu/vulkan/vulkan_swap_chain.h"
#include "ui/gfx/geometry/size.h"

namespace gpu {

VkImageLayout ConvertImageLayout(
    const VulkanImageView* image_view,
    VulkanRenderPass::ImageLayoutType layout_type) {
  switch (layout_type) {
    case VulkanRenderPass::ImageLayoutType::IMAGE_LAYOUT_UNDEFINED:
      return VK_IMAGE_LAYOUT_UNDEFINED;
    case VulkanRenderPass::ImageLayoutType::IMAGE_LAYOUT_TYPE_IMAGE_VIEW:
      switch (image_view->image_type()) {
        case VulkanImageView::ImageType::IMAGE_TYPE_COLOR:
          return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        case VulkanImageView::ImageType::IMAGE_TYPE_DEPTH:
        case VulkanImageView::ImageType::IMAGE_TYPE_STENCIL:
        case VulkanImageView::ImageType::IMAGE_TYPE_DEPTH_STENCIL:
          return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        default:
          return VK_IMAGE_LAYOUT_UNDEFINED;
      }
      break;
    case VulkanRenderPass::ImageLayoutType::IMAGE_LAYOUT_TYPE_PRESENT:
      return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  }

  return VK_IMAGE_LAYOUT_UNDEFINED;
}

bool VulkanRenderPass::AttachmentData::ValidateData(
    const VulkanSwapChain* swap_chain) const {
#if DCHECK_IS_ON()
  if (attachment_type < AttachmentType::ATTACHMENT_TYPE_SWAP_IMAGE ||
      attachment_type > AttachmentType::ATTACHMENT_TYPE_ATTACHMENT_VIEW) {
    DLOG(ERROR) << "Invalid Attachment Type: "
                << static_cast<int>(attachment_type);
    return false;
  }

  if (sample_count &
      ~(VK_SAMPLE_COUNT_1_BIT | VK_SAMPLE_COUNT_2_BIT | VK_SAMPLE_COUNT_4_BIT |
        VK_SAMPLE_COUNT_8_BIT | VK_SAMPLE_COUNT_16_BIT |
        VK_SAMPLE_COUNT_32_BIT | VK_SAMPLE_COUNT_64_BIT)) {
    DLOG(ERROR) << "Invalid Sample Count: "
                << static_cast<uint32_t>(sample_count);
    return false;
  }

  if (std::min(load_op, stencil_load_op) < VK_ATTACHMENT_LOAD_OP_LOAD ||
      std::max(load_op, stencil_load_op) > VK_ATTACHMENT_LOAD_OP_DONT_CARE) {
    DLOG(ERROR) << "Invalid Load Op (" << static_cast<int>(load_op)
                << ") or Stencil Load Op (" << static_cast<int>(stencil_load_op)
                << ").";
    return false;
  }

  if (std::min(store_op, stencil_store_op) < VK_ATTACHMENT_STORE_OP_STORE ||
      std::max(store_op, stencil_store_op) > VK_ATTACHMENT_STORE_OP_DONT_CARE) {
    DLOG(ERROR) << "Invalid Store Op (" << static_cast<int>(store_op)
                << ") or Stencil Store Op ("
                << static_cast<int>(stencil_store_op) << ").";
    return false;
  }

  if (start_layout < ImageLayoutType::IMAGE_LAYOUT_UNDEFINED ||
      start_layout > ImageLayoutType::IMAGE_LAYOUT_TYPE_PRESENT) {
    DLOG(ERROR) << "Invalid Start Layout: " << static_cast<int>(start_layout);
    return false;
  }

  if (end_layout < ImageLayoutType::IMAGE_LAYOUT_TYPE_IMAGE_VIEW ||
      end_layout > ImageLayoutType::IMAGE_LAYOUT_TYPE_PRESENT) {
    DLOG(ERROR) << "Invalid Start Layout: " << static_cast<int>(end_layout);
    return false;
  }

  if (attachment_type == AttachmentType::ATTACHMENT_TYPE_ATTACHMENT_VIEW) {
    if (nullptr == image_view) {
      DLOG(ERROR) << "Must specify image view for image view attachment";
      return false;
    }
  }
#endif

  return true;
}

VulkanRenderPass::SubpassData::SubpassData() {}

VulkanRenderPass::SubpassData::SubpassData(const SubpassData& data) = default;

VulkanRenderPass::SubpassData::SubpassData(SubpassData&& data) = default;

VulkanRenderPass::SubpassData::~SubpassData() {}

bool VulkanRenderPass::SubpassData::ValidateData(
    uint32_t num_attachments) const {
#if DCHECK_IS_ON()
  for (const SubpassAttachment subpass_attachment : subpass_attachments) {
    if (subpass_attachment.attachment_index >= num_attachments) {
      DLOG(ERROR) << "Invalid attachment index: "
                  << subpass_attachment.attachment_index << " < "
                  << num_attachments;
      return false;
    }

    const ImageLayoutType layout = subpass_attachment.subpass_layout;
    if (layout < ImageLayoutType::IMAGE_LAYOUT_TYPE_IMAGE_VIEW ||
        layout > ImageLayoutType::IMAGE_LAYOUT_TYPE_PRESENT) {
      DLOG(ERROR) << "Invalid subpass layout: " << static_cast<int>(layout);
      return false;
    }
  }
#endif

  return true;
}

VulkanRenderPass::RenderPassData::RenderPassData() {}

VulkanRenderPass::RenderPassData::RenderPassData(const RenderPassData& data) =
    default;

VulkanRenderPass::RenderPassData::RenderPassData(RenderPassData&& data) =
    default;

VulkanRenderPass::RenderPassData::~RenderPassData() {}

VulkanRenderPass::VulkanRenderPass(VulkanDeviceQueue* device_queue)
    : device_queue_(device_queue) {}

VulkanRenderPass::~VulkanRenderPass() {
  DCHECK_EQ(static_cast<VkRenderPass>(VK_NULL_HANDLE), render_pass_);
  DCHECK(frame_buffers_.empty());
}

bool VulkanRenderPass::RenderPassData::ValidateData(
    const VulkanSwapChain* swap_chain) const {
#if DCHECK_IS_ON()
  for (const AttachmentData& attachment : attachments) {
    if (!attachment.ValidateData(swap_chain))
      return false;
  }

  for (const SubpassData& subpass_data : subpass_datas) {
    if (!subpass_data.ValidateData(attachments.size()))
      return false;
  }
#endif

  return true;
}

bool VulkanRenderPass::Initialize(const VulkanSwapChain* swap_chain,
                                  const RenderPassData& render_pass_data) {
  DCHECK(!executing_);
  DCHECK_EQ(static_cast<VkRenderPass>(VK_NULL_HANDLE), render_pass_);
  DCHECK(frame_buffers_.empty());
  DCHECK(render_pass_data.ValidateData(swap_chain));

  VkDevice device = device_queue_->GetVulkanDevice();
  VkResult result = VK_SUCCESS;

  swap_chain_ = swap_chain;
  num_sub_passes_ = render_pass_data.subpass_datas.size();
  current_sub_pass_ = 0;
  attachment_clear_values_.clear();
  attachment_clear_indexes_.clear();

  // Fill out attachment information.
  const uint32_t num_attachments = render_pass_data.attachments.size();
  std::vector<VkAttachmentDescription> attachment_descs(num_attachments);
  std::vector<VulkanImageView*> attachment_image_view(num_attachments);
  for (uint32_t i = 0; i < num_attachments; ++i) {
    const AttachmentData& attachment_data = render_pass_data.attachments[i];

    VulkanImageView* image_view = nullptr;
    switch (attachment_data.attachment_type) {
      case AttachmentType::ATTACHMENT_TYPE_SWAP_IMAGE:
        // All the image views in the swap chain all share the same format.
        image_view = swap_chain->GetCurrentImageView();
        break;
      case AttachmentType::ATTACHMENT_TYPE_ATTACHMENT_VIEW:
        // All the image views in the attachment should have the same format.
        image_view = attachment_data.image_view;
        break;
    }
    DCHECK(image_view);
    attachment_image_view[i] = image_view;

    VkAttachmentDescription& attachment_desc = attachment_descs[i];
    attachment_desc.format = image_view->format();
    attachment_desc.samples = attachment_data.sample_count;
    attachment_desc.loadOp = attachment_data.load_op;
    attachment_desc.storeOp = attachment_data.store_op;
    attachment_desc.stencilLoadOp = attachment_data.stencil_load_op;
    attachment_desc.stencilStoreOp = attachment_data.stencil_store_op;
    attachment_desc.initialLayout =
        ConvertImageLayout(image_view, attachment_data.start_layout);
    attachment_desc.finalLayout =
        ConvertImageLayout(image_view, attachment_data.end_layout);

    if (attachment_desc.loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR ||
        attachment_desc.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_CLEAR) {
      attachment_clear_values_.push_back(attachment_data.clear_value);
      attachment_clear_indexes_.push_back(i);
    }
  }

  // Fill out subpass information.
  std::vector<VkSubpassDescription> subpass_descs(
      render_pass_data.subpass_datas.size());
  std::vector<std::vector<VkAttachmentReference>> color_refs(
      render_pass_data.subpass_datas.size());
  std::vector<VkAttachmentReference> depth_stencil_refs(
      render_pass_data.subpass_datas.size());
  for (uint32_t i = 0; i < render_pass_data.subpass_datas.size(); ++i) {
    depth_stencil_refs[i].attachment = VK_ATTACHMENT_UNUSED;

    for (const VulkanRenderPass::SubpassAttachment& subpass_attachment :
         render_pass_data.subpass_datas[i].subpass_attachments) {
      const uint32_t index = subpass_attachment.attachment_index;
      VulkanImageView* image_view = attachment_image_view[index];

      VkAttachmentReference attachment_reference = {};
      attachment_reference.attachment = index;
      attachment_reference.layout =
          ConvertImageLayout(image_view, subpass_attachment.subpass_layout);

      switch (image_view->image_type()) {
        case VulkanImageView::ImageType::IMAGE_TYPE_COLOR:
          color_refs[i].push_back(attachment_reference);
          break;
        case VulkanImageView::ImageType::IMAGE_TYPE_DEPTH:
        case VulkanImageView::ImageType::IMAGE_TYPE_STENCIL:
        case VulkanImageView::ImageType::IMAGE_TYPE_DEPTH_STENCIL:
          if (VK_ATTACHMENT_UNUSED != depth_stencil_refs[i].attachment) {
            DLOG(ERROR) << "Subpass cannot have multiple depth/stencil refs.";
            return false;
          }
          depth_stencil_refs[i] = attachment_reference;
          break;
        default:
          DLOG(ERROR) << "Invalid image type: "
                      << static_cast<int>(image_view->image_type());
          return false;
      }
    }

    VkSubpassDescription& subpass_desc = subpass_descs[i];
    subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_desc.colorAttachmentCount = color_refs[i].size();
    subpass_desc.pColorAttachments = color_refs[i].data();
    subpass_desc.pDepthStencilAttachment = &depth_stencil_refs[i];
  }

  // Create VkRenderPass;
  VkRenderPassCreateInfo render_pass_create_info = {};
  render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  render_pass_create_info.attachmentCount =
      static_cast<uint32_t>(attachment_descs.size());
  render_pass_create_info.pAttachments = attachment_descs.data();
  render_pass_create_info.subpassCount = subpass_descs.size();
  render_pass_create_info.pSubpasses = subpass_descs.data();

  result = vkCreateRenderPass(device, &render_pass_create_info, nullptr,
                              &render_pass_);
  if (VK_SUCCESS != result) {
    DLOG(ERROR) << "vkCreateRenderPass() failed: " << result;
    return false;
  }

  // Initialize frame buffers.
  const uint32_t num_frame_buffers = swap_chain->num_images();
  frame_buffers_.resize(num_frame_buffers);
  for (uint32_t i = 0; i < num_frame_buffers; ++i) {
    std::vector<VkImageView> image_views(num_attachments);
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t layers = 0;

    for (uint32_t n = 0; n < num_attachments; ++n) {
      const AttachmentData& attachment_data = render_pass_data.attachments[n];
      VulkanImageView* image_view = nullptr;

      switch (attachment_data.attachment_type) {
        case AttachmentType::ATTACHMENT_TYPE_SWAP_IMAGE:
          image_view = swap_chain->GetImageView(i);
          break;
        case AttachmentType::ATTACHMENT_TYPE_ATTACHMENT_VIEW:
          image_view = attachment_data.image_view;
          break;
      }
      DCHECK(image_view);

      if (n == 0) {
        width = image_view->width();
        height = image_view->height();
        layers = image_view->layers();
      } else if (width != image_view->width() ||
                 height != image_view->height() ||
                 layers != image_view->layers()) {
        DLOG(ERROR) << "Images in a frame buffer must have same dimensions.";
        return false;
      }

      image_views[n] = image_view->handle();
    }

    VkFramebufferCreateInfo framebuffer_create_info = {};
    framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_create_info.renderPass = render_pass_;
    framebuffer_create_info.attachmentCount = image_views.size();
    framebuffer_create_info.pAttachments = image_views.data();
    framebuffer_create_info.width = width;
    framebuffer_create_info.height = height;
    framebuffer_create_info.layers = layers;

    result = vkCreateFramebuffer(device, &framebuffer_create_info, nullptr,
                                 &frame_buffers_[i]);
    if (VK_SUCCESS != result) {
      DLOG(ERROR) << "vkCreateFramebuffer() failed: " << result;
      return false;
    }
  }

  return true;
}

void VulkanRenderPass::Destroy() {
  VkDevice device = device_queue_->GetVulkanDevice();

  for (VkFramebuffer frame_buffer : frame_buffers_) {
    vkDestroyFramebuffer(device, frame_buffer, nullptr);
  }
  frame_buffers_.clear();

  if (VK_NULL_HANDLE != render_pass_) {
    vkDestroyRenderPass(device, render_pass_, nullptr);
    render_pass_ = VK_NULL_HANDLE;
  }

  swap_chain_ = nullptr;
  attachment_clear_values_.clear();
  attachment_clear_indexes_.clear();
}

void VulkanRenderPass::BeginRenderPass(
    const CommandBufferRecorderBase& recorder,
    bool exec_inline) {
  DCHECK(!executing_);
  DCHECK_NE(0u, num_sub_passes_);
  executing_ = true;
  execution_type_ = exec_inline ? VK_SUBPASS_CONTENTS_INLINE
                                : VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS;
  current_sub_pass_ = 0;

  const gfx::Size& size = swap_chain_->size();

  VkRenderPassBeginInfo begin_info = {};
  begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  begin_info.renderPass = render_pass_;
  begin_info.framebuffer = frame_buffers_[swap_chain_->current_image()];
  begin_info.renderArea.extent.width = size.width();
  begin_info.renderArea.extent.height = size.height();
  begin_info.clearValueCount =
      static_cast<uint32_t>(attachment_clear_values_.size());
  begin_info.pClearValues = attachment_clear_values_.data();

  vkCmdBeginRenderPass(recorder.handle(), &begin_info, execution_type_);
}

void VulkanRenderPass::NextSubPass(const CommandBufferRecorderBase& recorder) {
  DCHECK(executing_);
  DCHECK_LT(current_sub_pass_ + 1, num_sub_passes_);
  vkCmdNextSubpass(recorder.handle(), execution_type_);
  current_sub_pass_++;
}

void VulkanRenderPass::EndRenderPass(
    const CommandBufferRecorderBase& recorder) {
  DCHECK(executing_);
  vkCmdEndRenderPass(recorder.handle());
  executing_ = false;
}

void VulkanRenderPass::SetClearValue(uint32_t attachment_index,
                                     VkClearValue clear_value) {
  DCHECK_EQ(attachment_clear_values_.size(), attachment_clear_indexes_.size());
  auto iter =
      std::lower_bound(attachment_clear_indexes_.begin(),
                       attachment_clear_indexes_.end(), attachment_index);
  if (iter != attachment_clear_indexes_.end() && *iter == attachment_index) {
    const uint32_t index = iter - attachment_clear_indexes_.begin();
    attachment_clear_values_[index] = clear_value;
  }
}

}  // namespace gpu
