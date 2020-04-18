// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/surfaces/surface_dependency_tracker.h"

#include "components/viz/common/surfaces/surface_info.h"
#include "components/viz/service/surfaces/surface.h"
#include "components/viz/service/surfaces/surface_manager.h"

namespace viz {

SurfaceDependencyTracker::SurfaceDependencyTracker(
    SurfaceManager* surface_manager)
    : surface_manager_(surface_manager) {}

SurfaceDependencyTracker::~SurfaceDependencyTracker() = default;

void SurfaceDependencyTracker::RequestSurfaceResolution(Surface* surface) {
  DCHECK(surface->HasPendingFrame());

  const CompositorFrame& pending_frame = surface->GetPendingFrame();

  if (IsSurfaceLate(surface)) {
    ActivateLateSurfaceSubtree(surface);
    return;
  }

  // Activation dependencies that aren't currently known to the surface manager
  // or do not have an active CompositorFrame block this frame.
  for (const SurfaceId& surface_id :
       pending_frame.metadata.activation_dependencies) {
    Surface* dependency = surface_manager_->GetSurfaceForId(surface_id);
    if (!dependency || !dependency->HasActiveFrame()) {
      blocked_surfaces_from_dependency_[surface_id.frame_sink_id()].insert(
          surface->surface_id());
    }
  }

  UpdateSurfaceDeadline(surface);
}

void SurfaceDependencyTracker::OnSurfaceActivated(Surface* surface) {
  if (!surface->late_activation_dependencies().empty())
    surfaces_with_missing_dependencies_.insert(surface->surface_id());
  else
    surfaces_with_missing_dependencies_.erase(surface->surface_id());
  NotifySurfaceIdAvailable(surface->surface_id());
}

void SurfaceDependencyTracker::OnSurfaceDependenciesChanged(
    Surface* surface,
    const base::flat_set<FrameSinkId>& added_dependencies,
    const base::flat_set<FrameSinkId>& removed_dependencies) {
  // Update the |blocked_surfaces_from_dependency_| map with the changes in
  // dependencies.
  for (const FrameSinkId& frame_sink_id : added_dependencies) {
    blocked_surfaces_from_dependency_[frame_sink_id].insert(
        surface->surface_id());
  }

  for (const FrameSinkId& frame_sink_id : removed_dependencies) {
    auto it = blocked_surfaces_from_dependency_.find(frame_sink_id);
    it->second.erase(surface->surface_id());
    if (it->second.empty())
      blocked_surfaces_from_dependency_.erase(it);
  }
}

void SurfaceDependencyTracker::OnSurfaceDiscarded(Surface* surface) {
  surfaces_with_missing_dependencies_.erase(surface->surface_id());

  // If the surface being destroyed doesn't have a pending frame then we have
  // nothing to do here.
  if (!surface->HasPendingFrame())
    return;

  const CompositorFrame& pending_frame = surface->GetPendingFrame();

  DCHECK(!pending_frame.metadata.activation_dependencies.empty());

  for (const SurfaceId& surface_id :
       pending_frame.metadata.activation_dependencies) {
    auto it =
        blocked_surfaces_from_dependency_.find(surface_id.frame_sink_id());
    if (it == blocked_surfaces_from_dependency_.end())
      continue;

    auto& blocked_surface_ids = it->second;
    auto blocked_surface_ids_it =
        blocked_surface_ids.find(surface->surface_id());
    if (blocked_surface_ids_it != blocked_surface_ids.end()) {
      blocked_surface_ids.erase(surface->surface_id());
      if (blocked_surface_ids.empty())
        blocked_surfaces_from_dependency_.erase(surface_id.frame_sink_id());
    }
  }

  // Pretend that the discarded surface's SurfaceId is now available to
  // unblock dependencies because we now know the surface will never activate.
  NotifySurfaceIdAvailable(surface->surface_id());
}

void SurfaceDependencyTracker::ActivateLateSurfaceSubtree(Surface* surface) {
  DCHECK(surface->HasPendingFrame());

  const CompositorFrame& pending_frame = surface->GetPendingFrame();

  for (const SurfaceId& surface_id :
       pending_frame.metadata.activation_dependencies) {
    Surface* dependency = surface_manager_->GetSurfaceForId(surface_id);
    if (dependency && dependency->HasPendingFrame())
      ActivateLateSurfaceSubtree(dependency);
  }

  surface->ActivatePendingFrameForDeadline(base::nullopt);
}

void SurfaceDependencyTracker::UpdateSurfaceDeadline(Surface* surface) {
  DCHECK(surface->HasPendingFrame());

  const CompositorFrame& pending_frame = surface->GetPendingFrame();

  // Inherit the deadline from the first parent blocked on this surface.
  auto it = blocked_surfaces_from_dependency_.find(
      surface->surface_id().frame_sink_id());
  if (it != blocked_surfaces_from_dependency_.end()) {
    const base::flat_set<SurfaceId>& dependent_parent_ids = it->second;
    for (const SurfaceId& parent_id : dependent_parent_ids) {
      Surface* parent = surface_manager_->GetSurfaceForId(parent_id);
      if (parent && parent->has_deadline() &&
          parent->activation_dependencies().count(surface->surface_id())) {
        surface->InheritActivationDeadlineFrom(parent);
        break;
      }
    }
  }

  DCHECK(!surface_manager_->activation_deadline_in_frames() ||
         surface->has_deadline());

  // Recursively propagate the newly set deadline to children.
  for (const SurfaceId& surface_id :
       pending_frame.metadata.activation_dependencies) {
    Surface* dependency = surface_manager_->GetSurfaceForId(surface_id);
    if (dependency && dependency->HasPendingFrame())
      UpdateSurfaceDeadline(dependency);
  }
}

bool SurfaceDependencyTracker::IsSurfaceLate(Surface* surface) {
  for (const SurfaceId& surface_id : surfaces_with_missing_dependencies_) {
    Surface* activated_surface = surface_manager_->GetSurfaceForId(surface_id);
    DCHECK(activated_surface->HasActiveFrame());
    if (activated_surface->late_activation_dependencies().count(
            surface->surface_id())) {
      return true;
    }
  }
  return false;
}

void SurfaceDependencyTracker::NotifySurfaceIdAvailable(
    const SurfaceId& surface_id) {
  auto it = blocked_surfaces_from_dependency_.find(surface_id.frame_sink_id());
  if (it == blocked_surfaces_from_dependency_.end())
    return;

  // Unblock surfaces that depend on this |surface_id|.
  base::flat_set<SurfaceId> blocked_surfaces_by_id(it->second);

  // Tell each surface about the availability of its blocker.
  for (const SurfaceId& blocked_surface_by_id : blocked_surfaces_by_id) {
    Surface* blocked_surface =
        surface_manager_->GetSurfaceForId(blocked_surface_by_id);
    if (!blocked_surface) {
      // A blocked surface may have been garbage collected during dependency
      // resolution.
      continue;
    }
    blocked_surface->NotifySurfaceIdAvailable(surface_id);
  }
}

}  // namespace viz
