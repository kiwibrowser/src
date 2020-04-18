// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/frame_sinks/root_compositor_frame_sink_impl.h"

#include <utility>

#include "components/viz/common/frame_sinks/begin_frame_source.h"
#include "components/viz/service/display/display.h"
#include "components/viz/service/display_embedder/external_begin_frame_controller_impl.h"
#include "components/viz/service/frame_sinks/frame_sink_manager_impl.h"
#include "components/viz/service/hit_test/hit_test_aggregator.h"

namespace viz {

RootCompositorFrameSinkImpl::RootCompositorFrameSinkImpl(
    FrameSinkManagerImpl* frame_sink_manager,
    const FrameSinkId& frame_sink_id,
    std::unique_ptr<Display> display,
    std::unique_ptr<SyntheticBeginFrameSource> synthetic_begin_frame_source,
    std::unique_ptr<ExternalBeginFrameControllerImpl>
        external_begin_frame_controller,
    mojom::CompositorFrameSinkAssociatedRequest request,
    mojom::CompositorFrameSinkClientPtr client,
    mojom::DisplayPrivateAssociatedRequest display_private_request,
    mojom::DisplayClientPtr display_client)
    : compositor_frame_sink_client_(std::move(client)),
      compositor_frame_sink_binding_(this, std::move(request)),
      display_client_(std::move(display_client)),
      display_private_binding_(this, std::move(display_private_request)),
      support_(std::make_unique<CompositorFrameSinkSupport>(
          compositor_frame_sink_client_.get(),
          frame_sink_manager,
          frame_sink_id,
          true /* is_root */,
          true /* needs_sync_points */)),
      synthetic_begin_frame_source_(std::move(synthetic_begin_frame_source)),
      external_begin_frame_controller_(
          std::move(external_begin_frame_controller)),
      display_(std::move(display)) {
  DCHECK(begin_frame_source());
  DCHECK(display_);

  compositor_frame_sink_binding_.set_connection_error_handler(
      base::Bind(&RootCompositorFrameSinkImpl::OnClientConnectionLost,
                 base::Unretained(this)));
  if (external_begin_frame_controller_)
    external_begin_frame_controller_->SetDisplay(display_.get());
  frame_sink_manager->RegisterBeginFrameSource(begin_frame_source(),
                                               frame_sink_id);
  display_->Initialize(this, frame_sink_manager->surface_manager());
  support_->SetUpHitTest(display_.get());
}

RootCompositorFrameSinkImpl::~RootCompositorFrameSinkImpl() {
  support_->frame_sink_manager()->UnregisterBeginFrameSource(
      begin_frame_source());
  if (external_begin_frame_controller_)
    external_begin_frame_controller_->SetDisplay(nullptr);
}

void RootCompositorFrameSinkImpl::SetDisplayVisible(bool visible) {
  display_->SetVisible(visible);
}

void RootCompositorFrameSinkImpl::SetDisplayColorMatrix(
    const gfx::Transform& color_matrix) {
  display_->SetColorMatrix(color_matrix.matrix());
}

void RootCompositorFrameSinkImpl::SetDisplayColorSpace(
    const gfx::ColorSpace& blending_color_space,
    const gfx::ColorSpace& device_color_space) {
  display_->SetColorSpace(blending_color_space, device_color_space);
}

void RootCompositorFrameSinkImpl::SetOutputIsSecure(bool secure) {
  display_->SetOutputIsSecure(secure);
}

void RootCompositorFrameSinkImpl::SetAuthoritativeVSyncInterval(
    base::TimeDelta interval) {
  if (synthetic_begin_frame_source_)
    synthetic_begin_frame_source_->SetAuthoritativeVSyncInterval(interval);
}

void RootCompositorFrameSinkImpl::SetDisplayVSyncParameters(
    base::TimeTicks timebase,
    base::TimeDelta interval) {
  if (synthetic_begin_frame_source_)
    synthetic_begin_frame_source_->OnUpdateVSyncParameters(timebase, interval);
}

void RootCompositorFrameSinkImpl::SetNeedsBeginFrame(bool needs_begin_frame) {
  support_->SetNeedsBeginFrame(needs_begin_frame);
}

void RootCompositorFrameSinkImpl::SetWantsAnimateOnlyBeginFrames() {
  support_->SetWantsAnimateOnlyBeginFrames();
}

void RootCompositorFrameSinkImpl::SubmitCompositorFrame(
    const LocalSurfaceId& local_surface_id,
    CompositorFrame frame,
    base::Optional<HitTestRegionList> hit_test_region_list,
    uint64_t submit_time) {
  // Update display when size or local surface id changes.
  if (support_->last_activated_local_surface_id() != local_surface_id) {
    display_->Resize(frame.size_in_pixels());
    display_->SetLocalSurfaceId(local_surface_id, frame.device_scale_factor());
  }

  const auto result = support_->MaybeSubmitCompositorFrame(
      local_surface_id, std::move(frame), std::move(hit_test_region_list),
      SubmitCompositorFrameSyncCallback());
  if (result == CompositorFrameSinkSupport::ACCEPTED)
    return;

  const char* reason =
      CompositorFrameSinkSupport::GetSubmitResultAsString(result);
  DLOG(ERROR) << "SubmitCompositorFrame failed for " << local_surface_id
              << " because " << reason;
  compositor_frame_sink_binding_.CloseWithReason(static_cast<uint32_t>(result),
                                                 reason);
  OnClientConnectionLost();
}

void RootCompositorFrameSinkImpl::SubmitCompositorFrameSync(
    const LocalSurfaceId& local_surface_id,
    CompositorFrame frame,
    base::Optional<HitTestRegionList> hit_test_region_list,
    uint64_t submit_time,
    SubmitCompositorFrameSyncCallback callback) {
  NOTIMPLEMENTED();
}

void RootCompositorFrameSinkImpl::DidNotProduceFrame(
    const BeginFrameAck& begin_frame_ack) {
  support_->DidNotProduceFrame(begin_frame_ack);
}

void RootCompositorFrameSinkImpl::DidAllocateSharedBitmap(
    mojo::ScopedSharedBufferHandle buffer,
    const SharedBitmapId& id) {
  if (!support_->DidAllocateSharedBitmap(std::move(buffer), id)) {
    DLOG(ERROR) << "DidAllocateSharedBitmap failed for duplicate "
                << "SharedBitmapId";
    compositor_frame_sink_binding_.Close();
    OnClientConnectionLost();
  }
}

void RootCompositorFrameSinkImpl::DidDeleteSharedBitmap(
    const SharedBitmapId& id) {
  support_->DidDeleteSharedBitmap(id);
}

void RootCompositorFrameSinkImpl::DisplayOutputSurfaceLost() {
  // TODO(staraz): Implement this. Client should hear about context/output
  // surface lost.
}

void RootCompositorFrameSinkImpl::DisplayWillDrawAndSwap(
    bool will_draw_and_swap,
    const RenderPassList& render_pass) {
  DCHECK(support_->GetHitTestAggregator());
  support_->GetHitTestAggregator()->Aggregate(display_->CurrentSurfaceId());
}

void RootCompositorFrameSinkImpl::DisplayDidReceiveCALayerParams(
    const gfx::CALayerParams& ca_layer_params) {
  // If |ca_layer_params| should have content only when there exists a client
  // to send it to.
  DCHECK(ca_layer_params.is_empty || display_client_);
  if (display_client_)
    display_client_->OnDisplayReceivedCALayerParams(ca_layer_params);
}

void RootCompositorFrameSinkImpl::DidSwapAfterSnapshotRequestReceived(
    const std::vector<ui::LatencyInfo>& latency_info) {
  display_client_->DidSwapAfterSnapshotRequestReceived(latency_info);
}

void RootCompositorFrameSinkImpl::DisplayDidDrawAndSwap() {}

void RootCompositorFrameSinkImpl::OnClientConnectionLost() {
  // TODO(kylechar): I'm not sure what we need to do here. If |this| is
  // destroyed then |display_| will be destroyed and we'll stop producing
  // frames.
}

BeginFrameSource* RootCompositorFrameSinkImpl::begin_frame_source() {
  if (external_begin_frame_controller_)
    return external_begin_frame_controller_->begin_frame_source();
  return synthetic_begin_frame_source_.get();
}

}  // namespace viz
