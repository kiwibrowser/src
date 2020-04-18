// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_IPC_SERVICE_IMAGE_TRANSPORT_SURFACE_OVERLAY_MAC_H_
#define GPU_IPC_SERVICE_IMAGE_TRANSPORT_SURFACE_OVERLAY_MAC_H_

#include <vector>

#import "base/mac/scoped_nsobject.h"
#include "base/memory/weak_ptr.h"
#include "gpu/ipc/service/command_buffer_stub.h"
#include "gpu/ipc/service/image_transport_surface.h"
#include "ui/gfx/presentation_feedback.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_surface.h"
#include "ui/gl/gpu_switching_observer.h"

@class CAContext;
@class CALayer;

namespace ui {
class CALayerTreeCoordinator;
struct CARendererLayerParams;
}

namespace gl {
class GLFence;
}

namespace gpu {

class ImageTransportSurfaceOverlayMac : public gl::GLSurface,
                                        public ui::GpuSwitchingObserver {
 public:
  explicit ImageTransportSurfaceOverlayMac(
      base::WeakPtr<ImageTransportSurfaceDelegate> delegate);

  // GLSurface implementation
  bool Initialize(gl::GLSurfaceFormat format) override;
  void Destroy() override;
  void PrepareToDestroy(bool have_context) override;
  bool Resize(const gfx::Size& size,
              float scale_factor,
              ColorSpace color_space,
              bool has_alpha) override;
  bool IsOffscreen() override;
  gfx::SwapResult SwapBuffers(const PresentationCallback& callback) override;
  gfx::SwapResult PostSubBuffer(int x,
                                int y,
                                int width,
                                int height,
                                const PresentationCallback& callback) override;
  bool SupportsPostSubBuffer() override;
  gfx::Size GetSize() override;
  void* GetHandle() override;
  gl::GLSurfaceFormat GetFormat() override;
  bool OnMakeCurrent(gl::GLContext* context) override;
  bool ScheduleOverlayPlane(int z_order,
                            gfx::OverlayTransform transform,
                            gl::GLImage* image,
                            const gfx::Rect& bounds_rect,
                            const gfx::RectF& crop_rect,
                            bool enable_blend,
                            std::unique_ptr<gfx::GpuFence> gpu_fence) override;
  bool ScheduleCALayer(const ui::CARendererLayerParams& params) override;
  void ScheduleCALayerInUseQuery(
      std::vector<CALayerInUseQuery> queries) override;
  bool IsSurfaceless() const override;
  bool SupportsPresentationCallback() override;

  // ui::GpuSwitchingObserver implementation.
  void OnGpuSwitched() override;

 private:
  ~ImageTransportSurfaceOverlayMac() override;

  void SetSnapshotRequested();
  bool GetAndResetSnapshotRequested();

  gfx::SwapResult SwapBuffersInternal(const gfx::Rect& pixel_damage_rect,
                                      const PresentationCallback& callback);
  void ApplyBackpressure(base::TimeTicks* before_flush_time,
                         base::TimeTicks* after_flush_before_commit_time);
  void BufferPresented(const PresentationCallback& callback,
                       const gfx::PresentationFeedback& feedback);

  base::WeakPtr<ImageTransportSurfaceDelegate> delegate_;

  bool use_remote_layer_api_;
  base::scoped_nsobject<CAContext> ca_context_;
  std::unique_ptr<ui::CALayerTreeCoordinator> ca_layer_tree_coordinator_;

  gfx::Size pixel_size_;
  float scale_factor_;

  std::vector<CALayerInUseQuery> ca_layer_in_use_queries_;

  // A GLFence marking the end of the previous frame. Must only be accessed
  // while the associated |previous_frame_context_| is bound.
  std::unique_ptr<gl::GLFence> previous_frame_fence_;
  base::ScopedTypeRef<CGLContextObj> fence_context_obj_;

  // The renderer ID that all contexts made current to this surface should be
  // targeting.
  GLint gl_renderer_id_;
  base::WeakPtrFactory<ImageTransportSurfaceOverlayMac> weak_ptr_factory_;
};

}  // namespace gpu

#endif  // GPU_IPC_SERVICE_IMAGE_TRANSPORT_SURFACE_OVERLAY_MAC_H_
