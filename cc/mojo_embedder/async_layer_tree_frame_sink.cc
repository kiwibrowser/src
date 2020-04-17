// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/mojo_embedder/async_layer_tree_frame_sink.h"

#include <utility>

#include "base/bind.h"
#include "base/trace_event/trace_event.h"
#include "cc/trees/layer_tree_frame_sink_client.h"
#include "components/viz/client/hit_test_data_provider.h"
#include "components/viz/client/local_surface_id_provider.h"
#include "components/viz/common/frame_sinks/begin_frame_args.h"
#include "components/viz/common/hit_test/hit_test_region_list.h"
#include "components/viz/common/quads/compositor_frame.h"

#if defined(USE_EFL)
#include "gpu/command_buffer/client/gles2_interface.h"
#endif

namespace cc {
namespace mojo_embedder {

AsyncLayerTreeFrameSink::InitParams::InitParams() = default;
AsyncLayerTreeFrameSink::InitParams::~InitParams() = default;

AsyncLayerTreeFrameSink::UnboundMessagePipes::UnboundMessagePipes() = default;
AsyncLayerTreeFrameSink::UnboundMessagePipes::~UnboundMessagePipes() = default;

bool AsyncLayerTreeFrameSink::UnboundMessagePipes::HasUnbound() const {
  return client_request.is_pending() &&
         (compositor_frame_sink_info.is_valid() ^
          compositor_frame_sink_associated_info.is_valid());
}

AsyncLayerTreeFrameSink::UnboundMessagePipes::UnboundMessagePipes(
    UnboundMessagePipes&& other) = default;

AsyncLayerTreeFrameSink::AsyncLayerTreeFrameSink(
    scoped_refptr<viz::ContextProvider> context_provider,
    scoped_refptr<viz::RasterContextProvider> worker_context_provider,
    InitParams* params)
    : LayerTreeFrameSink(std::move(context_provider),
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

AsyncLayerTreeFrameSink::~AsyncLayerTreeFrameSink() {}

bool AsyncLayerTreeFrameSink::BindToClient(LayerTreeFrameSinkClient* client) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  if (!LayerTreeFrameSink::BindToClient(client))
    return false;

  DCHECK(pipes_.HasUnbound());
  if (pipes_.compositor_frame_sink_info.is_valid()) {
    compositor_frame_sink_.Bind(std::move(pipes_.compositor_frame_sink_info));
    compositor_frame_sink_.set_connection_error_with_reason_handler(
        base::BindOnce(&AsyncLayerTreeFrameSink::OnMojoConnectionError,
                       weak_factory_.GetWeakPtr()));
    compositor_frame_sink_ptr_ = compositor_frame_sink_.get();
  } else if (pipes_.compositor_frame_sink_associated_info.is_valid()) {
    compositor_frame_sink_associated_.Bind(
        std::move(pipes_.compositor_frame_sink_associated_info));
    compositor_frame_sink_associated_.set_connection_error_with_reason_handler(
        base::BindOnce(&AsyncLayerTreeFrameSink::OnMojoConnectionError,
                       weak_factory_.GetWeakPtr()));
    compositor_frame_sink_ptr_ = compositor_frame_sink_associated_.get();
  }
  client_binding_.Bind(std::move(pipes_.client_request),
                       compositor_task_runner_);

  if (synthetic_begin_frame_source_) {
    client->SetBeginFrameSource(synthetic_begin_frame_source_.get());
  } else {
    begin_frame_source_ = std::make_unique<viz::ExternalBeginFrameSource>(this);
    begin_frame_source_->OnSetBeginFrameSourcePaused(begin_frames_paused_);
    client->SetBeginFrameSource(begin_frame_source_.get());
  }

  if (wants_animate_only_begin_frames_)
    compositor_frame_sink_->SetWantsAnimateOnlyBeginFrames();

  return true;
}

void AsyncLayerTreeFrameSink::DetachFromClient() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  client_->SetBeginFrameSource(nullptr);
  begin_frame_source_.reset();
  synthetic_begin_frame_source_.reset();
  client_binding_.Close();
  compositor_frame_sink_.reset();
  compositor_frame_sink_associated_.reset();
  compositor_frame_sink_ptr_ = nullptr;
  LayerTreeFrameSink::DetachFromClient();
}

void AsyncLayerTreeFrameSink::SetLocalSurfaceId(
    const viz::LocalSurfaceId& local_surface_id) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(local_surface_id.is_valid());
  DCHECK(enable_surface_synchronization_);
  local_surface_id_ = local_surface_id;
}

