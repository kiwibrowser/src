// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/client/client_layer_tree_frame_sink.h"

#include <utility>

#include "base/bind.h"
#include "base/trace_event/trace_event.h"
#include "cc/trees/layer_tree_frame_sink_client.h"
#include "components/viz/client/hit_test_data_provider.h"
#include "components/viz/client/local_surface_id_provider.h"
#include "components/viz/common/frame_sinks/begin_frame_args.h"
#include "components/viz/common/hit_test/hit_test_region_list.h"
#include "components/viz/common/quads/compositor_frame.h"

namespace viz {

ClientLayerTreeFrameSink::InitParams::InitParams() = default;

ClientLayerTreeFrameSink::InitParams::~InitParams() = default;

ClientLayerTreeFrameSink::UnboundMessagePipes::UnboundMessagePipes() = default;

ClientLayerTreeFrameSink::UnboundMessagePipes::~UnboundMessagePipes() = default;

bool ClientLayerTreeFrameSink::UnboundMessagePipes::HasUnbound() const {
  return client_request.is_pending() &&
         (compositor_frame_sink_info.is_valid() ^
          compositor_frame_sink_associated_info.is_valid());
}

ClientLayerTreeFrameSink::UnboundMessagePipes::UnboundMessagePipes(
    UnboundMessagePipes&& other) = default;

ClientLayerTreeFrameSink::ClientLayerTreeFrameSink(
    scoped_refptr<ContextProvider> context_provider,
    scoped_refptr<RasterContextProvider> worker_context_provider,
    InitParams* params)
    : cc::LayerTreeFrameSink(std::move(context_provider),
                             std::move(worker_context_provider),
                             std::move(params->compositor_task_runner),
                             params->gpu_memory_buffer_manager),
      hit_test_data_provider_(std::move(params->hit_test_data_provider)),
      local_surface_id_provider_(std::move(params->local_surface_id_provider)),
      synthetic_begin_frame_source_(
          std::move(params->synthetic_begin_frame_source)),
      pipes_(std::move(params->pipes)),
      client_binding_(this),
      enable_surface_synchronization_(params->enable_surface_synchronization),
      wants_animate_only_begin_frames_(params->wants_animate_only_begin_frames),
      weak_factory_(this) {
  DETACH_FROM_THREAD(thread_checker_);
}

ClientLayerTreeFrameSink::~ClientLayerTreeFrameSink() {}

bool ClientLayerTreeFrameSink::BindToClient(
    cc::LayerTreeFrameSinkClient* client) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  if (!cc::LayerTreeFrameSink::BindToClient(client))
    return false;

  DCHECK(pipes_.HasUnbound());
  if (pipes_.compositor_frame_sink_info.is_valid()) {
    compositor_frame_sink_.Bind(std::move(pipes_.compositor_frame_sink_info));
    compositor_frame_sink_.set_connection_error_with_reason_handler(
        base::Bind(&ClientLayerTreeFrameSink::OnMojoConnectionError,
                   weak_factory_.GetWeakPtr()));
    compositor_frame_sink_ptr_ = compositor_frame_sink_.get();
  } else if (pipes_.compositor_frame_sink_associated_info.is_valid()) {
    compositor_frame_sink_associated_.Bind(
        std::move(pipes_.compositor_frame_sink_associated_info));
    compositor_frame_sink_associated_.set_connection_error_with_reason_handler(
        base::Bind(&ClientLayerTreeFrameSink::OnMojoConnectionError,
                   weak_factory_.GetWeakPtr()));
    compositor_frame_sink_ptr_ = compositor_frame_sink_associated_.get();
  }
  client_binding_.Bind(std::move(pipes_.client_request),
                       compositor_task_runner_);

  if (synthetic_begin_frame_source_) {
    client->SetBeginFrameSource(synthetic_begin_frame_source_.get());
  } else {
    begin_frame_source_ = std::make_unique<ExternalBeginFrameSource>(this);
    begin_frame_source_->OnSetBeginFrameSourcePaused(begin_frames_paused_);
    client->SetBeginFrameSource(begin_frame_source_.get());
  }

  if (wants_animate_only_begin_frames_)
    compositor_frame_sink_->SetWantsAnimateOnlyBeginFrames();

  return true;
}

void ClientLayerTreeFrameSink::DetachFromClient() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  client_->SetBeginFrameSource(nullptr);
  begin_frame_source_.reset();
  synthetic_begin_frame_source_.reset();
  client_binding_.Close();
  compositor_frame_sink_.reset();
  compositor_frame_sink_associated_.reset();
  compositor_frame_sink_ptr_ = nullptr;
  cc::LayerTreeFrameSink::DetachFromClient();
}

void ClientLayerTreeFrameSink::SetLocalSurfaceId(
    const LocalSurfaceId& local_surface_id) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(local_surface_id.is_valid());
  DCHECK(enable_surface_synchronization_);
  local_surface_id_ = local_surface_id;
}

