// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_SERVICE_DISPLAY_VULKAN_RENDERER_H_
#define COMPONENTS_VIZ_SERVICE_DISPLAY_VULKAN_RENDERER_H_

#include "components/viz/service/display/direct_renderer.h"
#include "components/viz/service/viz_service_export.h"
#include "ui/latency/latency_info.h"

namespace viz {

class OutputSurface;

class VIZ_SERVICE_EXPORT VulkanRenderer : public DirectRenderer {
 public:
  VulkanRenderer(const RendererSettings* settings,
                 OutputSurface* output_surface,
                 DisplayResourceProvider* resource_provider);
  ~VulkanRenderer() override;

  // Implementation of public DirectRenderer functions.
  void SwapBuffers(std::vector<ui::LatencyInfo> latency_info) override;

 protected:
  // Implementations of protected Renderer functions.
  void DidChangeVisibility() override;
  ResourceFormat BackbufferFormat() const override;

  // Implementations of protected DirectRenderer functions.
  void BindFramebufferToOutputSurface() override;
  void BindFramebufferToTexture(const RenderPassId render_pass_id) override;
  void SetScissorTestRect(const gfx::Rect& scissor_rect) override;
  void PrepareSurfaceForPass(SurfaceInitializationMode initialization_mode,
                             const gfx::Rect& render_pass_scissor) override;
  void DoDrawQuad(const DrawQuad* quad, const gfx::QuadF* clip_region) override;
  void BeginDrawingFrame() override;
  void FinishDrawingFrame() override;
  void FinishDrawingQuadList() override;
  bool FlippedFramebuffer() const override;
  void EnsureScissorTestEnabled() override;
  void EnsureScissorTestDisabled() override;
  void CopyDrawnRenderPass(std::unique_ptr<CopyOutputRequest> request) override;
  bool CanPartialSwap() override;
  void SetEnableDCLayers(bool enable) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(VulkanRenderer);
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_SERVICE_DISPLAY_VULKAN_RENDERER_H_
