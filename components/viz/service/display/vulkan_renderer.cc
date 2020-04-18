// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/display/vulkan_renderer.h"

#include "components/viz/service/display/output_surface.h"
#include "components/viz/service/display/output_surface_frame.h"

namespace viz {

VulkanRenderer::~VulkanRenderer() {}

void VulkanRenderer::SwapBuffers(std::vector<ui::LatencyInfo> latency_info) {
  OutputSurfaceFrame output_frame;
  output_frame.latency_info = std::move(latency_info);
  output_surface_->SwapBuffers(std::move(output_frame));
}

VulkanRenderer::VulkanRenderer(const RendererSettings* settings,
                               OutputSurface* output_surface,
                               DisplayResourceProvider* resource_provider)
    : DirectRenderer(settings, output_surface, resource_provider) {}

void VulkanRenderer::DidChangeVisibility() {
  NOTIMPLEMENTED();
}

void VulkanRenderer::BindFramebufferToOutputSurface() {
  NOTIMPLEMENTED();
}

ResourceFormat VulkanRenderer::BackbufferFormat() const {
  NOTIMPLEMENTED();
  return RGBA_8888;
}

void VulkanRenderer::BindFramebufferToTexture(
    const RenderPassId render_pass_id) {
  NOTIMPLEMENTED();
  return false;
}

void VulkanRenderer::SetScissorTestRect(const gfx::Rect& scissor_rect) {
  NOTIMPLEMENTED();
}

void VulkanRenderer::PrepareSurfaceForPass(
    SurfaceInitializationMode initialization_mode,
    const gfx::Rect& render_pass_scissor) {
  NOTIMPLEMENTED();
}

void VulkanRenderer::SetEnableDCLayers(bool enable) {
  NOTIMPLEMENTED();
}

void VulkanRenderer::DoDrawQuad(const DrawQuad* quad,
                                const gfx::QuadF* clip_region) {
  NOTIMPLEMENTED();
}

void VulkanRenderer::BeginDrawingFrame() {
  NOTIMPLEMENTED();
}

void VulkanRenderer::FinishDrawingFrame() {
  NOTIMPLEMENTED();
}

void VulkanRenderer::FinishDrawingQuadList() {
  NOTIMPLEMENTED();
}

bool VulkanRenderer::FlippedFramebuffer() const {
  NOTIMPLEMENTED();
  return false;
}

void VulkanRenderer::EnsureScissorTestEnabled() {
  NOTIMPLEMENTED();
}

void VulkanRenderer::EnsureScissorTestDisabled() {
  NOTIMPLEMENTED();
}

void VulkanRenderer::CopyDrawnRenderPass(
    std::unique_ptr<CopyOutputRequest> request) {
  NOTIMPLEMENTED();
}

bool VulkanRenderer::CanPartialSwap() {
  NOTIMPLEMENTED();
  return false;
}

}  // namespace viz