void ClientLayerTreeFrameSink::SubmitCompositorFrame(CompositorFrame frame) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(compositor_frame_sink_ptr_);
  DCHECK(frame.metadata.begin_frame_ack.has_damage);
  DCHECK_LE(BeginFrameArgs::kStartingFrameNumber,
            frame.metadata.begin_frame_ack.sequence_number);

  if (!enable_surface_synchronization_) {
    local_surface_id_ =
        local_surface_id_provider_->GetLocalSurfaceIdForFrame(frame);
  } else {
    if (local_surface_id_ == last_submitted_local_surface_id_) {
      CHECK_EQ(last_submitted_device_scale_factor_,
               frame.device_scale_factor());
      CHECK_EQ(last_submitted_size_in_pixels_.height(),
               frame.size_in_pixels().height());
      CHECK_EQ(last_submitted_size_in_pixels_.width(),
               frame.size_in_pixels().width());
    }
  }

  TRACE_EVENT_FLOW_BEGIN0(TRACE_DISABLED_BY_DEFAULT("cc.debug.ipc"),
                          "SubmitCompositorFrame", local_surface_id_.hash());
  bool tracing_enabled;
  TRACE_EVENT_CATEGORY_GROUP_ENABLED(TRACE_DISABLED_BY_DEFAULT("cc.debug.ipc"),
                                     &tracing_enabled);

  base::Optional<HitTestRegionList> hit_test_region_list;
  if (hit_test_data_provider_)
    hit_test_region_list = hit_test_data_provider_->GetHitTestData(frame);
  else
    hit_test_region_list = client_->BuildHitTestData();

  last_submitted_local_surface_id_ = local_surface_id_;
  last_submitted_device_scale_factor_ = frame.device_scale_factor();
  last_submitted_size_in_pixels_ = frame.size_in_pixels();

  compositor_frame_sink_ptr_->SubmitCompositorFrame(
      local_surface_id_, std::move(frame), std::move(hit_test_region_list),
      tracing_enabled ? base::TimeTicks::Now().since_origin().InMicroseconds()
                      : 0);
}

void ClientLayerTreeFrameSink::DidNotProduceFrame(const BeginFrameAck& ack) {
  DCHECK(compositor_frame_sink_ptr_);
  DCHECK(!ack.has_damage);
  DCHECK_LE(BeginFrameArgs::kStartingFrameNumber, ack.sequence_number);
  compositor_frame_sink_ptr_->DidNotProduceFrame(ack);
}

void ClientLayerTreeFrameSink::DidAllocateSharedBitmap(
    mojo::ScopedSharedBufferHandle buffer,
    const SharedBitmapId& id) {
  DCHECK(compositor_frame_sink_ptr_);
  compositor_frame_sink_ptr_->DidAllocateSharedBitmap(std::move(buffer), id);
}

void ClientLayerTreeFrameSink::DidDeleteSharedBitmap(const SharedBitmapId& id) {
  DCHECK(compositor_frame_sink_ptr_);
  compositor_frame_sink_ptr_->DidDeleteSharedBitmap(id);
}

void ClientLayerTreeFrameSink::DidReceiveCompositorFrameAck(
    const std::vector<ReturnedResource>& resources) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  client_->ReclaimResources(resources);
  client_->DidReceiveCompositorFrameAck();
}

void ClientLayerTreeFrameSink::DidPresentCompositorFrame(
    uint32_t presentation_token,
    base::TimeTicks time,
    base::TimeDelta refresh,
    uint32_t flags) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  client_->DidPresentCompositorFrame(presentation_token, time, refresh, flags);
}

void ClientLayerTreeFrameSink::DidDiscardCompositorFrame(
    uint32_t presentation_token) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  client_->DidDiscardCompositorFrame(presentation_token);
}

void ClientLayerTreeFrameSink::OnBeginFrame(const BeginFrameArgs& args) {
  if (!needs_begin_frames_) {
    // We had a race with SetNeedsBeginFrame(false) and still need to let the
    // sink know that we didn't use this BeginFrame.
    DidNotProduceFrame(
        BeginFrameAck(args.source_id, args.sequence_number, false));
  }
  if (begin_frame_source_)
    begin_frame_source_->OnBeginFrame(args);
}

void ClientLayerTreeFrameSink::OnBeginFramePausedChanged(bool paused) {
  begin_frames_paused_ = paused;
  if (begin_frame_source_)
    begin_frame_source_->OnSetBeginFrameSourcePaused(paused);
}

void ClientLayerTreeFrameSink::ReclaimResources(
    const std::vector<ReturnedResource>& resources) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  client_->ReclaimResources(resources);
}

void ClientLayerTreeFrameSink::OnNeedsBeginFrames(bool needs_begin_frames) {
  DCHECK(compositor_frame_sink_ptr_);
  needs_begin_frames_ = needs_begin_frames;
  compositor_frame_sink_ptr_->SetNeedsBeginFrame(needs_begin_frames);
}

void ClientLayerTreeFrameSink::OnMojoConnectionError(
    uint32_t custom_reason,
    const std::string& description) {
  if (custom_reason)
    DLOG(FATAL) << description;
  if (client_)
    client_->DidLoseLayerTreeFrameSink();
}

}  // namespace viz
