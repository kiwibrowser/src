// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/compositor/software_browser_compositor_output_surface.h"

#include <utility>

#include "base/bind.h"
#include "base/location.h"
#include "base/memory/ref_counted.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "components/viz/service/display/output_surface_client.h"
#include "components/viz/service/display/output_surface_frame.h"
#include "components/viz/service/display/software_output_device.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "ui/gfx/vsync_provider.h"
#include "ui/latency/latency_info.h"

namespace content {

SoftwareBrowserCompositorOutputSurface::SoftwareBrowserCompositorOutputSurface(
    std::unique_ptr<viz::SoftwareOutputDevice> software_device,
    const UpdateVSyncParametersCallback& update_vsync_parameters_callback)
    : BrowserCompositorOutputSurface(std::move(software_device),
                                     update_vsync_parameters_callback),
      weak_factory_(this) {}

SoftwareBrowserCompositorOutputSurface::
    ~SoftwareBrowserCompositorOutputSurface() {
}

void SoftwareBrowserCompositorOutputSurface::BindToClient(
    viz::OutputSurfaceClient* client) {
  DCHECK(client);
  DCHECK(!client_);
  client_ = client;
}

void SoftwareBrowserCompositorOutputSurface::EnsureBackbuffer() {
  software_device()->EnsureBackbuffer();
}

void SoftwareBrowserCompositorOutputSurface::DiscardBackbuffer() {
  software_device()->DiscardBackbuffer();
}

void SoftwareBrowserCompositorOutputSurface::BindFramebuffer() {
  // Not used for software surfaces.
  NOTREACHED();
}

void SoftwareBrowserCompositorOutputSurface::SetDrawRectangle(
    const gfx::Rect& draw_rectangle) {
  NOTREACHED();
}

void SoftwareBrowserCompositorOutputSurface::Reshape(
    const gfx::Size& size,
    float device_scale_factor,
    const gfx::ColorSpace& color_space,
    bool has_alpha,
    bool use_stencil) {
  software_device()->Resize(size, device_scale_factor);
}

void SoftwareBrowserCompositorOutputSurface::SwapBuffers(
    viz::OutputSurfaceFrame frame) {
  DCHECK(client_);
  base::TimeTicks swap_time = base::TimeTicks::Now();
  for (auto& latency : frame.latency_info) {
    latency.AddLatencyNumberWithTimestamp(
        ui::INPUT_EVENT_GPU_SWAP_BUFFER_COMPONENT, 0, swap_time, 1);
    latency.AddLatencyNumberWithTimestamp(
        ui::INPUT_EVENT_LATENCY_TERMINATED_FRAME_SWAP_COMPONENT, 0, swap_time,
        1);
  }

  gfx::VSyncProvider* vsync_provider = software_device()->GetVSyncProvider();
  if (vsync_provider) {
    vsync_provider->GetVSyncParameters(
        base::Bind(&SoftwareBrowserCompositorOutputSurface::UpdateVSyncCallback,
                   weak_factory_.GetWeakPtr()));
  }

  software_device()->OnSwapBuffers(base::BindOnce(
      &SoftwareBrowserCompositorOutputSurface::SwapBuffersCallback,
      weak_factory_.GetWeakPtr(), frame.latency_info,
      frame.need_presentation_feedback));
}

void SoftwareBrowserCompositorOutputSurface::SwapBuffersCallback(
    const std::vector<ui::LatencyInfo>& latency_info,
    bool need_presentation_feedback) {
  RenderWidgetHostImpl::OnGpuSwapBuffersCompleted(latency_info);
  latency_tracker_.OnGpuSwapBuffersCompleted(latency_info);
  client_->DidReceiveSwapBuffersAck();
  if (need_presentation_feedback) {
    client_->DidReceivePresentationFeedback(gfx::PresentationFeedback(
        base::TimeTicks::Now(), refresh_interval_, 0u));
  }
}

void SoftwareBrowserCompositorOutputSurface::UpdateVSyncCallback(
    const base::TimeTicks timebase,
    const base::TimeDelta interval) {
  refresh_interval_ = interval;
  update_vsync_parameters_callback_.Run(timebase, interval);
}

bool SoftwareBrowserCompositorOutputSurface::IsDisplayedAsOverlayPlane() const {
  return false;
}

unsigned SoftwareBrowserCompositorOutputSurface::GetOverlayTextureId() const {
  return 0;
}

gfx::BufferFormat
SoftwareBrowserCompositorOutputSurface::GetOverlayBufferFormat() const {
  return gfx::BufferFormat::RGBX_8888;
}

uint32_t
SoftwareBrowserCompositorOutputSurface::GetFramebufferCopyTextureFormat() {
  // Not used for software surfaces.
  NOTREACHED();
  return 0;
}

#if BUILDFLAG(ENABLE_VULKAN)
gpu::VulkanSurface* SoftwareBrowserCompositorOutputSurface::GetVulkanSurface() {
  NOTIMPLEMENTED();
  return nullptr;
}
#endif

unsigned SoftwareBrowserCompositorOutputSurface::UpdateGpuFence() {
  return 0;
}

}  // namespace content
