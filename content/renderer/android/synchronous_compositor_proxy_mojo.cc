// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/android/synchronous_compositor_proxy_mojo.h"

namespace content {

SynchronousCompositorProxyMojo::SynchronousCompositorProxyMojo(
    ui::SynchronousInputHandlerProxy* input_handler_proxy)
    : SynchronousCompositorProxy(input_handler_proxy), binding_(this) {}

SynchronousCompositorProxyMojo::~SynchronousCompositorProxyMojo() {}

void SynchronousCompositorProxyMojo::SendDemandDrawHwAsyncReply(
    const content::SyncCompositorCommonRendererParams&,
    uint32_t layer_tree_frame_sink_id,
    uint32_t metadata_version,
    base::Optional<viz::CompositorFrame> frame) {
  control_host_->ReturnFrame(layer_tree_frame_sink_id, metadata_version,
                             std::move(frame));
}

void SynchronousCompositorProxyMojo::SendBeginFrameResponse(
    const content::SyncCompositorCommonRendererParams& param) {
  control_host_->BeginFrameResponse(param);
}

void SynchronousCompositorProxyMojo::SendAsyncRendererStateIfNeeded() {
  if (hardware_draw_reply_ || software_draw_reply_ || zoom_by_reply_ || !host_)
    return;

  SyncCompositorCommonRendererParams params;
  PopulateCommonParams(&params);
  host_->UpdateState(params);
}

void SynchronousCompositorProxyMojo::SendSetNeedsBeginFrames(
    bool needs_begin_frames) {
  needs_begin_frame_ = needs_begin_frames;
  if (host_)
    host_->SetNeedsBeginFrames(needs_begin_frames);
}

void SynchronousCompositorProxyMojo::LayerTreeFrameSinkCreated() {
  DCHECK(layer_tree_frame_sink_);
  if (host_)
    host_->LayerTreeFrameSinkCreated();
}

void SynchronousCompositorProxyMojo::BindChannel(
    mojom::SynchronousCompositorControlHostPtr control_host,
    mojom::SynchronousCompositorHostAssociatedPtrInfo host,
    mojom::SynchronousCompositorAssociatedRequest compositor_request) {
  control_host_ = std::move(control_host);
  host_.Bind(std::move(host));
  binding_.Bind(std::move(compositor_request));

  if (layer_tree_frame_sink_)
    LayerTreeFrameSinkCreated();

  if (needs_begin_frame_)
    host_->SetNeedsBeginFrames(true);
}

}  // namespace content
