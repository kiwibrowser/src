// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/ipc/service/pass_through_image_transport_surface.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/command_line.h"
#include "build/build_config.h"
#include "gpu/command_buffer/common/swap_buffers_complete_params.h"
#include "ui/gfx/vsync_provider.h"
#include "ui/gl/gl_context.h"
#include "ui/gl/gl_switches.h"

namespace gpu {

namespace {
// Number of swap generations before vsync is reenabled after we've stopped
// doing multiple swaps per frame.
const int kMultiWindowSwapEnableVSyncDelay = 60;

int g_current_swap_generation_ = 0;
int g_num_swaps_in_current_swap_generation_ = 0;
int g_last_multi_window_swap_generation_ = 0;

bool HasSwitch(const char switch_constant[]) {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(switch_constant);
}

}  // anonymous namespace

PassThroughImageTransportSurface::PassThroughImageTransportSurface(
    base::WeakPtr<ImageTransportSurfaceDelegate> delegate,
    gl::GLSurface* surface,
    bool override_vsync_for_multi_window_swap)
    : GLSurfaceAdapter(surface),
      is_gpu_vsync_disabled_(HasSwitch(switches::kDisableGpuVsync)),
      is_multi_window_swap_vsync_override_enabled_(
          override_vsync_for_multi_window_swap),
      delegate_(delegate),
      weak_ptr_factory_(this) {}

PassThroughImageTransportSurface::~PassThroughImageTransportSurface() {
  if (delegate_)
    delegate_->SetSnapshotRequestedCallback(base::Closure());
}

bool PassThroughImageTransportSurface::Initialize(gl::GLSurfaceFormat format) {
  DCHECK(gl::GLSurfaceAdapter::SupportsPresentationCallback());
  // The surface is assumed to have already been initialized.
  delegate_->SetSnapshotRequestedCallback(
      base::Bind(&PassThroughImageTransportSurface::SetSnapshotRequested,
                 base::Unretained(this)));
  return true;
}

gfx::SwapResult PassThroughImageTransportSurface::SwapBuffers(
    const PresentationCallback& callback) {
  gfx::SwapResponse response;
  StartSwapBuffers(&response);
  gfx::SwapResult result = gl::GLSurfaceAdapter::SwapBuffers(
      base::Bind(&PassThroughImageTransportSurface::BufferPresented,
                 weak_ptr_factory_.GetWeakPtr(), callback));
  response.result = result;
  FinishSwapBuffers(GetAndResetSnapshotRequested(), std::move(response));
  return result;
}

void PassThroughImageTransportSurface::SwapBuffersAsync(
    const SwapCompletionCallback& completion_callback,
    const PresentationCallback& presentation_callback) {
  gfx::SwapResponse response;
  StartSwapBuffers(&response);

  // We use WeakPtr here to avoid manual management of life time of an instance
  // of this class. Callback will not be called once the instance of this class
  // is destroyed. However, this also means that the callback can be run on
  // the calling thread only.
  gl::GLSurfaceAdapter::SwapBuffersAsync(
      base::Bind(&PassThroughImageTransportSurface::FinishSwapBuffersAsync,
                 weak_ptr_factory_.GetWeakPtr(), completion_callback,
                 GetAndResetSnapshotRequested(), base::Passed(&response)),
      base::Bind(&PassThroughImageTransportSurface::BufferPresented,
                 weak_ptr_factory_.GetWeakPtr(), presentation_callback));
}

gfx::SwapResult PassThroughImageTransportSurface::SwapBuffersWithBounds(
    const std::vector<gfx::Rect>& rects,
    const PresentationCallback& callback) {
  gfx::SwapResponse response;
  StartSwapBuffers(&response);
  gfx::SwapResult result = gl::GLSurfaceAdapter::SwapBuffersWithBounds(
      rects, base::Bind(&PassThroughImageTransportSurface::BufferPresented,
                        weak_ptr_factory_.GetWeakPtr(), callback));
  response.result = result;
  FinishSwapBuffers(GetAndResetSnapshotRequested(), std::move(response));
  return result;
}

gfx::SwapResult PassThroughImageTransportSurface::PostSubBuffer(
    int x,
    int y,
    int width,
    int height,
    const PresentationCallback& callback) {
  gfx::SwapResponse response;
  StartSwapBuffers(&response);
  gfx::SwapResult result = gl::GLSurfaceAdapter::PostSubBuffer(
      x, y, width, height,
      base::Bind(&PassThroughImageTransportSurface::BufferPresented,
                 weak_ptr_factory_.GetWeakPtr(), callback));
  response.result = result;
  FinishSwapBuffers(GetAndResetSnapshotRequested(), std::move(response));

  return result;
}

void PassThroughImageTransportSurface::PostSubBufferAsync(
    int x,
    int y,
    int width,
    int height,
    const GLSurface::SwapCompletionCallback& completion_callback,
    const PresentationCallback& presentation_callback) {
  gfx::SwapResponse response;
  StartSwapBuffers(&response);
  gl::GLSurfaceAdapter::PostSubBufferAsync(
      x, y, width, height,
      base::Bind(&PassThroughImageTransportSurface::FinishSwapBuffersAsync,
                 weak_ptr_factory_.GetWeakPtr(), completion_callback,
                 GetAndResetSnapshotRequested(), base::Passed(&response)),
      base::Bind(&PassThroughImageTransportSurface::BufferPresented,
                 weak_ptr_factory_.GetWeakPtr(), presentation_callback));
}

gfx::SwapResult PassThroughImageTransportSurface::CommitOverlayPlanes(
    const PresentationCallback& callback) {
  gfx::SwapResponse response;
  StartSwapBuffers(&response);
  gfx::SwapResult result = gl::GLSurfaceAdapter::CommitOverlayPlanes(
      base::Bind(&PassThroughImageTransportSurface::BufferPresented,
                 weak_ptr_factory_.GetWeakPtr(), callback));
  response.result = result;
  FinishSwapBuffers(GetAndResetSnapshotRequested(), std::move(response));
  return result;
}

void PassThroughImageTransportSurface::CommitOverlayPlanesAsync(
    const GLSurface::SwapCompletionCallback& callback,
    const PresentationCallback& presentation_callback) {
  gfx::SwapResponse response;
  StartSwapBuffers(&response);
  gl::GLSurfaceAdapter::CommitOverlayPlanesAsync(
      base::Bind(&PassThroughImageTransportSurface::FinishSwapBuffersAsync,
                 weak_ptr_factory_.GetWeakPtr(), callback,
                 GetAndResetSnapshotRequested(), base::Passed(&response)),
      base::Bind(&PassThroughImageTransportSurface::BufferPresented,
                 weak_ptr_factory_.GetWeakPtr(), presentation_callback));
}

void PassThroughImageTransportSurface::SetVSyncEnabled(bool enabled) {
  if (vsync_enabled_ == enabled)
    return;
  vsync_enabled_ = enabled;
  GLSurfaceAdapter::SetVSyncEnabled(enabled);
}

void PassThroughImageTransportSurface::SetSnapshotRequested() {
  snapshot_requested_ = true;
}

bool PassThroughImageTransportSurface::GetAndResetSnapshotRequested() {
  bool sr = snapshot_requested_;
  snapshot_requested_ = false;
  return sr;
}

void PassThroughImageTransportSurface::UpdateVSyncEnabled() {
  if (is_gpu_vsync_disabled_) {
    SetVSyncEnabled(false);
    return;
  }

  bool should_override_vsync = false;
  if (is_multi_window_swap_vsync_override_enabled_) {
    // This code is a simple way of enforcing that we only vsync if one surface
    // is swapping per frame. This provides single window cases a stable refresh
    // while allowing multi-window cases to not slow down due to multiple syncs
    // on a single thread. A better way to fix this problem would be to have
    // each surface present on its own thread.

    if (g_current_swap_generation_ == swap_generation_) {
      // No other surface has swapped since we swapped last time.
      if (g_num_swaps_in_current_swap_generation_ > 1)
        g_last_multi_window_swap_generation_ = g_current_swap_generation_;
      g_num_swaps_in_current_swap_generation_ = 0;
      g_current_swap_generation_++;
    }

    swap_generation_ = g_current_swap_generation_;
    g_num_swaps_in_current_swap_generation_++;

    should_override_vsync =
        (g_num_swaps_in_current_swap_generation_ > 1) ||
        (g_current_swap_generation_ - g_last_multi_window_swap_generation_ <
         kMultiWindowSwapEnableVSyncDelay);
  }
  SetVSyncEnabled(!should_override_vsync);
}

void PassThroughImageTransportSurface::StartSwapBuffers(
    gfx::SwapResponse* response) {
  UpdateVSyncEnabled();
  allow_running_presentation_callback_ = false;

  // Populated later in the DecoderClient, before passing to client.
  response->swap_id = 0;

  response->swap_start = base::TimeTicks::Now();
}

void PassThroughImageTransportSurface::FinishSwapBuffers(
    bool snapshot_requested,
    gfx::SwapResponse response) {
  response.swap_end = base::TimeTicks::Now();
  if (snapshot_requested)
    WaitForSnapshotRendering();

  if (delegate_) {
    SwapBuffersCompleteParams params;
    params.swap_response = std::move(response);
    delegate_->DidSwapBuffersComplete(std::move(params));
  }
  allow_running_presentation_callback_ = true;
}

void PassThroughImageTransportSurface::FinishSwapBuffersAsync(
    GLSurface::SwapCompletionCallback callback,
    bool snapshot_requested,
    gfx::SwapResponse response,
    gfx::SwapResult result) {
  response.result = result;
  FinishSwapBuffers(snapshot_requested, std::move(response));
  callback.Run(result);
}

void PassThroughImageTransportSurface::BufferPresented(
    const GLSurface::PresentationCallback& callback,
    const gfx::PresentationFeedback& feedback) {
  DCHECK(allow_running_presentation_callback_);
  callback.Run(feedback);
  if (delegate_)
    delegate_->BufferPresented(feedback);
}

}  // namespace gpu
