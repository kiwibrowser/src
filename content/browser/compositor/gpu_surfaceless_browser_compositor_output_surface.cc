// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/compositor/gpu_surfaceless_browser_compositor_output_surface.h"

#include <utility>

#include "components/viz/common/gl_helper.h"
#include "components/viz/service/display/output_surface_client.h"
#include "components/viz/service/display/output_surface_frame.h"
#include "components/viz/service/display_embedder/buffer_queue.h"
#include "components/viz/service/display_embedder/compositor_overlay_candidate_validator.h"
#include "content/browser/compositor/reflector_impl.h"
#include "gpu/GLES2/gl2extchromium.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "gpu/command_buffer/common/swap_buffers_complete_params.h"
#include "services/ui/public/cpp/gpu/context_provider_command_buffer.h"

namespace content {

GpuSurfacelessBrowserCompositorOutputSurface::
    GpuSurfacelessBrowserCompositorOutputSurface(
        scoped_refptr<ui::ContextProviderCommandBuffer> context,
        gpu::SurfaceHandle surface_handle,
        const UpdateVSyncParametersCallback& update_vsync_parameters_callback,
        std::unique_ptr<viz::CompositorOverlayCandidateValidator>
            overlay_candidate_validator,
        unsigned int target,
        unsigned int internalformat,
        gfx::BufferFormat format,
        gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager)
    : GpuBrowserCompositorOutputSurface(std::move(context),
                                        update_vsync_parameters_callback,
                                        std::move(overlay_candidate_validator)),
      use_gpu_fence_(
          context_provider_->ContextCapabilities().chromium_gpu_fence &&
          context_provider_->ContextCapabilities()
              .use_gpu_fences_for_overlay_planes),
      gpu_fence_id_(0),
      gpu_memory_buffer_manager_(gpu_memory_buffer_manager) {
  capabilities_.uses_default_gl_framebuffer = false;
  capabilities_.flipped_output_surface = true;
  // Set |max_frames_pending| to 2 for surfaceless, which aligns scheduling
  // more closely with the previous surfaced behavior.
  // With a surface, swap buffer ack used to return early, before actually
  // presenting the back buffer, enabling the browser compositor to run ahead.
  // Surfaceless implementation acks at the time of actual buffer swap, which
  // shifts the start of the new frame forward relative to the old
  // implementation.
  capabilities_.max_frames_pending = 2;

  gl_helper_.reset(new viz::GLHelper(context_provider_->ContextGL(),
                                     context_provider_->ContextSupport()));
  buffer_queue_.reset(new viz::BufferQueue(
      context_provider_->ContextGL(), target, internalformat, format,
      gl_helper_.get(), gpu_memory_buffer_manager_, surface_handle));
  buffer_queue_->Initialize();
}

GpuSurfacelessBrowserCompositorOutputSurface::
    ~GpuSurfacelessBrowserCompositorOutputSurface() {
  if (gpu_fence_id_ > 0)
    context_provider_->ContextGL()->DestroyGpuFenceCHROMIUM(gpu_fence_id_);
}

bool GpuSurfacelessBrowserCompositorOutputSurface::IsDisplayedAsOverlayPlane()
    const {
  return true;
}

unsigned GpuSurfacelessBrowserCompositorOutputSurface::GetOverlayTextureId()
    const {
  return buffer_queue_->GetCurrentTextureId();
}

gfx::BufferFormat
GpuSurfacelessBrowserCompositorOutputSurface::GetOverlayBufferFormat() const {
  DCHECK(buffer_queue_);
  return buffer_queue_->buffer_format();
}

void GpuSurfacelessBrowserCompositorOutputSurface::SwapBuffers(
    viz::OutputSurfaceFrame frame) {
  DCHECK(buffer_queue_);
  DCHECK(reshape_size_ == frame.size);
  // TODO(ccameron): What if a swap comes again before OnGpuSwapBuffersCompleted
  // happens, we'd see the wrong swap size there?
  swap_size_ = reshape_size_;

  gfx::Rect damage_rect =
      frame.sub_buffer_rect ? *frame.sub_buffer_rect : gfx::Rect(swap_size_);
  buffer_queue_->SwapBuffers(damage_rect);

  GpuBrowserCompositorOutputSurface::SwapBuffers(std::move(frame));
}

void GpuSurfacelessBrowserCompositorOutputSurface::BindFramebuffer() {
  DCHECK(buffer_queue_);
  buffer_queue_->BindFramebuffer();
}

GLenum GpuSurfacelessBrowserCompositorOutputSurface::
    GetFramebufferCopyTextureFormat() {
  return buffer_queue_->internal_format();
}

void GpuSurfacelessBrowserCompositorOutputSurface::Reshape(
    const gfx::Size& size,
    float device_scale_factor,
    const gfx::ColorSpace& color_space,
    bool has_alpha,
    bool use_stencil) {
  reshape_size_ = size;
  GpuBrowserCompositorOutputSurface::Reshape(
      size, device_scale_factor, color_space, has_alpha, use_stencil);
  DCHECK(buffer_queue_);
  buffer_queue_->Reshape(size, device_scale_factor, color_space, use_stencil);
}

void GpuSurfacelessBrowserCompositorOutputSurface::OnGpuSwapBuffersCompleted(
    std::vector<ui::LatencyInfo> latency_info,
    const gpu::SwapBuffersCompleteParams& params) {
  gpu::SwapBuffersCompleteParams modified_params(params);
  bool force_swap = false;
  if (params.swap_response.result ==
      gfx::SwapResult::SWAP_NAK_RECREATE_BUFFERS) {
    // Even through the swap failed, this is a fixable error so we can pretend
    // it succeeded to the rest of the system.
    modified_params.swap_response.result = gfx::SwapResult::SWAP_ACK;
    buffer_queue_->RecreateBuffers();
    force_swap = true;
  }
  buffer_queue_->PageFlipComplete();
  GpuBrowserCompositorOutputSurface::OnGpuSwapBuffersCompleted(
      std::move(latency_info), modified_params);
  if (force_swap)
    client_->SetNeedsRedrawRect(gfx::Rect(swap_size_));
}

unsigned GpuSurfacelessBrowserCompositorOutputSurface::UpdateGpuFence() {
  if (!use_gpu_fence_)
    return 0;

  if (gpu_fence_id_ > 0)
    context_provider_->ContextGL()->DestroyGpuFenceCHROMIUM(gpu_fence_id_);

  gpu_fence_id_ = context_provider_->ContextGL()->CreateGpuFenceCHROMIUM();

  return gpu_fence_id_;
}

}  // namespace content
