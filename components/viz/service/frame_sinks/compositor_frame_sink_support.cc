// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/frame_sinks/compositor_frame_sink_support.h"

#include <algorithm>
#include <utility>

#include "components/viz/common/frame_sinks/begin_frame_source.h"
#include "components/viz/common/quads/compositor_frame.h"
#include "components/viz/common/surfaces/surface_info.h"
#include "components/viz/service/display/display.h"
#include "components/viz/service/display_embedder/server_shared_bitmap_manager.h"
#include "components/viz/service/frame_sinks/frame_sink_manager_impl.h"
#include "components/viz/service/surfaces/surface.h"
#include "components/viz/service/surfaces/surface_reference.h"
#include "mojo/public/cpp/system/platform_handle.h"

namespace viz {

CompositorFrameSinkSupport::CompositorFrameSinkSupport(
    mojom::CompositorFrameSinkClient* client,
    FrameSinkManagerImpl* frame_sink_manager,
    const FrameSinkId& frame_sink_id,
    bool is_root,
    bool needs_sync_tokens)
    : client_(client),
      frame_sink_manager_(frame_sink_manager),
      surface_manager_(frame_sink_manager->surface_manager()),
      frame_sink_id_(frame_sink_id),
      surface_resource_holder_(this),
      is_root_(is_root),
      needs_sync_tokens_(needs_sync_tokens),
      allow_copy_output_requests_(is_root),
      weak_factory_(this) {
  // This may result in SetBeginFrameSource() being called.
  frame_sink_manager_->RegisterCompositorFrameSinkSupport(frame_sink_id_, this);
}

CompositorFrameSinkSupport::~CompositorFrameSinkSupport() {
  if (!destruction_callback_.is_null())
    std::move(destruction_callback_).Run();

  // Unregister |this| as a BeginFrameObserver so that the
  // BeginFrameSource does not call into |this| after it's deleted.
  callback_received_begin_frame_ = true;
  callback_received_receive_ack_ = true;
  SetNeedsBeginFrame(false);

  // For display root surfaces the surface is no longer going to be visible.
  // Make it unreachable from the top-level root.
  if (referenced_local_surface_id_.has_value()) {
    auto reference = MakeTopLevelRootReference(
        SurfaceId(frame_sink_id_, referenced_local_surface_id_.value()));
    surface_manager_->RemoveSurfaceReferences({reference});
  }

  EvictLastActivatedSurface();
  if (last_created_surface_id_.is_valid())
    surface_manager_->DestroySurface(last_created_surface_id_);
  frame_sink_manager_->UnregisterCompositorFrameSinkSupport(frame_sink_id_);

  // The display compositor has ownership of shared memory for each
  // SharedBitmapId that has been reported from the client. Since the client is
  // gone that memory can be freed. If we don't then it would leak.
  for (const auto& id : owned_bitmaps_)
    ServerSharedBitmapManager::current()->ChildDeletedSharedBitmap(id);

  // No video capture clients should remain after calling
  // UnregisterCompositorFrameSinkSupport().
  DCHECK(capture_clients_.empty());
}

void CompositorFrameSinkSupport::SetUpHitTest(
    LatestLocalSurfaceIdLookupDelegate* local_surface_id_lookup_delegate) {
  DCHECK(is_root_);
  hit_test_aggregator_ = std::make_unique<HitTestAggregator>(
      frame_sink_manager_->hit_test_manager(), frame_sink_manager_,
      local_surface_id_lookup_delegate, frame_sink_id_);
}

void CompositorFrameSinkSupport::SetAggregatedDamageCallbackForTesting(
    AggregatedDamageCallback callback) {
  aggregated_damage_callback_ = std::move(callback);
}

void CompositorFrameSinkSupport::SetDestructionCallback(
    base::OnceClosure callback) {
  destruction_callback_ = std::move(callback);
}

void CompositorFrameSinkSupport::SetBeginFrameSource(
    BeginFrameSource* begin_frame_source) {
  if (begin_frame_source_ && added_frame_observer_) {
    begin_frame_source_->RemoveObserver(this);
    added_frame_observer_ = false;
  }
  begin_frame_source_ = begin_frame_source;
  UpdateNeedsBeginFramesInternal();
}

void CompositorFrameSinkSupport::OnSurfaceActivated(Surface* surface) {
  DCHECK(surface);
  DCHECK(surface->HasActiveFrame());
  if (last_activated_surface_id_ != surface->surface_id()) {
    if (last_activated_surface_id_.is_valid()) {
      const LocalSurfaceId& local_surface_id =
          surface->surface_id().local_surface_id();
      const LocalSurfaceId& last_activated_local_surface_id =
          last_activated_surface_id_.local_surface_id();
      CHECK_GE(local_surface_id.parent_sequence_number(),
               last_activated_local_surface_id.parent_sequence_number());
      CHECK_GE(local_surface_id.child_sequence_number(),
               last_activated_local_surface_id.child_sequence_number());
      CHECK(local_surface_id.parent_sequence_number() >
                last_activated_local_surface_id.parent_sequence_number() ||
            local_surface_id.child_sequence_number() >
                last_activated_local_surface_id.child_sequence_number());
      Surface* prev_surface =
          surface_manager_->GetSurfaceForId(last_activated_surface_id_);
      DCHECK(prev_surface);
      surface->SetPreviousFrameSurface(prev_surface);
      surface_manager_->DestroySurface(prev_surface->surface_id());
    }
    last_activated_surface_id_ = surface->surface_id();
  }

  DCHECK(surface->active_referenced_surfaces());
  UpdateSurfaceReferences(surface->surface_id().local_surface_id(),
                          *surface->active_referenced_surfaces());
  uint32_t frame_token = surface->GetActiveFrame().metadata.frame_token;
  if (frame_token)
    frame_sink_manager_->OnFrameTokenChanged(frame_sink_id_, frame_token);
}

void CompositorFrameSinkSupport::OnSurfaceDiscarded(Surface* surface) {
  if (surface->surface_id() == last_activated_surface_id_)
    last_activated_surface_id_ = SurfaceId();

  if (surface->surface_id() == last_created_surface_id_)
    last_created_surface_id_ = SurfaceId();
}

void CompositorFrameSinkSupport::RefResources(
    const std::vector<TransferableResource>& resources) {
  surface_resource_holder_.RefResources(resources);
}

void CompositorFrameSinkSupport::UnrefResources(
    const std::vector<ReturnedResource>& resources) {
  surface_resource_holder_.UnrefResources(resources);
}

void CompositorFrameSinkSupport::ReturnResources(
    const std::vector<ReturnedResource>& resources) {
  if (resources.empty())
    return;
  if (!ack_pending_count_ && client_) {
    client_->ReclaimResources(resources);
    return;
  }

  std::copy(resources.begin(), resources.end(),
            std::back_inserter(surface_returned_resources_));
}

void CompositorFrameSinkSupport::ReceiveFromChild(
    const std::vector<TransferableResource>& resources) {
  surface_resource_holder_.ReceiveFromChild(resources);
}

std::vector<std::unique_ptr<CopyOutputRequest>>
CompositorFrameSinkSupport::TakeCopyOutputRequests(
    const LocalSurfaceId& latest_local_id) {
  std::vector<std::unique_ptr<CopyOutputRequest>> results;
  for (auto it = copy_output_requests_.begin();
       it != copy_output_requests_.end();) {
    // Requests with a non-valid local id should be satisfied as soon as
    // possible.
    if (!it->first.is_valid() || it->first <= latest_local_id) {
      results.push_back(std::move(it->second));
      it = copy_output_requests_.erase(it);
    } else {
      ++it;
    }
  }
  return results;
}

void CompositorFrameSinkSupport::EvictLastActivatedSurface() {
  if (!last_activated_surface_id_.is_valid())
    return;

  SurfaceId to_destroy_surface_id = last_activated_surface_id_;
  if (last_created_surface_id_ == last_activated_surface_id_)
    last_created_surface_id_ = SurfaceId();
  last_activated_surface_id_ = SurfaceId();
  surface_manager_->DestroySurface(to_destroy_surface_id);

  // For display root surfaces the surface is no longer going to be visible.
  // Make it unreachable from the top-level root.
  if (referenced_local_surface_id_.has_value()) {
    auto reference = MakeTopLevelRootReference(
        SurfaceId(frame_sink_id_, referenced_local_surface_id_.value()));
    surface_manager_->RemoveSurfaceReferences({reference});
    referenced_local_surface_id_.reset();
  }
}

void CompositorFrameSinkSupport::SetNeedsBeginFrame(bool needs_begin_frame) {
  client_needs_begin_frame_ = needs_begin_frame;
  UpdateNeedsBeginFramesInternal();
}

void CompositorFrameSinkSupport::SetWantsAnimateOnlyBeginFrames() {
  wants_animate_only_begin_frames_ = true;
}

bool CompositorFrameSinkSupport::WantsAnimateOnlyBeginFrames() const {
  return wants_animate_only_begin_frames_;
}

void CompositorFrameSinkSupport::DidNotProduceFrame(const BeginFrameAck& ack) {
  TRACE_EVENT2("viz", "CompositorFrameSinkSupport::DidNotProduceFrame",
               "ack.source_id", ack.source_id, "ack.sequence_number",
               ack.sequence_number);
  DCHECK_GE(ack.sequence_number, BeginFrameArgs::kStartingFrameNumber);

  // Override the has_damage flag (ignoring invalid data from clients).
  BeginFrameAck modified_ack(ack);
  modified_ack.has_damage = false;

  if (last_activated_surface_id_.is_valid())
    surface_manager_->SurfaceModified(last_activated_surface_id_, modified_ack);

  if (begin_frame_source_)
    begin_frame_source_->DidFinishFrame(this);
}

void CompositorFrameSinkSupport::SubmitCompositorFrame(
    const LocalSurfaceId& local_surface_id,
    CompositorFrame frame,
    base::Optional<HitTestRegionList> hit_test_region_list,
    uint64_t submit_time) {
  const auto result = MaybeSubmitCompositorFrame(
      local_surface_id, std::move(frame), std::move(hit_test_region_list),
      mojom::CompositorFrameSink::SubmitCompositorFrameSyncCallback());
  DCHECK_EQ(result, ACCEPTED);
}

bool CompositorFrameSinkSupport::DidAllocateSharedBitmap(
    mojo::ScopedSharedBufferHandle buffer,
    const SharedBitmapId& id) {
  if (!ServerSharedBitmapManager::current()->ChildAllocatedSharedBitmap(
          std::move(buffer), id))
    return false;

  owned_bitmaps_.insert(id);
  return true;
}

void CompositorFrameSinkSupport::DidDeleteSharedBitmap(
    const SharedBitmapId& id) {
  ServerSharedBitmapManager::current()->ChildDeletedSharedBitmap(id);
  owned_bitmaps_.erase(id);
}

CompositorFrameSinkSupport::SubmitResult
CompositorFrameSinkSupport::MaybeSubmitCompositorFrame(
    const LocalSurfaceId& local_surface_id,
    CompositorFrame frame,
    base::Optional<HitTestRegionList> hit_test_region_list,
    mojom::CompositorFrameSink::SubmitCompositorFrameSyncCallback callback) {
  TRACE_EVENT1("viz", "CompositorFrameSinkSupport::MaybeSubmitCompositorFrame",
               "FrameSinkId", frame_sink_id_.ToString());
  DCHECK(local_surface_id.is_valid());
  DCHECK(!frame.render_pass_list.empty());
  DCHECK(!frame.size_in_pixels().IsEmpty());

  CHECK(callback_received_begin_frame_);
  CHECK(callback_received_receive_ack_);

  compositor_frame_callback_ = std::move(callback);
  if (compositor_frame_callback_) {
    callback_received_begin_frame_ = false;
    callback_received_receive_ack_ = false;
    UpdateNeedsBeginFramesInternal();
  }

  // Ensure no CopyOutputRequests have been submitted if they are banned.
  if (!allow_copy_output_requests_ && frame.HasCopyOutputRequests()) {
    TRACE_EVENT_INSTANT0("viz", "CopyOutputRequests not allowed",
                         TRACE_EVENT_SCOPE_THREAD);
    return COPY_OUTPUT_REQUESTS_NOT_ALLOWED;
  }

  uint64_t frame_index = ++last_frame_index_;
  ++ack_pending_count_;

  // Override the has_damage flag (ignoring invalid data from clients).
  frame.metadata.begin_frame_ack.has_damage = true;
  BeginFrameAck ack = frame.metadata.begin_frame_ack;
  DCHECK_LE(BeginFrameArgs::kStartingFrameNumber, ack.sequence_number);

  if (!ui::LatencyInfo::Verify(frame.metadata.latency_info,
                               "RenderWidgetHostImpl::OnSwapCompositorFrame")) {
    std::vector<ui::LatencyInfo>().swap(frame.metadata.latency_info);
  }
  for (ui::LatencyInfo& latency : frame.metadata.latency_info) {
    if (latency.latency_components().size() > 0) {
      latency.AddLatencyNumber(ui::DISPLAY_COMPOSITOR_RECEIVED_FRAME_COMPONENT,
                               0);
    }
  }

  Surface* prev_surface =
      surface_manager_->GetSurfaceForId(last_created_surface_id_);
  Surface* current_surface = nullptr;
  if (prev_surface &&
      local_surface_id == last_created_surface_id_.local_surface_id()) {
    current_surface = prev_surface;
  } else {
    SurfaceId surface_id(frame_sink_id_, local_surface_id);
    SurfaceInfo surface_info(surface_id, frame.device_scale_factor(),
                             frame.size_in_pixels());

    // LocalSurfaceIds should be monotonically increasing. This ID is used
    // to determine the freshness of a surface at aggregation time.
    const LocalSurfaceId& last_created_local_surface_id =
        last_created_surface_id_.local_surface_id();
    // Neither sequence numbers of the LocalSurfaceId can decrease and at least
    // one must increase.
    bool monotonically_increasing_id =
        (local_surface_id.parent_sequence_number() >=
             last_created_local_surface_id.parent_sequence_number() &&
         local_surface_id.child_sequence_number() >=
             last_created_local_surface_id.child_sequence_number()) &&
        (local_surface_id.parent_sequence_number() >
             last_created_local_surface_id.parent_sequence_number() ||
         local_surface_id.child_sequence_number() >
             last_created_local_surface_id.child_sequence_number());

    if (!surface_info.is_valid() || !monotonically_increasing_id) {
      TRACE_EVENT_INSTANT0("viz", "Surface Invariants Violation",
                           TRACE_EVENT_SCOPE_THREAD);
      std::vector<ReturnedResource> resources =
          TransferableResource::ReturnResources(frame.resource_list);
      ReturnResources(resources);
      DidReceiveCompositorFrameAck();
      if (frame.metadata.presentation_token) {
        DidPresentCompositorFrame(frame.metadata.presentation_token,
                                  base::TimeTicks(), base::TimeDelta(), 0);
      }
      return SURFACE_INVARIANTS_VIOLATION;
    }

    current_surface = CreateSurface(surface_info);
    last_created_surface_id_ = SurfaceId(frame_sink_id_, local_surface_id);
    surface_manager_->SurfaceDamageExpected(current_surface->surface_id(),
                                            last_begin_frame_args_);
  }

  // QueueFrame can fail in unit tests, so SubmitHitTestRegionList has to be
  // called before that.
  frame_sink_manager()->SubmitHitTestRegionList(
      last_created_surface_id_, frame_index, std::move(hit_test_region_list));

  bool result = current_surface->QueueFrame(
      std::move(frame), frame_index,
      base::BindOnce(&CompositorFrameSinkSupport::DidReceiveCompositorFrameAck,
                     weak_factory_.GetWeakPtr()),
      base::BindRepeating(&CompositorFrameSinkSupport::OnAggregatedDamage,
                          weak_factory_.GetWeakPtr()),
      frame.metadata.presentation_token
          ? base::BindOnce(
                &CompositorFrameSinkSupport::DidPresentCompositorFrame,
                weak_factory_.GetWeakPtr(), frame.metadata.presentation_token)
          : Surface::PresentedCallback());
  if (!result) {
    TRACE_EVENT_INSTANT0("viz", "QueueFrame failed", TRACE_EVENT_SCOPE_THREAD);
    return SURFACE_INVARIANTS_VIOLATION;
  }

  if (begin_frame_source_)
    begin_frame_source_->DidFinishFrame(this);

  return ACCEPTED;
}

void CompositorFrameSinkSupport::UpdateSurfaceReferences(
    const LocalSurfaceId& local_surface_id,
    const std::vector<SurfaceId>& active_referenced_surfaces) {
  SurfaceId surface_id(frame_sink_id_, local_surface_id);

  const base::flat_set<SurfaceId>& existing_referenced_surfaces =
      surface_manager_->GetSurfacesReferencedByParent(surface_id);

  base::flat_set<SurfaceId> new_referenced_surfaces(
      active_referenced_surfaces.begin(), active_referenced_surfaces.end(),
      base::KEEP_FIRST_OF_DUPES);

  // Populate list of surface references to add and remove by getting the
  // difference between existing surface references and surface references for
  // latest activated CompositorFrame.
  std::vector<SurfaceReference> references_to_add;
  std::vector<SurfaceReference> references_to_remove;
  GetSurfaceReferenceDifference(surface_id, existing_referenced_surfaces,
                                new_referenced_surfaces, &references_to_add,
                                &references_to_remove);

  // Check if this is a display root surface and the SurfaceId is changing.
  if (is_root_ && (!referenced_local_surface_id_.has_value() ||
                   referenced_local_surface_id_.value() != local_surface_id)) {
    // Make the new SurfaceId reachable from the top-level root.
    references_to_add.push_back(MakeTopLevelRootReference(surface_id));

    // Make the old SurfaceId unreachable from the top-level root if applicable.
    if (referenced_local_surface_id_.has_value()) {
      references_to_remove.push_back(MakeTopLevelRootReference(
          SurfaceId(frame_sink_id_, referenced_local_surface_id_.value())));
    }

    referenced_local_surface_id_ = local_surface_id;
  }

  // Modify surface references stored in SurfaceManager.
  if (!references_to_add.empty())
    surface_manager_->AddSurfaceReferences(references_to_add);
  if (!references_to_remove.empty())
    surface_manager_->RemoveSurfaceReferences(references_to_remove);
}

SurfaceReference CompositorFrameSinkSupport::MakeTopLevelRootReference(
    const SurfaceId& surface_id) {
  return SurfaceReference(surface_manager_->GetRootSurfaceId(), surface_id);
}

void CompositorFrameSinkSupport::HandleCallback() {
  if (!compositor_frame_callback_ || !callback_received_begin_frame_ ||
      !callback_received_receive_ack_) {
    return;
  }

  std::move(compositor_frame_callback_)
      .Run(std::move(surface_returned_resources_));
  surface_returned_resources_.clear();
}

void CompositorFrameSinkSupport::DidReceiveCompositorFrameAck() {
  DCHECK_GT(ack_pending_count_, 0);
  ack_pending_count_--;
  if (!client_)
    return;

  // If we have a callback, we only return the resource on onBeginFrame.
  if (compositor_frame_callback_) {
    callback_received_receive_ack_ = true;
    UpdateNeedsBeginFramesInternal();
    HandleCallback();
    return;
  }

  client_->DidReceiveCompositorFrameAck(surface_returned_resources_);
  surface_returned_resources_.clear();
}

void CompositorFrameSinkSupport::DidPresentCompositorFrame(
    uint32_t presentation_token,
    base::TimeTicks time,
    base::TimeDelta refresh,
    uint32_t flags) {
  DCHECK(presentation_token);
  if (client_) {
    if (time != base::TimeTicks()) {
      client_->DidPresentCompositorFrame(presentation_token, time, refresh,
                                         flags);
    } else {
      client_->DidDiscardCompositorFrame(presentation_token);
    }
  }
}

void CompositorFrameSinkSupport::OnBeginFrame(const BeginFrameArgs& args) {
  if (last_activated_surface_id_.is_valid())
    surface_manager_->SurfaceDamageExpected(last_activated_surface_id_, args);
  last_begin_frame_args_ = args;

  if (compositor_frame_callback_) {
    callback_received_begin_frame_ = true;
    UpdateNeedsBeginFramesInternal();
    HandleCallback();
  }

  if (client_ && client_needs_begin_frame_)
    client_->OnBeginFrame(args);
}

const BeginFrameArgs& CompositorFrameSinkSupport::LastUsedBeginFrameArgs()
    const {
  return last_begin_frame_args_;
}

void CompositorFrameSinkSupport::OnBeginFrameSourcePausedChanged(bool paused) {
  if (client_)
    client_->OnBeginFramePausedChanged(paused);
}

void CompositorFrameSinkSupport::UpdateNeedsBeginFramesInternal() {
  if (!begin_frame_source_)
    return;

  // We require a begin frame if there's a callback pending, or if the client
  // requested it.
  bool needs_begin_frame =
      client_needs_begin_frame_ ||
      (compositor_frame_callback_ && !callback_received_begin_frame_);

  if (needs_begin_frame == added_frame_observer_)
    return;

  added_frame_observer_ = needs_begin_frame;
  if (needs_begin_frame)
    begin_frame_source_->AddObserver(this);
  else
    begin_frame_source_->RemoveObserver(this);
}

Surface* CompositorFrameSinkSupport::CreateSurface(
    const SurfaceInfo& surface_info) {
  return surface_manager_->CreateSurface(
      weak_factory_.GetWeakPtr(), surface_info,
      frame_sink_manager_->GetPrimaryBeginFrameSource(), needs_sync_tokens_);
}

void CompositorFrameSinkSupport::AttachCaptureClient(
    CapturableFrameSink::Client* client) {
  DCHECK(std::find(capture_clients_.begin(), capture_clients_.end(), client) ==
         capture_clients_.end());
  capture_clients_.push_back(client);
}

void CompositorFrameSinkSupport::DetachCaptureClient(
    CapturableFrameSink::Client* client) {
  const auto it =
      std::find(capture_clients_.begin(), capture_clients_.end(), client);
  if (it != capture_clients_.end())
    capture_clients_.erase(it);
}

gfx::Size CompositorFrameSinkSupport::GetActiveFrameSize() {
  if (last_activated_surface_id_.is_valid()) {
    Surface* current_surface =
        surface_manager_->GetSurfaceForId(last_activated_surface_id_);
    DCHECK(current_surface);
    if (current_surface->HasActiveFrame()) {
      DCHECK(current_surface->GetActiveFrame().size_in_pixels() ==
             current_surface->size_in_pixels());
      return current_surface->size_in_pixels();
    }
  }
  return gfx::Size();
}

void CompositorFrameSinkSupport::RequestCopyOfOutput(
    const LocalSurfaceId& local_surface_id,
    std::unique_ptr<CopyOutputRequest> copy_request) {
  copy_output_requests_.push_back(
      std::make_pair(local_surface_id, std::move(copy_request)));
  if (last_activated_surface_id_.is_valid()) {
    BeginFrameAck ack;
    ack.has_damage = true;
    surface_manager_->SurfaceModified(last_activated_surface_id_, ack);
  }
}

const CompositorFrameMetadata*
CompositorFrameSinkSupport::GetLastActivatedFrameMetadata() {
  if (!last_activated_surface_id_.is_valid())
    return nullptr;
  Surface* surface =
      surface_manager_->GetSurfaceForId(last_activated_surface_id_);
  DCHECK(surface);
  return &surface->GetActiveFrame().metadata;
}

HitTestAggregator* CompositorFrameSinkSupport::GetHitTestAggregator() {
  DCHECK(is_root_);
  return hit_test_aggregator_.get();
}

Surface* CompositorFrameSinkSupport::GetLastCreatedSurfaceForTesting() {
  return surface_manager_->GetSurfaceForId(last_created_surface_id_);
}

// static
const char* CompositorFrameSinkSupport::GetSubmitResultAsString(
    SubmitResult result) {
  switch (result) {
    case CompositorFrameSinkSupport::ACCEPTED:
      return "Accepted";
    case CompositorFrameSinkSupport::COPY_OUTPUT_REQUESTS_NOT_ALLOWED:
      return "CopyOutputRequests not allowed";
    case CompositorFrameSinkSupport::SURFACE_INVARIANTS_VIOLATION:
      return "Surface invariants violation";
  }
  NOTREACHED();
  return nullptr;
}

void CompositorFrameSinkSupport::OnAggregatedDamage(
    const LocalSurfaceId& local_surface_id,
    const CompositorFrame& frame,
    const gfx::Rect& damage_rect,
    base::TimeTicks expected_display_time) const {
  DCHECK(!damage_rect.IsEmpty());

  const gfx::Size& frame_size_in_pixels = frame.size_in_pixels();
  if (aggregated_damage_callback_) {
    aggregated_damage_callback_.Run(local_surface_id, frame_size_in_pixels,
                                    damage_rect, expected_display_time);
  }

  for (CapturableFrameSink::Client* client : capture_clients_) {
    client->OnFrameDamaged(frame_size_in_pixels, damage_rect,
                           expected_display_time, frame.metadata);
  }
}

}  // namespace viz
