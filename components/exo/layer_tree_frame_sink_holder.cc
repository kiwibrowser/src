// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/layer_tree_frame_sink_holder.h"

#include "ash/shell.h"
#include "base/threading/thread_task_runner_handle.h"
#include "cc/trees/layer_tree_frame_sink.h"
#include "components/exo/surface_tree_host.h"
#include "components/viz/common/hit_test/hit_test_region_list.h"
#include "components/viz/common/resources/returned_resource.h"

namespace exo {

////////////////////////////////////////////////////////////////////////////////
// LayerTreeFrameSinkHolder, public:

LayerTreeFrameSinkHolder::LayerTreeFrameSinkHolder(
    SurfaceTreeHost* surface_tree_host,
    std::unique_ptr<cc::LayerTreeFrameSink> frame_sink)
    : surface_tree_host_(surface_tree_host),
      frame_sink_(std::move(frame_sink)),
      weak_ptr_factory_(this) {
  frame_sink_->BindToClient(this);
}

LayerTreeFrameSinkHolder::~LayerTreeFrameSinkHolder() {
  if (frame_sink_)
    frame_sink_->DetachFromClient();

  for (auto& callback : release_callbacks_)
    std::move(callback.second).Run(gpu::SyncToken(), true /* lost */);

  if (shell_)
    shell_->RemoveShellObserver(this);
}

// static
void LayerTreeFrameSinkHolder::DeleteWhenLastResourceHasBeenReclaimed(
    std::unique_ptr<LayerTreeFrameSinkHolder> holder) {
  if (holder->last_frame_size_in_pixels_.IsEmpty()) {
    // Delete sink holder immediately if no frame has been submitted.
    DCHECK(holder->last_frame_resources_.empty());
    return;
  }

  // Submit an empty frame to ensure that pending release callbacks will be
  // processed in a finite amount of time.
  viz::CompositorFrame frame;
  frame.metadata.begin_frame_ack.source_id =
      viz::BeginFrameArgs::kManualSourceId;
  frame.metadata.begin_frame_ack.sequence_number =
      viz::BeginFrameArgs::kStartingFrameNumber;
  frame.metadata.begin_frame_ack.has_damage = true;
  frame.metadata.device_scale_factor = holder->last_frame_device_scale_factor_;
  std::unique_ptr<viz::RenderPass> pass = viz::RenderPass::Create();
  pass->SetNew(1, gfx::Rect(holder->last_frame_size_in_pixels_),
               gfx::Rect(holder->last_frame_size_in_pixels_), gfx::Transform());
  frame.render_pass_list.push_back(std::move(pass));
  holder->last_frame_resources_.clear();
  holder->frame_sink_->SubmitCompositorFrame(std::move(frame));

  // Delete sink holder immediately if not waiting for resources to be
  // reclaimed.
  if (holder->release_callbacks_.empty())
    return;

  ash::Shell* shell = ash::Shell::Get();
  holder->shell_ = shell;
  holder->surface_tree_host_ = nullptr;

  // If we have pending release callbacks then extend the lifetime of holder
  // by adding it as a shell observer. The holder will delete itself when shell
  // shuts down or when all pending release callbacks have been called.
  shell->AddShellObserver(holder.release());
}

void LayerTreeFrameSinkHolder::SubmitCompositorFrame(
    viz::CompositorFrame frame) {
  last_frame_size_in_pixels_ = frame.size_in_pixels();
  last_frame_device_scale_factor_ = frame.metadata.device_scale_factor;
  last_frame_resources_.clear();
  for (auto& resource : frame.resource_list)
    last_frame_resources_.push_back(resource.id);
  frame_sink_->SubmitCompositorFrame(std::move(frame));
}

void LayerTreeFrameSinkHolder::DidNotProduceFrame(
    const viz::BeginFrameAck& ack) {
  frame_sink_->DidNotProduceFrame(ack);
}

bool LayerTreeFrameSinkHolder::HasReleaseCallbackForResource(
    viz::ResourceId id) {
  return release_callbacks_.find(id) != release_callbacks_.end();
}

void LayerTreeFrameSinkHolder::SetResourceReleaseCallback(
    viz::ResourceId id,
    viz::ReleaseCallback callback) {
  DCHECK(!callback.is_null());
  release_callbacks_[id] = std::move(callback);
}

int LayerTreeFrameSinkHolder::AllocateResourceId() {
  return next_resource_id_++;
}

base::WeakPtr<LayerTreeFrameSinkHolder> LayerTreeFrameSinkHolder::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

////////////////////////////////////////////////////////////////////////////////
// cc::LayerTreeFrameSinkClient overrides:

void LayerTreeFrameSinkHolder::SetBeginFrameSource(
    viz::BeginFrameSource* source) {
  if (surface_tree_host_)
    surface_tree_host_->SetBeginFrameSource(source);
}

base::Optional<viz::HitTestRegionList>
LayerTreeFrameSinkHolder::BuildHitTestData() {
  return {};
}

void LayerTreeFrameSinkHolder::ReclaimResources(
    const std::vector<viz::ReturnedResource>& resources) {
  for (auto& resource : resources) {
    // Skip resources that are also in last frame. This can happen if
    // the frame sink id changed.
    if (std::find(last_frame_resources_.begin(), last_frame_resources_.end(),
                  resource.id) != last_frame_resources_.end()) {
      continue;
    }
    auto it = release_callbacks_.find(resource.id);
    DCHECK(it != release_callbacks_.end());
    if (it != release_callbacks_.end()) {
      std::move(it->second).Run(resource.sync_token, resource.lost);
      release_callbacks_.erase(it);
    }
  }

  if (shell_ && release_callbacks_.empty())
    ScheduleDelete();
}

void LayerTreeFrameSinkHolder::DidReceiveCompositorFrameAck() {
  if (surface_tree_host_)
    surface_tree_host_->DidReceiveCompositorFrameAck();
}

void LayerTreeFrameSinkHolder::DidPresentCompositorFrame(
    uint32_t presentation_token,
    base::TimeTicks time,
    base::TimeDelta refresh,
    uint32_t flags) {
  if (surface_tree_host_)
    surface_tree_host_->DidPresentCompositorFrame(presentation_token, time,
                                                  refresh, flags);
}

void LayerTreeFrameSinkHolder::DidDiscardCompositorFrame(
    uint32_t presentation_token) {
  if (surface_tree_host_)
    surface_tree_host_->DidDiscardCompositorFrame(presentation_token);
}

void LayerTreeFrameSinkHolder::DidLoseLayerTreeFrameSink() {
  last_frame_resources_.clear();
  for (auto& callback : release_callbacks_)
    std::move(callback.second).Run(gpu::SyncToken(), true /* lost */);
  release_callbacks_.clear();

  if (shell_)
    ScheduleDelete();
}

////////////////////////////////////////////////////////////////////////////////
// ash::ShellObserver overrides:

void LayerTreeFrameSinkHolder::OnShellDestroyed() {
  shell_->RemoveShellObserver(this);
  shell_ = nullptr;
  // Make sure frame sink never outlives the shell.
  frame_sink_->DetachFromClient();
  frame_sink_.reset();
  ScheduleDelete();
}

////////////////////////////////////////////////////////////////////////////////
// LayerTreeFrameSinkHolder, private:

void LayerTreeFrameSinkHolder::ScheduleDelete() {
  if (delete_pending_)
    return;
  delete_pending_ = true;
  base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE, this);
}

}  // namespace exo
