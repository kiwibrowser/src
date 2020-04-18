// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_COMPOSITOR_SOFTWARE_BROWSER_COMPOSITOR_OUTPUT_SURFACE_H_
#define CONTENT_BROWSER_COMPOSITOR_SOFTWARE_BROWSER_COMPOSITOR_OUTPUT_SURFACE_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "build/build_config.h"
#include "content/browser/compositor/browser_compositor_output_surface.h"
#include "content/common/content_export.h"
#include "gpu/vulkan/buildflags.h"
#include "ui/latency/latency_tracker.h"

namespace content {

class CONTENT_EXPORT SoftwareBrowserCompositorOutputSurface
    : public BrowserCompositorOutputSurface {
 public:
  SoftwareBrowserCompositorOutputSurface(
      std::unique_ptr<viz::SoftwareOutputDevice> software_device,
      const UpdateVSyncParametersCallback& update_vsync_parameters_callback);

  ~SoftwareBrowserCompositorOutputSurface() override;

  // OutputSurface implementation.
  void BindToClient(viz::OutputSurfaceClient* client) override;
  void EnsureBackbuffer() override;
  void DiscardBackbuffer() override;
  void BindFramebuffer() override;
  void SetDrawRectangle(const gfx::Rect& draw_rectangle) override;
  void Reshape(const gfx::Size& size,
               float device_scale_factor,
               const gfx::ColorSpace& color_space,
               bool has_alpha,
               bool use_stencil) override;
  void SwapBuffers(viz::OutputSurfaceFrame frame) override;
  bool IsDisplayedAsOverlayPlane() const override;
  unsigned GetOverlayTextureId() const override;
  gfx::BufferFormat GetOverlayBufferFormat() const override;
  uint32_t GetFramebufferCopyTextureFormat() override;
#if BUILDFLAG(ENABLE_VULKAN)
  gpu::VulkanSurface* GetVulkanSurface() override;
#endif
  unsigned UpdateGpuFence() override;

 private:
  void SwapBuffersCallback(const std::vector<ui::LatencyInfo>& latency_info,
                           bool need_presentation_feedback);
  void UpdateVSyncCallback(const base::TimeTicks timebase,
                           const base::TimeDelta interval);

  viz::OutputSurfaceClient* client_ = nullptr;
  base::TimeDelta refresh_interval_;
  ui::LatencyTracker latency_tracker_;
  base::WeakPtrFactory<SoftwareBrowserCompositorOutputSurface> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(SoftwareBrowserCompositorOutputSurface);
};

}  // namespace content

#endif  // CONTENT_BROWSER_COMPOSITOR_SOFTWARE_BROWSER_COMPOSITOR_OUTPUT_SURFACE_H_
