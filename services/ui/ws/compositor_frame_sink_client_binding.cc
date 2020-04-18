// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/compositor_frame_sink_client_binding.h"

namespace ui {
namespace ws {

CompositorFrameSinkClientBinding::CompositorFrameSinkClientBinding(
    viz::mojom::CompositorFrameSinkClient* sink_client,
    viz::mojom::CompositorFrameSinkClientRequest sink_client_request,
    viz::mojom::CompositorFrameSinkAssociatedPtr compositor_frame_sink,
    viz::mojom::DisplayPrivateAssociatedPtr display_private)
    : binding_(sink_client, std::move(sink_client_request)),
      display_private_(std::move(display_private)),
      compositor_frame_sink_(std::move(compositor_frame_sink)) {}

CompositorFrameSinkClientBinding::~CompositorFrameSinkClientBinding() = default;

void CompositorFrameSinkClientBinding::SetWantsAnimateOnlyBeginFrames() {
  compositor_frame_sink_->SetWantsAnimateOnlyBeginFrames();
}

void CompositorFrameSinkClientBinding::SetNeedsBeginFrame(
    bool needs_begin_frame) {
  compositor_frame_sink_->SetNeedsBeginFrame(needs_begin_frame);
}

void CompositorFrameSinkClientBinding::SubmitCompositorFrame(
    const viz::LocalSurfaceId& local_surface_id,
    viz::CompositorFrame frame,
    base::Optional<viz::HitTestRegionList> hit_test_region_list,
    uint64_t submit_time) {
  compositor_frame_sink_->SubmitCompositorFrame(
      local_surface_id, std::move(frame), std::move(hit_test_region_list),
      submit_time);
}

void CompositorFrameSinkClientBinding::SubmitCompositorFrameSync(
    const viz::LocalSurfaceId& local_surface_id,
    viz::CompositorFrame frame,
    base::Optional<viz::HitTestRegionList> hit_test_region_list,
    uint64_t submit_time,
    const SubmitCompositorFrameSyncCallback callback) {
  NOTIMPLEMENTED();
}

void CompositorFrameSinkClientBinding::DidNotProduceFrame(
    const viz::BeginFrameAck& ack) {
  compositor_frame_sink_->DidNotProduceFrame(ack);
}

void CompositorFrameSinkClientBinding::DidAllocateSharedBitmap(
    mojo::ScopedSharedBufferHandle buffer,
    const viz::SharedBitmapId& id) {
  compositor_frame_sink_->DidAllocateSharedBitmap(std::move(buffer), id);
}

void CompositorFrameSinkClientBinding::DidDeleteSharedBitmap(
    const viz::SharedBitmapId& id) {
  compositor_frame_sink_->DidDeleteSharedBitmap(id);
}

}  // namespace ws
}  // namespace ui
