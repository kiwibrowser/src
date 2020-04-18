// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/local/layer_tree_frame_sink_local.h"

#include "cc/trees/layer_tree_frame_sink_client.h"
#include "components/viz/common/surfaces/surface_info.h"
#include "components/viz/host/host_frame_sink_manager.h"
#include "components/viz/service/frame_sinks/compositor_frame_sink_support.h"
#include "ui/aura/client/cursor_client.h"
#include "ui/aura/env.h"
#include "ui/aura/window.h"
#include "ui/aura/window_delegate.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"

namespace aura {

LayerTreeFrameSinkLocal::LayerTreeFrameSinkLocal(
    const viz::FrameSinkId& frame_sink_id,
    viz::HostFrameSinkManager* host_frame_sink_manager,
    const std::string& debug_label)
    : cc::LayerTreeFrameSink(nullptr, nullptr, nullptr, nullptr),
      frame_sink_id_(frame_sink_id),
      host_frame_sink_manager_(host_frame_sink_manager) {
  host_frame_sink_manager_->RegisterFrameSinkId(frame_sink_id_, this);
  host_frame_sink_manager_->SetFrameSinkDebugLabel(frame_sink_id_, debug_label);
}

LayerTreeFrameSinkLocal::~LayerTreeFrameSinkLocal() {
  host_frame_sink_manager_->InvalidateFrameSinkId(frame_sink_id_);
}

bool LayerTreeFrameSinkLocal::BindToClient(
    cc::LayerTreeFrameSinkClient* client) {
  if (!cc::LayerTreeFrameSink::BindToClient(client))
    return false;
  DCHECK(!thread_checker_);
  thread_checker_ = std::make_unique<base::ThreadChecker>();

  support_ = host_frame_sink_manager_->CreateCompositorFrameSinkSupport(
      this, frame_sink_id_, false /* is_root */,
      true /* needs_sync_points */);
  begin_frame_source_ = std::make_unique<viz::ExternalBeginFrameSource>(this);
  client->SetBeginFrameSource(begin_frame_source_.get());
  return true;
}

void LayerTreeFrameSinkLocal::SetSurfaceChangedCallback(
    const SurfaceChangedCallback& callback) {
  DCHECK(!surface_changed_callback_);
  surface_changed_callback_ = callback;
}

void LayerTreeFrameSinkLocal::DetachFromClient() {
  DCHECK(thread_checker_);
  DCHECK(thread_checker_->CalledOnValidThread());
  client_->SetBeginFrameSource(nullptr);
  begin_frame_source_.reset();
  support_.reset();
  thread_checker_.reset();
  cc::LayerTreeFrameSink::DetachFromClient();
}

void LayerTreeFrameSinkLocal::SetLocalSurfaceId(
    const viz::LocalSurfaceId& local_surface_id) {
  DCHECK(local_surface_id.is_valid());
  local_surface_id_ = local_surface_id;
}

void LayerTreeFrameSinkLocal::SubmitCompositorFrame(
    viz::CompositorFrame frame) {
  DCHECK(thread_checker_);
  DCHECK(thread_checker_->CalledOnValidThread());
  DCHECK(frame.metadata.begin_frame_ack.has_damage);
  DCHECK_LE(viz::BeginFrameArgs::kStartingFrameNumber,
            frame.metadata.begin_frame_ack.sequence_number);

  DCHECK(local_surface_id_.is_valid());

  support_->SubmitCompositorFrame(local_surface_id_, std::move(frame));
}

void LayerTreeFrameSinkLocal::DidNotProduceFrame(
    const viz::BeginFrameAck& ack) {
  DCHECK(thread_checker_);
  DCHECK(thread_checker_->CalledOnValidThread());
  DCHECK(!ack.has_damage);
  DCHECK_LE(viz::BeginFrameArgs::kStartingFrameNumber, ack.sequence_number);
  support_->DidNotProduceFrame(ack);
}

void LayerTreeFrameSinkLocal::DidAllocateSharedBitmap(
    mojo::ScopedSharedBufferHandle buffer,
    const viz::SharedBitmapId& id) {
  // No software compositing used with this implementation.
  NOTIMPLEMENTED();
}

void LayerTreeFrameSinkLocal::DidDeleteSharedBitmap(
    const viz::SharedBitmapId& id) {
  // No software compositing used with this implementation.
  NOTIMPLEMENTED();
}

void LayerTreeFrameSinkLocal::DidReceiveCompositorFrameAck(
    const std::vector<viz::ReturnedResource>& resources) {
  DCHECK(thread_checker_);
  DCHECK(thread_checker_->CalledOnValidThread());
  if (!client_)
    return;
  if (!resources.empty())
    client_->ReclaimResources(resources);
  client_->DidReceiveCompositorFrameAck();
}

void LayerTreeFrameSinkLocal::DidPresentCompositorFrame(
    uint32_t presentation_token,
    base::TimeTicks time,
    base::TimeDelta refresh,
    uint32_t flags) {
  DCHECK(thread_checker_);
  DCHECK(thread_checker_->CalledOnValidThread());
  client_->DidPresentCompositorFrame(presentation_token, time, refresh, flags);
}

void LayerTreeFrameSinkLocal::DidDiscardCompositorFrame(
    uint32_t presentation_token) {
  DCHECK(thread_checker_);
  DCHECK(thread_checker_->CalledOnValidThread());
  client_->DidDiscardCompositorFrame(presentation_token);
}

void LayerTreeFrameSinkLocal::OnBeginFrame(const viz::BeginFrameArgs& args) {
  DCHECK(thread_checker_);
  DCHECK(thread_checker_->CalledOnValidThread());
  begin_frame_source_->OnBeginFrame(args);
}

void LayerTreeFrameSinkLocal::OnBeginFramePausedChanged(bool paused) {
  DCHECK(thread_checker_);
  DCHECK(thread_checker_->CalledOnValidThread());
  begin_frame_source_->OnSetBeginFrameSourcePaused(paused);
}

void LayerTreeFrameSinkLocal::ReclaimResources(
    const std::vector<viz::ReturnedResource>& resources) {
  DCHECK(thread_checker_);
  DCHECK(thread_checker_->CalledOnValidThread());
  if (!client_)
    return;
  client_->ReclaimResources(resources);
}

void LayerTreeFrameSinkLocal::OnNeedsBeginFrames(bool needs_begin_frames) {
  DCHECK(thread_checker_);
  DCHECK(thread_checker_->CalledOnValidThread());
  support_->SetNeedsBeginFrame(needs_begin_frames);
}

void LayerTreeFrameSinkLocal::OnFirstSurfaceActivation(
    const viz::SurfaceInfo& surface_info) {
  surface_changed_callback_.Run(surface_info);
}

void LayerTreeFrameSinkLocal::OnFrameTokenChanged(uint32_t frame_token) {
  // TODO(yiyix, fsamuel): Implement frame token propagation for
  // LayerTreeFrameSinkLocal.
  NOTREACHED();
}

}  // namespace aura
