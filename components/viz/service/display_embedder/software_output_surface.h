// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_SERVICE_DISPLAY_EMBEDDER_SOFTWARE_OUTPUT_SURFACE_H_
#define COMPONENTS_VIZ_SERVICE_DISPLAY_EMBEDDER_SOFTWARE_OUTPUT_SURFACE_H_

#include "base/memory/weak_ptr.h"
#include "components/viz/service/display/output_surface.h"
#include "components/viz/service/viz_service_export.h"
#include "ui/latency/latency_info.h"
#include "ui/latency/latency_tracker.h"

namespace viz {
class SoftwareOutputDevice;

class VIZ_SERVICE_EXPORT SoftwareOutputSurface : public OutputSurface {
 public:
  explicit SoftwareOutputSurface(
      std::unique_ptr<SoftwareOutputDevice> software_device);
  ~SoftwareOutputSurface() override;

  // OutputSurface implementation.
  void BindToClient(OutputSurfaceClient* client) override;
  void EnsureBackbuffer() override;
  void DiscardBackbuffer() override;
  void BindFramebuffer() override;
  void SetDrawRectangle(const gfx::Rect& draw_rectangle) override;
  void Reshape(const gfx::Size& size,
               float device_scale_factor,
               const gfx::ColorSpace& color_space,
               bool has_alpha,
               bool use_stencil) override;
  void SwapBuffers(OutputSurfaceFrame frame) override;
  bool IsDisplayedAsOverlayPlane() const override;
  OverlayCandidateValidator* GetOverlayCandidateValidator() const override;
  unsigned GetOverlayTextureId() const override;
  gfx::BufferFormat GetOverlayBufferFormat() const override;
  bool HasExternalStencilTest() const override;
  void ApplyExternalStencil() override;
  uint32_t GetFramebufferCopyTextureFormat() override;
#if BUILDFLAG(ENABLE_VULKAN)
  gpu::VulkanSurface* GetVulkanSurface() override;
#endif
  unsigned UpdateGpuFence() override;

 private:
  void SwapBuffersCallback(bool need_presentation_feedback);

  OutputSurfaceClient* client_ = nullptr;
  base::TimeDelta refresh_interval_;
  std::vector<ui::LatencyInfo> stored_latency_info_;
  ui::LatencyTracker latency_tracker_;
  base::WeakPtrFactory<SoftwareOutputSurface> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(SoftwareOutputSurface);
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_SERVICE_DISPLAY_EMBEDDER_SOFTWARE_OUTPUT_SURFACE_H_
