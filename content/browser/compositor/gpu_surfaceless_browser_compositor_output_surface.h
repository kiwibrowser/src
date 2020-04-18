// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_COMPOSITOR_GPU_SURFACELESS_BROWSER_COMPOSITOR_OUTPUT_SURFACE_H_
#define CONTENT_BROWSER_COMPOSITOR_GPU_SURFACELESS_BROWSER_COMPOSITOR_OUTPUT_SURFACE_H_

#include <memory>

#include "content/browser/compositor/gpu_browser_compositor_output_surface.h"
#include "gpu/ipc/common/surface_handle.h"

namespace gpu {
class GpuMemoryBufferManager;
}

namespace viz {
class BufferQueue;
class GLHelper;
}

namespace content {

class GpuSurfacelessBrowserCompositorOutputSurface
    : public GpuBrowserCompositorOutputSurface {
 public:
  GpuSurfacelessBrowserCompositorOutputSurface(
      scoped_refptr<ui::ContextProviderCommandBuffer> context,
      gpu::SurfaceHandle surface_handle,
      const UpdateVSyncParametersCallback& update_vsync_parameters_callback,
      std::unique_ptr<viz::CompositorOverlayCandidateValidator>
          overlay_candidate_validator,
      unsigned int target,
      unsigned int internalformat,
      gfx::BufferFormat format,
      gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager);
  ~GpuSurfacelessBrowserCompositorOutputSurface() override;

  // viz::OutputSurface implementation.
  void SwapBuffers(viz::OutputSurfaceFrame frame) override;
  void BindFramebuffer() override;
  uint32_t GetFramebufferCopyTextureFormat() override;
  void Reshape(const gfx::Size& size,
               float device_scale_factor,
               const gfx::ColorSpace& color_space,
               bool has_alpha,
               bool use_stencil) override;
  bool IsDisplayedAsOverlayPlane() const override;
  unsigned GetOverlayTextureId() const override;
  gfx::BufferFormat GetOverlayBufferFormat() const override;
  unsigned UpdateGpuFence() override;

  // BrowserCompositorOutputSurface implementation.
  void OnGpuSwapBuffersCompleted(
      std::vector<ui::LatencyInfo> latency_info,
      const gpu::SwapBuffersCompleteParams& params) override;

 private:
  gfx::Size reshape_size_;
  gfx::Size swap_size_;
  bool use_gpu_fence_;
  unsigned gpu_fence_id_;

  std::unique_ptr<viz::GLHelper> gl_helper_;
  std::unique_ptr<viz::BufferQueue> buffer_queue_;
  gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_COMPOSITOR_GPU_SURFACELESS_BROWSER_COMPOSITOR_OUTPUT_SURFACE_H_
