// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "gpu/vulkan/tests/basic_vulkan_test.h"
#include "gpu/vulkan/vulkan_command_buffer.h"
#include "gpu/vulkan/vulkan_render_pass.h"
#include "gpu/vulkan/vulkan_surface.h"
#include "gpu/vulkan/vulkan_swap_chain.h"

// This file tests basic vulkan initialization steps.

namespace gpu {

TEST_F(BasicVulkanTest, BasicVulkanSurface) {
  std::unique_ptr<VulkanSurface> surface = CreateViewSurface(window());
  EXPECT_TRUE(surface);
  EXPECT_TRUE(surface->Initialize(GetDeviceQueue(),
                                  VulkanSurface::DEFAULT_SURFACE_FORMAT));
  surface->Destroy();
}

TEST_F(BasicVulkanTest, EmptyVulkanSwaps) {
  std::unique_ptr<VulkanSurface> surface = CreateViewSurface(window());
  ASSERT_TRUE(surface);
  ASSERT_TRUE(surface->Initialize(GetDeviceQueue(),
                                  VulkanSurface::DEFAULT_SURFACE_FORMAT));

  // First swap is a special case, call it first to get better errors.
  EXPECT_EQ(gfx::SwapResult::SWAP_ACK, surface->SwapBuffers());

  // Also make sure we can swap multiple times.
  for (int i = 0; i < 10; ++i) {
    EXPECT_EQ(gfx::SwapResult::SWAP_ACK, surface->SwapBuffers());
  }
  surface->Finish();
  surface->Destroy();
}

TEST_F(BasicVulkanTest, BasicRenderPass) {
  std::unique_ptr<VulkanSurface> surface = CreateViewSurface(window());
  ASSERT_TRUE(surface);
  ASSERT_TRUE(surface->Initialize(GetDeviceQueue(),
                                  VulkanSurface::DEFAULT_SURFACE_FORMAT));
  VulkanSwapChain* swap_chain = surface->GetSwapChain();

  VulkanRenderPass::RenderPassData render_pass_data;

  // There is a single attachment which transitions present -> color -> present.
  render_pass_data.attachments.resize(1);
  VulkanRenderPass::AttachmentData* attachment =
      &render_pass_data.attachments[0];
  attachment->attachment_type =
      VulkanRenderPass::AttachmentType::ATTACHMENT_TYPE_SWAP_IMAGE;
  attachment->sample_count = VK_SAMPLE_COUNT_1_BIT;
  attachment->load_op = VK_ATTACHMENT_LOAD_OP_LOAD;
  attachment->store_op = VK_ATTACHMENT_STORE_OP_STORE;
  attachment->stencil_load_op = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachment->stencil_store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachment->start_layout =
      VulkanRenderPass::ImageLayoutType::IMAGE_LAYOUT_TYPE_PRESENT;
  attachment->end_layout =
      VulkanRenderPass::ImageLayoutType::IMAGE_LAYOUT_TYPE_PRESENT;

  // Single subpass.
  render_pass_data.subpass_datas.resize(1);
  VulkanRenderPass::SubpassData* subpass_data =
      &render_pass_data.subpass_datas[0];

  // Our subpass will handle the transition to Color.
  subpass_data->subpass_attachments.resize(1);
  VulkanRenderPass::SubpassAttachment* subpass_attachment =
      &subpass_data->subpass_attachments[0];
  subpass_attachment->attachment_index = 0;
  subpass_attachment->subpass_layout =
      VulkanRenderPass::ImageLayoutType::IMAGE_LAYOUT_TYPE_IMAGE_VIEW;

  ASSERT_TRUE(render_pass_data.ValidateData(swap_chain));

  VulkanRenderPass render_pass(GetDeviceQueue());
  EXPECT_TRUE(render_pass.Initialize(swap_chain, render_pass_data));

  for (int i = 0; i < 10; ++i) {
    VulkanCommandBuffer* command_buffer = swap_chain->GetCurrentCommandBuffer();
    {
      ScopedSingleUseCommandBufferRecorder recorder(*command_buffer);

      render_pass.BeginRenderPass(recorder, true);
      render_pass.EndRenderPass(recorder);
    }

    EXPECT_EQ(gfx::SwapResult::SWAP_ACK, surface->SwapBuffers());
  }

  surface->Finish();
  render_pass.Destroy();
  surface->Destroy();
}

}  // namespace gpu
