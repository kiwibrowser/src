// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_IPC_SERVICE_PASS_THROUGH_IMAGE_TRANSPORT_SURFACE_H_
#define GPU_IPC_SERVICE_PASS_THROUGH_IMAGE_TRANSPORT_SURFACE_H_

#include <stdint.h>

#include <memory>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "gpu/ipc/service/image_transport_surface.h"
#include "gpu/ipc/service/image_transport_surface_delegate.h"
#include "ui/gl/gl_surface.h"

namespace gpu {

// An implementation of ImageTransportSurface that implements GLSurface through
// GLSurfaceAdapter, thereby forwarding GLSurface methods through to it.
class PassThroughImageTransportSurface : public gl::GLSurfaceAdapter {
 public:
  PassThroughImageTransportSurface(
      base::WeakPtr<ImageTransportSurfaceDelegate> delegate,
      gl::GLSurface* surface,
      bool override_vsync_for_multi_window_swap);

  // GLSurface implementation.
  bool Initialize(gl::GLSurfaceFormat format) override;
  gfx::SwapResult SwapBuffers(const PresentationCallback& callback) override;
  void SwapBuffersAsync(
      const SwapCompletionCallback& completion_callback,
      const PresentationCallback& presentation_callback) override;
  gfx::SwapResult SwapBuffersWithBounds(
      const std::vector<gfx::Rect>& rects,
      const PresentationCallback& callback) override;
  gfx::SwapResult PostSubBuffer(int x,
                                int y,
                                int width,
                                int height,
                                const PresentationCallback& callback) override;
  void PostSubBufferAsync(
      int x,
      int y,
      int width,
      int height,
      const SwapCompletionCallback& completion_callback,
      const PresentationCallback& presentation_callback) override;
  gfx::SwapResult CommitOverlayPlanes(
      const PresentationCallback& callback) override;
  void CommitOverlayPlanesAsync(
      const SwapCompletionCallback& completion_callback,
      const PresentationCallback& presentation_callback) override;
  void SetVSyncEnabled(bool enabled) override;

 private:
  ~PassThroughImageTransportSurface() override;

  void SetSnapshotRequested();
  bool GetAndResetSnapshotRequested();

  void UpdateVSyncEnabled();

  void StartSwapBuffers(gfx::SwapResponse* response);
  void FinishSwapBuffers(bool snapshot_requested, gfx::SwapResponse response);
  void FinishSwapBuffersAsync(GLSurface::SwapCompletionCallback callback,
                              bool snapshot_requested,
                              gfx::SwapResponse response,
                              gfx::SwapResult result);

  void BufferPresented(const GLSurface::PresentationCallback& callback,
                       const gfx::PresentationFeedback& feedback);

  const bool is_gpu_vsync_disabled_;
  const bool is_multi_window_swap_vsync_override_enabled_;
  base::WeakPtr<ImageTransportSurfaceDelegate> delegate_;
  bool snapshot_requested_ = false;
  int swap_generation_ = 0;
  bool vsync_enabled_ = true;
  bool allow_running_presentation_callback_ = true;

  base::WeakPtrFactory<PassThroughImageTransportSurface> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(PassThroughImageTransportSurface);
};

}  // namespace gpu

#endif  // GPU_IPC_SERVICE_PASS_THROUGH_IMAGE_TRANSPORT_SURFACE_H_