void AsyncLayerTreeFrameSink::SubmitCompositorFrame(
    viz::CompositorFrame frame) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(compositor_frame_sink_ptr_);
  DCHECK(frame.metadata.begin_frame_ack.has_damage);
  DCHECK_LE(viz::BeginFrameArgs::kStartingFrameNumber,
            frame.metadata.begin_frame_ack.sequence_number);
  TRACE_EVENT_WITH_FLOW1(
      "viz,benchmark", "Graphics.Pipeline",
      TRACE_ID_GLOBAL(frame.metadata.begin_frame_ack.trace_id),
      TRACE_EVENT_FLAG_FLOW_IN | TRACE_EVENT_FLAG_FLOW_OUT, "step",
      "SubmitCompositorFrame");

  if (!enable_surface_synchronization_) {
    local_surface_id_ =
        local_surface_id_provider_->GetLocalSurfaceIdForFrame(frame);
  } else {
    if (local_surface_id_ == last_submitted_local_surface_id_) {
      DCHECK_EQ(last_submitted_device_scale_factor_,
                frame.device_scale_factor());
      DCHECK_EQ(last_submitted_size_in_pixels_.height(),
                frame.size_in_pixels().height());
      DCHECK_EQ(last_submitted_size_in_pixels_.width(),
                frame.size_in_pixels().width());
    }
  }

  TRACE_EVENT_FLOW_BEGIN0(TRACE_DISABLED_BY_DEFAULT("cc.debug.ipc"),
                          "SubmitCompositorFrame", local_surface_id_.hash());
  bool tracing_enabled;
  TRACE_EVENT_CATEGORY_GROUP_ENABLED(TRACE_DISABLED_BY_DEFAULT("cc.debug.ipc"),
                                     &tracing_enabled);

  base::Optional<viz::HitTestRegionList> hit_test_region_list;
  if (hit_test_data_provider_)
    hit_test_region_list = hit_test_data_provider_->GetHitTestData(frame);
  else
    hit_test_region_list = client_->BuildHitTestData();

  if (last_submitted_local_surface_id_ != local_surface_id_) {
    last_submitted_local_surface_id_ = local_surface_id_;
    last_submitted_device_scale_factor_ = frame.device_scale_factor();
    last_submitted_size_in_pixels_ = frame.size_in_pixels();

    TRACE_EVENT_WITH_FLOW2(
        TRACE_DISABLED_BY_DEFAULT("viz.surface_id_flow"),
        "LocalSurfaceId.Submission.Flow",
        TRACE_ID_GLOBAL(local_surface_id_.submission_trace_id()),
        TRACE_EVENT_FLAG_FLOW_IN | TRACE_EVENT_FLAG_FLOW_OUT, "step",
        "SubmitCompositorFrame", "surface_id", local_surface_id_.ToString());
  }

#if defined(USE_EFL)
  bool need_flush = true;
#if defined(OS_TIZEN)
  need_flush = !frame.can_skip_flush;
#endif
  auto* compositor_context_provider = context_provider();
  if (compositor_context_provider && need_flush) {
    compositor_context_provider->ContextGL()->Flush();
    compositor_context_provider->ContextGL()->GetError();
  }
