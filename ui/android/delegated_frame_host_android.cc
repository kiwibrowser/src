// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/android/delegated_frame_host_android.h"

#include "base/android/build_info.h"
#include "base/bind.h"
#include "base/logging.h"
#include "cc/layers/solid_color_layer.h"
#include "cc/layers/surface_layer.h"
#include "components/viz/common/features.h"
#include "components/viz/common/frame_sinks/copy_output_result.h"
#include "components/viz/common/quads/compositor_frame.h"
#include "components/viz/common/surfaces/surface_id.h"
#include "components/viz/host/host_frame_sink_manager.h"
#include "components/viz/service/surfaces/surface.h"
#include "ui/android/view_android.h"
#include "ui/android/window_android.h"
#include "ui/android/window_android_compositor.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/gfx/geometry/dip_util.h"

namespace ui {

namespace {

// Wait up to 5 seconds for the first frame to be produced. Having Android
// display a placeholder for a longer period of time is preferable to drawing
// nothing, and the first frame can take a while on low-end systems.
static const int64_t kFirstFrameTimeoutSeconds = 5;

// Wait up to 1 second for a frame of the correct size to be produced. Android
// OS will only wait 4 seconds, so we limit this to 1 second to make sure we
// have always produced a frame before the OS stops waiting.
static const int64_t kResizeTimeoutSeconds = 1;

constexpr viz::LocalSurfaceId kInvalidLocalSurfaceId;

scoped_refptr<cc::SurfaceLayer> CreateSurfaceLayer(
    const viz::SurfaceId& surface_id,
    const gfx::Size& size_in_pixels,
    bool surface_opaque) {
  // manager must outlive compositors using it.
  auto layer = cc::SurfaceLayer::Create();
  layer->SetPrimarySurfaceId(surface_id,
                             cc::DeadlinePolicy::UseDefaultDeadline());
  layer->SetFallbackSurfaceId(surface_id);
  layer->SetBounds(size_in_pixels);
  layer->SetIsDrawable(true);
  layer->SetContentsOpaque(surface_opaque);
  layer->SetSurfaceHitTestable(true);

  return layer;
}

}  // namespace

DelegatedFrameHostAndroid::DelegatedFrameHostAndroid(
    ui::ViewAndroid* view,
    viz::HostFrameSinkManager* host_frame_sink_manager,
    Client* client,
    const viz::FrameSinkId& frame_sink_id)
    : frame_sink_id_(frame_sink_id),
      view_(view),
      host_frame_sink_manager_(host_frame_sink_manager),
      client_(client),
      begin_frame_source_(this),
      enable_surface_synchronization_(
          features::IsSurfaceSynchronizationEnabled()),
      enable_viz_(
          base::FeatureList::IsEnabled(features::kVizDisplayCompositor)) {
  DCHECK(view_);
  DCHECK(client_);

  host_frame_sink_manager_->RegisterFrameSinkId(frame_sink_id_, this);
  host_frame_sink_manager_->SetFrameSinkDebugLabel(frame_sink_id_,
                                                   "DelegatedFrameHostAndroid");
  CreateNewCompositorFrameSinkSupport();
}

DelegatedFrameHostAndroid::~DelegatedFrameHostAndroid() {
  DestroyDelegatedContent();
  DetachFromCompositor();
  support_.reset();
  host_frame_sink_manager_->InvalidateFrameSinkId(frame_sink_id_);
}

void DelegatedFrameHostAndroid::SubmitCompositorFrame(
    const viz::LocalSurfaceId& local_surface_id,
    viz::CompositorFrame frame,
    base::Optional<viz::HitTestRegionList> hit_test_region_list) {
  DCHECK(!enable_viz_);

  if (local_surface_id != surface_info_.id().local_surface_id()) {
    DestroyDelegatedContent();
    DCHECK(!content_layer_);

    viz::RenderPass* root_pass = frame.render_pass_list.back().get();
    gfx::Size frame_size = root_pass->output_rect.size();
    surface_info_ = viz::SurfaceInfo(
        viz::SurfaceId(frame_sink_id_, local_surface_id), 1.f, frame_size);
    has_transparent_background_ = root_pass->has_transparent_background;

    support_->SubmitCompositorFrame(local_surface_id, std::move(frame),
                                    std::move(hit_test_region_list));

    content_layer_ =
        CreateSurfaceLayer(surface_info_.id(), surface_info_.size_in_pixels(),
                           !has_transparent_background_);
    view_->GetLayer()->AddChild(content_layer_);
  } else {
    support_->SubmitCompositorFrame(local_surface_id, std::move(frame),
                                    std::move(hit_test_region_list));
  }

  compositor_attach_until_frame_lock_.reset();

  DCHECK(content_layer_);
  if (content_layer_->bounds() == expected_pixel_size_)
    compositor_pending_resize_lock_.reset();
}

void DelegatedFrameHostAndroid::DidNotProduceFrame(
    const viz::BeginFrameAck& ack) {
  DCHECK(!enable_viz_);
  support_->DidNotProduceFrame(ack);
}

viz::FrameSinkId DelegatedFrameHostAndroid::GetFrameSinkId() const {
  return frame_sink_id_;
}

void DelegatedFrameHostAndroid::CopyFromCompositingSurface(
    const gfx::Rect& src_subrect,
    const gfx::Size& output_size,
    base::OnceCallback<void(const SkBitmap&)> callback) {
  // TODO(vmpstr): We should defer this request until such time that this
  // returns true. https://crbug.com/826097.
  if (!CanCopyFromCompositingSurface()) {
    std::move(callback).Run(SkBitmap());
    return;
  }

  scoped_refptr<cc::Layer> readback_layer =
      CreateSurfaceLayer(surface_info_.id(), surface_info_.size_in_pixels(),
                         !has_transparent_background_);
  readback_layer->SetHideLayerAndSubtree(true);
  view_->GetWindowAndroid()->GetCompositor()->AttachLayerForReadback(
      readback_layer);
  std::unique_ptr<viz::CopyOutputRequest> request =
      std::make_unique<viz::CopyOutputRequest>(
          viz::CopyOutputRequest::ResultFormat::RGBA_BITMAP,
          base::BindOnce(
              [](base::OnceCallback<void(const SkBitmap&)> callback,
                 scoped_refptr<cc::Layer> readback_layer,
                 std::unique_ptr<viz::CopyOutputResult> result) {
                readback_layer->RemoveFromParent();
                std::move(callback).Run(result->AsSkBitmap());
              },
              std::move(callback), std::move(readback_layer)));

  if (src_subrect.IsEmpty()) {
    request->set_area(gfx::Rect(surface_info_.size_in_pixels()));
  } else {
    request->set_area(
        gfx::ConvertRectToPixel(view_->GetDipScale(), src_subrect));
  }

  if (!output_size.IsEmpty()) {
    request->set_result_selection(gfx::Rect(output_size));
    request->SetScaleRatio(
        gfx::Vector2d(request->area().width(), request->area().height()),
        gfx::Vector2d(output_size.width(), output_size.height()));
  }
  support_->RequestCopyOfOutput(kInvalidLocalSurfaceId, std::move(request));
}

bool DelegatedFrameHostAndroid::CanCopyFromCompositingSurface() const {
  // TODO(ericrk): Handle Viz cases here.
  return support_ && surface_info_.is_valid() && view_->GetWindowAndroid() &&
         view_->GetWindowAndroid()->GetCompositor();
}

void DelegatedFrameHostAndroid::DestroyDelegatedContent() {
  // TakeFallbackContentFrom() can populate |content_layer_| when
  // |surface_info_| is invalid.
  if (content_layer_) {
    content_layer_->RemoveFromParent();
    content_layer_ = nullptr;
  }

  if (surface_info_.is_valid()) {
    support_->EvictLastActivatedSurface();
    surface_info_ = viz::SurfaceInfo();
  }
}

bool DelegatedFrameHostAndroid::HasDelegatedContent() const {
  return surface_info_.is_valid();
}

void DelegatedFrameHostAndroid::CompositorFrameSinkChanged() {
  DestroyDelegatedContent();
  CreateNewCompositorFrameSinkSupport();
  if (registered_parent_compositor_)
    AttachToCompositor(registered_parent_compositor_);
}

void DelegatedFrameHostAndroid::AttachToCompositor(
    WindowAndroidCompositor* compositor) {
  if (registered_parent_compositor_)
    DetachFromCompositor();
  // If this is the first frame after the compositor became visible, we want to
  // take the compositor lock, preventing compositor frames from being produced
  // until all delegated frames are ready. This improves the resume transition,
  // preventing flashes. Set a 5 second timeout to prevent locking up the
  // browser in cases where the renderer hangs or another factor prevents a
  // frame from being produced. If we already have delegated content, no need
  // to take the lock.
  if (!enable_viz_ && compositor->IsDrawingFirstVisibleFrame() &&
      !HasDelegatedContent()) {
    compositor_attach_until_frame_lock_ = compositor->GetCompositorLock(
        this, base::TimeDelta::FromSeconds(kFirstFrameTimeoutSeconds));
  }
  compositor->AddChildFrameSink(frame_sink_id_);
  if (!enable_viz_)
    client_->SetBeginFrameSource(&begin_frame_source_);
  registered_parent_compositor_ = compositor;
}

void DelegatedFrameHostAndroid::DetachFromCompositor() {
  if (!registered_parent_compositor_)
    return;
  compositor_attach_until_frame_lock_.reset();
  compositor_pending_resize_lock_.reset();
  if (!enable_viz_) {
    client_->SetBeginFrameSource(nullptr);
    support_->SetNeedsBeginFrame(false);
  }
  registered_parent_compositor_->RemoveChildFrameSink(frame_sink_id_);
  registered_parent_compositor_ = nullptr;
}

void DelegatedFrameHostAndroid::SynchronizeVisualProperties(gfx::Size size) {
  local_surface_id_allocator_.GenerateId();

  if (enable_viz_) {
    DestroyDelegatedContent();
    DCHECK(!content_layer_);

    // TODO(ericrk): Do we need to handle transparency in Viz?
    bool is_transparent = true;
    content_layer_ = CreateSurfaceLayer(
        viz::SurfaceId(frame_sink_id_,
                       local_surface_id_allocator_.GetCurrentLocalSurfaceId()),
        size, is_transparent);
    view_->GetLayer()->AddChild(content_layer_);
  }
}

void DelegatedFrameHostAndroid::PixelSizeWillChange(
    const gfx::Size& pixel_size) {
  if (enable_surface_synchronization_)
    return;

  // We never take the resize lock unless we're on O+, as previous versions of
  // Android won't wait for us to produce the correct sized frame and will end
  // up looking worse.
  if (base::android::BuildInfo::GetInstance()->sdk_int() <
      base::android::SDK_VERSION_OREO) {
    return;
  }

  expected_pixel_size_ = pixel_size;
  if (registered_parent_compositor_) {
    if (!content_layer_ || content_layer_->bounds() != expected_pixel_size_) {
      compositor_pending_resize_lock_ =
          registered_parent_compositor_->GetCompositorLock(
              this, base::TimeDelta::FromSeconds(kResizeTimeoutSeconds));
    }
  }
}

void DelegatedFrameHostAndroid::DidReceiveCompositorFrameAck(
    const std::vector<viz::ReturnedResource>& resources) {
  client_->ReclaimResources(resources);
  client_->DidReceiveCompositorFrameAck();
}

void DelegatedFrameHostAndroid::DidPresentCompositorFrame(
    uint32_t presentation_token,
    base::TimeTicks time,
    base::TimeDelta refresh,
    uint32_t flags) {
  client_->DidPresentCompositorFrame(presentation_token, time, refresh, flags);
}

void DelegatedFrameHostAndroid::DidDiscardCompositorFrame(
    uint32_t presentation_token) {
  client_->DidDiscardCompositorFrame(presentation_token);
}

void DelegatedFrameHostAndroid::OnBeginFrame(const viz::BeginFrameArgs& args) {
  if (enable_viz_) {
    NOTREACHED();
    return;
  }
  begin_frame_source_.OnBeginFrame(args);
}

void DelegatedFrameHostAndroid::ReclaimResources(
    const std::vector<viz::ReturnedResource>& resources) {
  client_->ReclaimResources(resources);
}

void DelegatedFrameHostAndroid::OnBeginFramePausedChanged(bool paused) {
  begin_frame_source_.OnSetBeginFrameSourcePaused(paused);
}

void DelegatedFrameHostAndroid::OnNeedsBeginFrames(bool needs_begin_frames) {
  DCHECK(!enable_viz_);
  support_->SetNeedsBeginFrame(needs_begin_frames);
}

void DelegatedFrameHostAndroid::OnFirstSurfaceActivation(
    const viz::SurfaceInfo& surface_info) {
  if (!enable_viz_)
    return;

  uint32_t parent_sequence_number =
      surface_info.id().local_surface_id().parent_sequence_number();

  // TODO(ericrk): Handle errors due to wrapping.
  if (parent_sequence_number < parent_sequence_number_at_navigation_) {
    // TODO(ericrk): Drop reference from HostFrameSinkManager.
    return;
  }

  if (!received_frame_after_navigation_) {
    client_->DidReceiveFirstFrameAfterNavigation();
    received_frame_after_navigation_ = true;
  }

  if (!content_layer_) {
    // TODO(ericrk): Drop reference from HostFrameSinkManager.
    return;
  }

  content_layer_->SetFallbackSurfaceId(surface_info.id());
}

void DelegatedFrameHostAndroid::OnFrameTokenChanged(uint32_t frame_token) {
  client_->OnFrameTokenChanged(frame_token);
}

void DelegatedFrameHostAndroid::CompositorLockTimedOut() {}

void DelegatedFrameHostAndroid::CreateNewCompositorFrameSinkSupport() {
  if (enable_viz_)
    return;

  constexpr bool is_root = false;
  constexpr bool needs_sync_points = true;
  support_.reset();
  support_ = host_frame_sink_manager_->CreateCompositorFrameSinkSupport(
      this, frame_sink_id_, is_root, needs_sync_points);
}

const viz::SurfaceId& DelegatedFrameHostAndroid::SurfaceId() const {
  return surface_info_.id();
}

const viz::LocalSurfaceId& DelegatedFrameHostAndroid::GetLocalSurfaceId()
    const {
  return local_surface_id_allocator_.GetCurrentLocalSurfaceId();
}

void DelegatedFrameHostAndroid::TakeFallbackContentFrom(
    DelegatedFrameHostAndroid* other) {
  if (content_layer_ || !other->content_layer_)
    return;
  content_layer_ =
      CreateSurfaceLayer(other->content_layer_->fallback_surface_id(),
                         other->content_layer_->bounds(),
                         other->content_layer_->contents_opaque());
  view_->GetLayer()->AddChild(content_layer_);
}

void DelegatedFrameHostAndroid::DidNavigate() {
  if (!enable_surface_synchronization_)
    return;

  received_frame_after_navigation_ = false;
  parent_sequence_number_at_navigation_ =
      local_surface_id_allocator_.GetCurrentLocalSurfaceId()
          .parent_sequence_number();
}

}  // namespace ui
