// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_ANDROID_DELEGATED_FRAME_HOST_ANDROID_H_
#define UI_ANDROID_DELEGATED_FRAME_HOST_ANDROID_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "components/viz/common/frame_sinks/copy_output_request.h"
#include "components/viz/common/resources/returned_resource.h"
#include "components/viz/common/surfaces/parent_local_surface_id_allocator.h"
#include "components/viz/common/surfaces/surface_info.h"
#include "components/viz/host/host_frame_sink_client.h"
#include "components/viz/service/frame_sinks/compositor_frame_sink_support.h"
#include "services/viz/public/interfaces/compositing/compositor_frame_sink.mojom.h"
#include "ui/android/ui_android_export.h"
#include "ui/compositor/compositor_lock.h"

namespace cc {
class SurfaceLayer;
enum class SurfaceDrawStatus;
}  // namespace cc

namespace viz {
class CompositorFrame;
class HostFrameSinkManager;
}  // namespace viz

namespace ui {
class ViewAndroid;
class WindowAndroidCompositor;

class UI_ANDROID_EXPORT DelegatedFrameHostAndroid
    : public viz::mojom::CompositorFrameSinkClient,
      public viz::ExternalBeginFrameSourceClient,
      public viz::HostFrameSinkClient,
      public ui::CompositorLockClient {
 public:
  class Client {
   public:
    virtual void SetBeginFrameSource(
        viz::BeginFrameSource* begin_frame_source) = 0;
    virtual void DidReceiveCompositorFrameAck() = 0;
    virtual void DidPresentCompositorFrame(uint32_t presentation_token,
                                           base::TimeTicks time,
                                           base::TimeDelta refresh,
                                           uint32_t flags) = 0;
    virtual void DidDiscardCompositorFrame(uint32_t presentation_token) = 0;
    virtual void ReclaimResources(
        const std::vector<viz::ReturnedResource>&) = 0;
    virtual void OnFrameTokenChanged(uint32_t frame_token) = 0;
    virtual void DidReceiveFirstFrameAfterNavigation() = 0;
  };

  DelegatedFrameHostAndroid(ViewAndroid* view,
                            viz::HostFrameSinkManager* host_frame_sink_manager,
                            Client* client,
                            const viz::FrameSinkId& frame_sink_id);

  ~DelegatedFrameHostAndroid() override;

  void SubmitCompositorFrame(
      const viz::LocalSurfaceId& local_surface_id,
      viz::CompositorFrame frame,
      base::Optional<viz::HitTestRegionList> hit_test_region_list);
  void DidNotProduceFrame(const viz::BeginFrameAck& ack);

  void DestroyDelegatedContent();

  bool HasDelegatedContent() const;

  viz::FrameSinkId GetFrameSinkId() const;

  // Should only be called when the host has a content layer. Use this for one-
  // off screen capture, not for video. Always provides RGBA_BITMAP
  // CopyOutputResults.
  void CopyFromCompositingSurface(
      const gfx::Rect& src_subrect,
      const gfx::Size& output_size,
      base::OnceCallback<void(const SkBitmap&)> callback);
  bool CanCopyFromCompositingSurface() const;

  void CompositorFrameSinkChanged();

  // Called when this DFH is attached/detached from a parent browser compositor
  // and needs to be attached to the surface hierarchy.
  void AttachToCompositor(WindowAndroidCompositor* compositor);
  void DetachFromCompositor();

  void SynchronizeVisualProperties(gfx::Size size_in_pixels);

  // Called when we begin a resize operation. Takes the compositor lock until we
  // receive a frame of the expected size.
  void PixelSizeWillChange(const gfx::Size& pixel_size);

  // Returns the ID for the current Surface. Returns an invalid ID if no
  // surface exists (!HasDelegatedContent()).
  const viz::SurfaceId& SurfaceId() const;

  // Returns the local surface ID for this delegated content.
  const viz::LocalSurfaceId& GetLocalSurfaceId() const;

  // TODO(fsamuel): We should move the viz::ParentLocalSurfaceIdAllocator to
  // RenderWidgetHostViewAndroid.
  viz::ParentLocalSurfaceIdAllocator* GetLocalSurfaceIdAllocator() {
    return &local_surface_id_allocator_;
  }

  void TakeFallbackContentFrom(DelegatedFrameHostAndroid* other);

  void DidNavigate();

 private:
  // viz::mojom::CompositorFrameSinkClient implementation.
  void DidReceiveCompositorFrameAck(
      const std::vector<viz::ReturnedResource>& resources) override;
  void DidPresentCompositorFrame(uint32_t presentation_token,
                                 base::TimeTicks time,
                                 base::TimeDelta refresh,
                                 uint32_t flags) override;
  void DidDiscardCompositorFrame(uint32_t presentation_token) override;
  void OnBeginFrame(const viz::BeginFrameArgs& args) override;
  void ReclaimResources(
      const std::vector<viz::ReturnedResource>& resources) override;
  void OnBeginFramePausedChanged(bool paused) override;

  // viz::ExternalBeginFrameSourceClient implementation.
  void OnNeedsBeginFrames(bool needs_begin_frames) override;

  // viz::HostFrameSinkClient implementation.
  void OnFirstSurfaceActivation(const viz::SurfaceInfo& surface_info) override;
  void OnFrameTokenChanged(uint32_t frame_token) override;

  // ui::CompositorLockClient implementation.
  void CompositorLockTimedOut() override;

  void CreateNewCompositorFrameSinkSupport();

  const viz::FrameSinkId frame_sink_id_;

  ViewAndroid* view_;

  viz::HostFrameSinkManager* const host_frame_sink_manager_;
  WindowAndroidCompositor* registered_parent_compositor_ = nullptr;
  Client* client_;

  std::unique_ptr<viz::CompositorFrameSinkSupport> support_;
  viz::ExternalBeginFrameSource begin_frame_source_;

  viz::SurfaceInfo surface_info_;
  bool has_transparent_background_ = false;

  scoped_refptr<cc::SurfaceLayer> content_layer_;

  const bool enable_surface_synchronization_;
  const bool enable_viz_;
  viz::ParentLocalSurfaceIdAllocator local_surface_id_allocator_;

  // The size we are resizing to. Once we receive a frame of this size we can
  // release any resize compositor lock.
  gfx::Size expected_pixel_size_;

  // A lock that is held from the point at which we attach to the compositor to
  // the point at which we submit our first frame to the compositor. This
  // ensures that the compositor doesn't swap without a frame available.
  std::unique_ptr<ui::CompositorLock> compositor_attach_until_frame_lock_;

  // A lock that is held from the point we begin resizing this frame to the
  // point at which we receive a frame of the correct size.
  std::unique_ptr<ui::CompositorLock> compositor_pending_resize_lock_;

  // Whether we've received a frame from the renderer since navigating.
  // Only used in Viz mode.
  bool received_frame_after_navigation_ = false;
  uint32_t parent_sequence_number_at_navigation_ = 0;

  DISALLOW_COPY_AND_ASSIGN(DelegatedFrameHostAndroid);
};

}  // namespace ui

#endif  // UI_ANDROID_DELEGATED_FRAME_HOST_ANDROID_H_
