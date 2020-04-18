// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/demo/vulkan_renderer.h"

#include <vulkan/vulkan.h>

#include "base/location.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/trace_event/trace_event.h"
#include "gpu/vulkan/init/vulkan_factory.h"
#include "gpu/vulkan/vulkan_command_buffer.h"
#include "gpu/vulkan/vulkan_device_queue.h"
#include "gpu/vulkan/vulkan_implementation.h"
#include "gpu/vulkan/vulkan_render_pass.h"
#include "gpu/vulkan/vulkan_surface.h"
#include "gpu/vulkan/vulkan_swap_chain.h"

namespace ui {

VulkanRenderer::VulkanRenderer(gpu::VulkanImplementation* vulkan_implementation,
                               gfx::AcceleratedWidget widget,
                               const gfx::Size& size)
    : RendererBase(widget, size),
      vulkan_implementation_(vulkan_implementation),
      weak_ptr_factory_(this) {}

VulkanRenderer::~VulkanRenderer() {
  surface_->Finish();
  surface_->Destroy();
  surface_.reset();
  device_queue_->Destroy();
  device_queue_.reset();
}

bool VulkanRenderer::Initialize() {
  device_queue_ = gpu::CreateVulkanDeviceQueue(
      vulkan_implementation_,
      gpu::VulkanDeviceQueue::GRAPHICS_QUEUE_FLAG |
          gpu::VulkanDeviceQueue::PRESENTATION_SUPPORT_QUEUE_FLAG);
  CHECK(device_queue_);

  surface_ = vulkan_implementation_->CreateViewSurface(widget_);
  CHECK(surface_->Initialize(device_queue_.get(),
                             gpu::VulkanSurface::DEFAULT_SURFACE_FORMAT));

  gpu::VulkanRenderPass::RenderPassData render_pass_data;

  render_pass_data.attachments.resize(1);
  gpu::VulkanRenderPass::AttachmentData* attachment =
      &render_pass_data.attachments[0];
  attachment->attachment_type =
      gpu::VulkanRenderPass::AttachmentType::ATTACHMENT_TYPE_SWAP_IMAGE;
  attachment->sample_count = VK_SAMPLE_COUNT_1_BIT;
  attachment->load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachment->store_op = VK_ATTACHMENT_STORE_OP_STORE;
  attachment->stencil_load_op = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachment->stencil_store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachment->start_layout =
      gpu::VulkanRenderPass::ImageLayoutType::IMAGE_LAYOUT_UNDEFINED;
  attachment->end_layout =
      gpu::VulkanRenderPass::ImageLayoutType::IMAGE_LAYOUT_TYPE_PRESENT;

  render_pass_data.subpass_datas.resize(1);
  gpu::VulkanRenderPass::SubpassData* subpass_data =
      &render_pass_data.subpass_datas[0];

  subpass_data->subpass_attachments.resize(1);
  gpu::VulkanRenderPass::SubpassAttachment* subpass_attachment =
      &subpass_data->subpass_attachments[0];
  subpass_attachment->attachment_index = 0;
  subpass_attachment->subpass_layout =
      gpu::VulkanRenderPass::ImageLayoutType::IMAGE_LAYOUT_TYPE_IMAGE_VIEW;

  gpu::VulkanSwapChain* swap_chain = surface_->GetSwapChain();
  CHECK(render_pass_data.ValidateData(swap_chain));

  render_pass_ = std::make_unique<gpu::VulkanRenderPass>(device_queue_.get());
  CHECK(render_pass_->Initialize(swap_chain, render_pass_data));

  // Schedule the initial render.
  PostRenderFrameTask();
  return true;
}

void VulkanRenderer::RenderFrame() {
  TRACE_EVENT0("ozone", "VulkanRenderer::RenderFrame");
  VkClearValue clear_value = {.color = {{.5f, 1.f - NextFraction(), .5f, 1.f}}};

  render_pass_->SetClearValue(0, clear_value);

  gpu::VulkanCommandBuffer* command_buffer =
      surface_->GetSwapChain()->GetCurrentCommandBuffer();
  {
    gpu::ScopedSingleUseCommandBufferRecorder recorder(*command_buffer);
    render_pass_->BeginRenderPass(recorder, true);
    render_pass_->EndRenderPass(recorder);
  }

  CHECK_EQ(surface_->SwapBuffers(), gfx::SwapResult::SWAP_ACK);

  // TODO(spang): Use a better synchronization strategy.
  command_buffer->Wait(UINT64_MAX);

  PostRenderFrameTask();
}

void VulkanRenderer::PostRenderFrameTask() {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&VulkanRenderer::RenderFrame,
                                weak_ptr_factory_.GetWeakPtr()));
}

}  // namespace ui