#endif

  compositor_frame_sink_ptr_->SubmitCompositorFrame(
      local_surface_id_, std::move(frame), std::move(hit_test_region_list),
      tracing_enabled ? base::TimeTicks::Now().since_origin().InMicroseconds()
                      : 0);
}

void AsyncLayerTreeFrameSink::DidNotProduceFrame(
    const viz::BeginFrameAck& ack) {
  DCHECK(compositor_frame_sink_ptr_);
  DCHECK(!ack.has_damage);
  DCHECK_LE(viz::BeginFrameArgs::kStartingFrameNumber, ack.sequence_number);
  compositor_frame_sink_ptr_->DidNotProduceFrame(ack);
}

void AsyncLayerTreeFrameSink::DidAllocateSharedBitmap(
    mojo::ScopedSharedBufferHandle buffer,
    const viz::SharedBitmapId& id) {
  DCHECK(compositor_frame_sink_ptr_);
  compositor_frame_sink_ptr_->DidAllocateSharedBitmap(std::move(buffer), id);
}

void AsyncLayerTreeFrameSink::DidDeleteSharedBitmap(
    const viz::SharedBitmapId& id) {
  DCHECK(compositor_frame_sink_ptr_);
  compositor_frame_sink_ptr_->DidDeleteSharedBitmap(id);
}

void AsyncLayerTreeFrameSink::DidReceiveCompositorFrameAck(
    const std::vector<viz::ReturnedResource>& resources) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  client_->ReclaimResources(resources);
  client_->DidReceiveCompositorFrameAck();
}

void AsyncLayerTreeFrameSink::DidPresentCompositorFrame(
    uint32_t presentation_token,
    const gfx::PresentationFeedback& feedback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  client_->DidPresentCompositorFrame(presentation_token, feedback);
}

void AsyncLayerTreeFrameSink::OnBeginFrame(const viz::BeginFrameArgs& args) {
  if (!needs_begin_frames_) {
    TRACE_EVENT_WITH_FLOW1("viz,benchmark", "Graphics.Pipeline",
                           TRACE_ID_GLOBAL(args.trace_id),
                           TRACE_EVENT_FLAG_FLOW_IN | TRACE_EVENT_FLAG_FLOW_OUT,
                           "step", "ReceiveBeginFrameDiscard");
    // We had a race with SetNeedsBeginFrame(false) and still need to let the
    // sink know that we didn't use this BeginFrame.
    DidNotProduceFrame(viz::BeginFrameAck(args, false));
  } else {
    TRACE_EVENT_WITH_FLOW1("viz,benchmark", "Graphics.Pipeline",
                           TRACE_ID_GLOBAL(args.trace_id),
                           TRACE_EVENT_FLAG_FLOW_IN | TRACE_EVENT_FLAG_FLOW_OUT,
                           "step", "ReceiveBeginFrame");
  }
  if (begin_frame_source_)
    begin_frame_source_->OnBeginFrame(args);
}

void AsyncLayerTreeFrameSink::OnBeginFramePausedChanged(bool paused) {
  begin_frames_paused_ = paused;
  if (begin_frame_source_)
    begin_frame_source_->OnSetBeginFrameSourcePaused(paused);
}

void AsyncLayerTreeFrameSink::ReclaimResources(
    const std::vector<viz::ReturnedResource>& resources) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  client_->ReclaimResources(resources);
}

void AsyncLayerTreeFrameSink::OnNeedsBeginFrames(bool needs_begin_frames) {
  DCHECK(compositor_frame_sink_ptr_);
  needs_begin_frames_ = needs_begin_frames;
  compositor_frame_sink_ptr_->SetNeedsBeginFrame(needs_begin_frames);
}

void AsyncLayerTreeFrameSink::OnMojoConnectionError(
    uint32_t custom_reason,
    const std::string& description) {
  if (custom_reason)
    DLOG(FATAL) << description;
  if (client_)
    client_->DidLoseLayerTreeFrameSink();
}

}  // namespace mojo_embedder
}  // namespace cc
