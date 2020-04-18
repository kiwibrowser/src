// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/surfaces/surface_manager.h"

#include <stddef.h>
#include <stdint.h>

#include <algorithm>
#include <utility>

#include "base/containers/adapters.h"
#include "base/containers/queue.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/time/default_tick_clock.h"
#include "components/viz/common/surfaces/parent_local_surface_id_allocator.h"
#include "components/viz/common/surfaces/surface_info.h"
#include "components/viz/service/surfaces/surface.h"
#include "components/viz/service/surfaces/surface_client.h"

#if DCHECK_IS_ON()
#include <sstream>
#endif

namespace viz {
namespace {

constexpr base::TimeDelta kExpireInterval = base::TimeDelta::FromSeconds(10);

const char kUmaRemovedTemporaryReference[] =
    "Compositing.SurfaceManager.RemovedTemporaryReference";

}  // namespace

SurfaceManager::SurfaceReferenceInfo::SurfaceReferenceInfo() = default;

SurfaceManager::SurfaceReferenceInfo::~SurfaceReferenceInfo() = default;

SurfaceManager::TemporaryReferenceData::TemporaryReferenceData() = default;

SurfaceManager::TemporaryReferenceData::~TemporaryReferenceData() = default;

SurfaceManager::SurfaceManager(
    base::Optional<uint32_t> activation_deadline_in_frames)
    : activation_deadline_in_frames_(activation_deadline_in_frames),
      dependency_tracker_(this),
      root_surface_id_(FrameSinkId(0u, 0u),
                       LocalSurfaceId(1u, base::UnguessableToken::Create())),
      tick_clock_(base::DefaultTickClock::GetInstance()),
      weak_factory_(this) {
  thread_checker_.DetachFromThread();

  // Android WebView doesn't have a task runner and doesn't need the timer.
  if (base::SequencedTaskRunnerHandle::IsSet())
    expire_timer_.emplace();
}

SurfaceManager::~SurfaceManager() {
  // Garbage collect surfaces here to avoid calling back into
  // SurfaceDependencyTracker as members of SurfaceManager are being
  // destroyed.
  temporary_references_.clear();
  temporary_reference_ranges_.clear();
  // Create a copy of the children set as RemoveSurfaceReferenceImpl below will
  // mutate that set.
  base::flat_set<SurfaceId> children(
      GetSurfacesReferencedByParent(root_surface_id_));
  for (const auto& child : children)
    RemoveSurfaceReferenceImpl(root_surface_id_, child);

  GarbageCollectSurfaces();

  // All SurfaceClients and their surfaces are supposed to be
  // destroyed before SurfaceManager.
  DCHECK(surface_map_.empty());
  DCHECK(surfaces_to_destroy_.empty());
}

#if DCHECK_IS_ON()
std::string SurfaceManager::SurfaceReferencesToString() {
  std::stringstream str;
  SurfaceReferencesToStringImpl(root_surface_id_, "", &str);
  // Temporary references will have an asterisk in front of them.
  for (auto& map_entry : temporary_references_)
    SurfaceReferencesToStringImpl(map_entry.first, "* ", &str);

  return str.str();
}
#endif

void SurfaceManager::SetActivationDeadlineInFramesForTesting(
    base::Optional<uint32_t> activation_deadline_in_frames) {
  activation_deadline_in_frames_ = activation_deadline_in_frames;
}

void SurfaceManager::SetTickClockForTesting(const base::TickClock* tick_clock) {
  tick_clock_ = tick_clock;
}

Surface* SurfaceManager::CreateSurface(
    base::WeakPtr<SurfaceClient> surface_client,
    const SurfaceInfo& surface_info,
    BeginFrameSource* begin_frame_source,
    bool needs_sync_tokens) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(surface_info.is_valid());
  DCHECK(surface_client);

  // If no surface with this SurfaceId exists, simply create the surface
  // and return.
  auto it = surface_map_.find(surface_info.id());
  if (it == surface_map_.end()) {
    std::unique_ptr<Surface> surface = std::make_unique<Surface>(
        surface_info, this, surface_client, needs_sync_tokens);
    // If no default deadline is specified then don't track deadlines.
    if (activation_deadline_in_frames_) {
      surface->SetDependencyDeadline(
          std::make_unique<SurfaceDependencyDeadline>(
              surface.get(), begin_frame_source, tick_clock_));
    }
    surface_map_[surface_info.id()] = std::move(surface);
    // We can get into a situation where multiple CompositorFrames arrive for a
    // FrameSink before the client can add any references for the frame. When
    // the second frame with a new size arrives, the first will be destroyed in
    // SurfaceFactory and then if there are no references it will be deleted
    // during surface GC. A temporary reference, removed when a real reference
    // is received, is added to prevent this from happening.
    AddTemporaryReference(surface_info.id());

    for (auto& observer : observer_list_)
      observer.OnSurfaceCreated(surface_info.id());
    return surface_map_[surface_info.id()].get();
  }

  // If a surface with this SurfaceId exists, it must be marked as
  // destroyed. Otherwise, we wouldn't receive a request to reuse the same
  // SurfaceId. Remove the surface out of the garbage collector's queue and
  // reuse it.
  Surface* surface = it->second.get();

  DCHECK(IsMarkedForDestruction(surface_info.id()));
  surfaces_to_destroy_.erase(surface_info.id());
  SurfaceDiscarded(surface);
  surface->Reset(surface_client);
  for (auto& observer : observer_list_)
    observer.OnSurfaceCreated(surface_info.id());
  return surface;
}

void SurfaceManager::DestroySurface(const SurfaceId& surface_id) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(surface_map_.count(surface_id));
  for (auto& observer : observer_list_)
    observer.OnSurfaceDestroyed(surface_id);
  surfaces_to_destroy_.insert(surface_id);
}

void SurfaceManager::RegisterFrameSinkId(const FrameSinkId& frame_sink_id) {
  bool inserted = valid_frame_sink_labels_.emplace(frame_sink_id, "").second;
  DCHECK(inserted);
}

void SurfaceManager::InvalidateFrameSinkId(const FrameSinkId& frame_sink_id) {
  valid_frame_sink_labels_.erase(frame_sink_id);

  // Remove any temporary references owned by |frame_sink_id|.
  std::vector<SurfaceId> temp_refs_to_clear;
  for (auto& map_entry : temporary_references_) {
    base::Optional<FrameSinkId>& owner = map_entry.second.owner;
    if (owner.has_value() && owner.value() == frame_sink_id)
      temp_refs_to_clear.push_back(map_entry.first);
  }

  for (auto& surface_id : temp_refs_to_clear)
    RemoveTemporaryReference(surface_id, RemovedReason::INVALIDATED);

  GarbageCollectSurfaces();
}

void SurfaceManager::SetFrameSinkDebugLabel(const FrameSinkId& frame_sink_id,
                                            const std::string& debug_label) {
  auto it = valid_frame_sink_labels_.find(frame_sink_id);
  DCHECK(it != valid_frame_sink_labels_.end());
  it->second = debug_label;
}

std::string SurfaceManager::GetFrameSinkDebugLabel(
    const FrameSinkId& frame_sink_id) const {
  auto it = valid_frame_sink_labels_.find(frame_sink_id);
  if (it != valid_frame_sink_labels_.end())
    return it->second;
  return std::string();
}

const SurfaceId& SurfaceManager::GetRootSurfaceId() const {
  return root_surface_id_;
}

std::vector<SurfaceId> SurfaceManager::GetCreatedSurfaceIds() const {
  std::vector<SurfaceId> surface_ids;
  for (auto& map_entry : surface_map_)
    surface_ids.push_back(map_entry.first);
  return surface_ids;
}

void SurfaceManager::AddSurfaceReferences(
    const std::vector<SurfaceReference>& references) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  for (const auto& reference : references)
    AddSurfaceReferenceImpl(reference.parent_id(), reference.child_id());
}

void SurfaceManager::RemoveSurfaceReferences(
    const std::vector<SurfaceReference>& references) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  for (const auto& reference : references)
    RemoveSurfaceReferenceImpl(reference.parent_id(), reference.child_id());
}

void SurfaceManager::AssignTemporaryReference(const SurfaceId& surface_id,
                                              const FrameSinkId& owner) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  if (!HasTemporaryReference(surface_id))
    return;

  temporary_references_[surface_id].owner = owner;
}

void SurfaceManager::DropTemporaryReference(const SurfaceId& surface_id) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  if (!HasTemporaryReference(surface_id))
    return;

  RemoveTemporaryReference(surface_id, RemovedReason::DROPPED);
}

void SurfaceManager::GarbageCollectSurfaces() {
  TRACE_EVENT0("viz", "SurfaceManager::GarbageCollectSurfaces");
  if (surfaces_to_destroy_.empty())
    return;

  SurfaceIdSet reachable_surfaces = GetLiveSurfacesForReferences();

  std::vector<SurfaceId> surfaces_to_delete;

  // Delete all destroyed and unreachable surfaces.
  for (auto iter = surfaces_to_destroy_.begin();
       iter != surfaces_to_destroy_.end();) {
    if (reachable_surfaces.count(*iter) == 0) {
      surfaces_to_delete.push_back(*iter);
      iter = surfaces_to_destroy_.erase(iter);
    } else {
      ++iter;
    }
  }

  // ~Surface() draw callback could modify |surfaces_to_destroy_|.
  for (const SurfaceId& surface_id : surfaces_to_delete)
    DestroySurfaceInternal(surface_id);
}

const base::flat_set<SurfaceId>& SurfaceManager::GetSurfacesReferencedByParent(
    const SurfaceId& surface_id) const {
  auto iter = references_.find(surface_id);
  if (iter == references_.end())
    return empty_surface_id_set_;
  return iter->second.children;
}

const base::flat_set<SurfaceId>& SurfaceManager::GetSurfacesThatReferenceChild(
    const SurfaceId& surface_id) const {
  auto iter = references_.find(surface_id);
  if (iter == references_.end())
    return empty_surface_id_set_;
  return iter->second.parents;
}

SurfaceManager::SurfaceIdSet SurfaceManager::GetLiveSurfacesForReferences() {
  SurfaceIdSet reachable_surfaces;

  // Walk down from the root and mark each SurfaceId we encounter as
  // reachable.
  base::queue<SurfaceId> surface_queue;
  surface_queue.push(root_surface_id_);

  // All surfaces not marked for destruction are reachable.
  for (auto& map_entry : surface_map_) {
    if (!IsMarkedForDestruction(map_entry.first)) {
      reachable_surfaces.insert(map_entry.first);
      surface_queue.push(map_entry.first);
    }
  }

  // All surfaces with temporary references are also reachable.
  for (auto& map_entry : temporary_references_) {
    const SurfaceId& surface_id = map_entry.first;
    if (reachable_surfaces.insert(surface_id).second) {
      surface_queue.push(surface_id);
    }
  }

  while (!surface_queue.empty()) {
    const auto& children = GetSurfacesReferencedByParent(surface_queue.front());
    for (const SurfaceId& child_id : children) {
      // Check for cycles when inserting into |reachable_surfaces|.
      if (reachable_surfaces.insert(child_id).second)
        surface_queue.push(child_id);
    }
    surface_queue.pop();
  }

  return reachable_surfaces;
}

void SurfaceManager::AddSurfaceReferenceImpl(const SurfaceId& parent_id,
                                             const SurfaceId& child_id) {
  if (parent_id.frame_sink_id() == child_id.frame_sink_id()) {
    DLOG(ERROR) << "Cannot add self reference from " << parent_id << " to "
                << child_id;
    return;
  }

  // We trust that |parent_id| either exists or is about to exist, since is not
  // sent over IPC. We don't trust |child_id|, since it is sent over IPC.
  if (surface_map_.count(child_id) == 0) {
    DLOG(ERROR) << "No surface in map for " << child_id.ToString();
    return;
  }

  references_[parent_id].children.insert(child_id);
  references_[child_id].parents.insert(parent_id);

  for (auto& observer : observer_list_)
    observer.OnAddedSurfaceReference(parent_id, child_id);

  if (HasTemporaryReference(child_id))
    RemoveTemporaryReference(child_id, RemovedReason::EMBEDDED);
}

void SurfaceManager::RemoveSurfaceReferenceImpl(const SurfaceId& parent_id,
                                                const SurfaceId& child_id) {
  auto iter_parent = references_.find(parent_id);
  auto iter_child = references_.find(child_id);
  if (iter_parent == references_.end() || iter_child == references_.end())
    return;

  for (auto& observer : observer_list_)
    observer.OnRemovedSurfaceReference(parent_id, child_id);

  iter_parent->second.children.erase(child_id);
  iter_child->second.parents.erase(parent_id);
}

void SurfaceManager::RemoveAllSurfaceReferences(const SurfaceId& surface_id) {
  DCHECK(!HasTemporaryReference(surface_id));

  auto iter = references_.find(surface_id);
  if (iter != references_.end()) {
    // Remove all references from |surface_id| to a child surface.
    for (const SurfaceId& child_id : iter->second.children)
      references_[child_id].parents.erase(surface_id);

    // Remove all reference from parent surface to |surface_id|.
    for (const SurfaceId& parent_id : iter->second.parents)
      references_[parent_id].children.erase(surface_id);

    references_.erase(iter);
  }
}

bool SurfaceManager::HasTemporaryReference(const SurfaceId& surface_id) const {
  return temporary_references_.count(surface_id) != 0;
}

void SurfaceManager::AddTemporaryReference(const SurfaceId& surface_id) {
  DCHECK(!HasTemporaryReference(surface_id));

  // Add an entry to |temporary_references_| with no owner for the temporary
  // reference. Also add a range tracking entry so we know the order that
  // surfaces were created for the FrameSinkId.
  temporary_references_[surface_id].owner = base::Optional<FrameSinkId>();
  temporary_reference_ranges_[surface_id.frame_sink_id()].push_back(
      surface_id.local_surface_id());

  // Start timer to expire temporary references if it's not running.
  if (expire_timer_ && !expire_timer_->IsRunning()) {
    expire_timer_->Start(FROM_HERE, kExpireInterval, this,
                         &SurfaceManager::ExpireOldTemporaryReferences);
  }
}

void SurfaceManager::RemoveTemporaryReference(const SurfaceId& surface_id,
                                              RemovedReason reason) {
  DCHECK(HasTemporaryReference(surface_id));

  const FrameSinkId& frame_sink_id = surface_id.frame_sink_id();
  std::vector<LocalSurfaceId>& frame_sink_temp_refs =
      temporary_reference_ranges_[frame_sink_id];

  // If the temporary reference to |surface_id| is being removed because it was
  // embedded, then remove older temporary references with the same FrameSinkId.
  const bool remove_older = (reason == RemovedReason::EMBEDDED);

  // Find the iterator to the range tracking entry for |surface_id|. Use that
  // iterator and |remove_older| to find the right begin and end iterators for
  // the temporary references we want to remove.
  auto surface_id_iter =
      std::find(frame_sink_temp_refs.begin(), frame_sink_temp_refs.end(),
                surface_id.local_surface_id());
  auto begin_iter =
      remove_older ? frame_sink_temp_refs.begin() : surface_id_iter;
  auto end_iter = surface_id_iter + 1;

  // Remove temporary references and range tracking information.
  for (auto iter = begin_iter; iter != end_iter; ++iter) {
    temporary_references_.erase(SurfaceId(frame_sink_id, *iter));

    // If removing more than the temporary reference to |surface_id| then the
    // reason for removing others is because they are being skipped.
    const bool was_skipped = (*iter != surface_id.local_surface_id());
    UMA_HISTOGRAM_ENUMERATION(kUmaRemovedTemporaryReference,
                              was_skipped ? RemovedReason::SKIPPED : reason,
                              RemovedReason::COUNT);
  }
  frame_sink_temp_refs.erase(begin_iter, end_iter);

  // If last temporary reference is removed for |frame_sink_id| then cleanup
  // range tracking map entry.
  if (frame_sink_temp_refs.empty())
    temporary_reference_ranges_.erase(frame_sink_id);

  // Stop the timer if there are no temporary references that could expire.
  if (temporary_references_.empty()) {
    if (expire_timer_ && expire_timer_->IsRunning())
      expire_timer_->Stop();
  }
}

Surface* SurfaceManager::GetLatestInFlightSurface(
    const SurfaceId& primary_surface_id,
    const SurfaceId& fallback_surface_id) {
  // The fallback surface must exist before we begin looking for more recent
  // surfaces. This guarantees that the |parent| is allowed to embed the
  // |fallback_surface_id| and is not guessing surface IDs.
  Surface* fallback_surface = GetSurfaceForId(fallback_surface_id);
  if (!fallback_surface || !fallback_surface->HasActiveFrame())
    return nullptr;

  // If the FrameSinkId of the primary and the fallback surface IDs do not
  // match then we should avoid presenting a fallback newer than intended by
  // |parent|.
  if (primary_surface_id.frame_sink_id() != fallback_surface_id.frame_sink_id())
    return fallback_surface;

  auto it =
      temporary_reference_ranges_.find(fallback_surface_id.frame_sink_id());
  if (it == temporary_reference_ranges_.end())
    return fallback_surface;

  const std::vector<LocalSurfaceId>& temp_surfaces = it->second;
  for (const LocalSurfaceId& local_surface_id : base::Reversed(temp_surfaces)) {
    // The in-flight surface must be older than the primary surface ID.
    if (local_surface_id == primary_surface_id.local_surface_id() ||
        local_surface_id.parent_sequence_number() >
            primary_surface_id.local_surface_id().parent_sequence_number() ||
        local_surface_id.child_sequence_number() >
            primary_surface_id.local_surface_id().child_sequence_number()) {
      continue;
    }

    // If the embed_token doesn't match the primary or fallback's then the
    // parent does not have permission to embed this surface.
    if (local_surface_id.embed_token() !=
            fallback_surface_id.local_surface_id().embed_token() &&
        local_surface_id.embed_token() !=
            primary_surface_id.local_surface_id().embed_token()) {
      continue;
    }

    SurfaceId surface_id(fallback_surface_id.frame_sink_id(), local_surface_id);
    Surface* surface = GetSurfaceForId(surface_id);
    if (surface && surface->HasActiveFrame())
      return surface;
  }

  return fallback_surface;
}

void SurfaceManager::ExpireOldTemporaryReferences() {
  if (temporary_references_.empty())
    return;

  std::vector<SurfaceId> temporary_references_to_delete;
  for (auto& map_entry : temporary_references_) {
    const SurfaceId& surface_id = map_entry.first;
    TemporaryReferenceData& data = map_entry.second;
    if (data.marked_as_old) {
      // The temporary reference has existed for more than 10 seconds, a surface
      // reference should have replaced it by now. To avoid permanently leaking
      // memory delete the temporary reference.
      DLOG(ERROR) << "Old/orphaned temporary reference to " << surface_id;
      temporary_references_to_delete.push_back(surface_id);
    } else if (IsMarkedForDestruction(surface_id)) {
      // Never mark live surfaces as old, they can't be garbage collected.
      data.marked_as_old = true;
    }
  }

  for (auto& surface_id : temporary_references_to_delete)
    RemoveTemporaryReference(surface_id, RemovedReason::EXPIRED);
}

Surface* SurfaceManager::GetSurfaceForId(const SurfaceId& surface_id) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  auto it = surface_map_.find(surface_id);
  if (it == surface_map_.end())
    return nullptr;
  return it->second.get();
}

bool SurfaceManager::SurfaceModified(const SurfaceId& surface_id,
                                     const BeginFrameAck& ack) {
  CHECK(thread_checker_.CalledOnValidThread());
  bool changed = false;
  for (auto& observer : observer_list_)
    changed |= observer.OnSurfaceDamaged(surface_id, ack);
  return changed;
}

void SurfaceManager::FirstSurfaceActivation(const SurfaceInfo& surface_info) {
  CHECK(thread_checker_.CalledOnValidThread());

  for (auto& observer : observer_list_)
    observer.OnFirstSurfaceActivation(surface_info);
}

void SurfaceManager::SurfaceActivated(
    Surface* surface,
    base::Optional<base::TimeDelta> duration) {
  // Trigger a display frame if necessary.
  const CompositorFrame& frame = surface->GetActiveFrame();
  if (!SurfaceModified(surface->surface_id(), frame.metadata.begin_frame_ack)) {
    TRACE_EVENT_INSTANT0("cc", "Damage not visible.", TRACE_EVENT_SCOPE_THREAD);
    surface->RunDrawCallback();
  }

  for (auto& observer : observer_list_)
    observer.OnSurfaceActivated(surface->surface_id(), duration);

  dependency_tracker_.OnSurfaceActivated(surface);
}

void SurfaceManager::SurfaceDependenciesChanged(
    Surface* surface,
    const base::flat_set<FrameSinkId>& added_dependencies,
    const base::flat_set<FrameSinkId>& removed_dependencies) {
  dependency_tracker_.OnSurfaceDependenciesChanged(surface, added_dependencies,
                                                   removed_dependencies);
}

void SurfaceManager::SurfaceDiscarded(Surface* surface) {
  for (auto& observer : observer_list_)
    observer.OnSurfaceDiscarded(surface->surface_id());
  dependency_tracker_.OnSurfaceDiscarded(surface);
}

void SurfaceManager::SurfaceDamageExpected(const SurfaceId& surface_id,
                                           const BeginFrameArgs& args) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  for (auto& observer : observer_list_)
    observer.OnSurfaceDamageExpected(surface_id, args);
}

void SurfaceManager::DestroySurfaceInternal(const SurfaceId& surface_id) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  auto it = surface_map_.find(surface_id);
  DCHECK(it != surface_map_.end());
  // Make sure that the surface is removed from the map before being actually
  // destroyed. An ack could be sent during the destruction of a surface which
  // could trigger a synchronous frame submission to a half-destroyed surface
  // and that's not desirable.
  std::unique_ptr<Surface> doomed = std::move(it->second);
  surface_map_.erase(it);
  RemoveAllSurfaceReferences(surface_id);
}

#if DCHECK_IS_ON()
void SurfaceManager::SurfaceReferencesToStringImpl(const SurfaceId& surface_id,
                                                   std::string indent,
                                                   std::stringstream* str) {
  *str << indent;
  // Print the current line for |surface_id|.
  Surface* surface = GetSurfaceForId(surface_id);
  if (surface) {
    *str << surface->surface_id().ToString();

    std::string frame_sink_label =
        GetFrameSinkDebugLabel(surface_id.frame_sink_id());
    if (!frame_sink_label.empty())
      *str << " " << frame_sink_label;
    *str << (IsMarkedForDestruction(surface_id) ? " destroyed" : " live");

    if (surface->HasPendingFrame()) {
      // This provides the surface size from the root render pass.
      const CompositorFrame& frame = surface->GetPendingFrame();
      *str << " pending " << frame.size_in_pixels().ToString();
    }

    if (surface->HasActiveFrame()) {
      // This provides the surface size from the root render pass.
      const CompositorFrame& frame = surface->GetActiveFrame();
      *str << " active " << frame.size_in_pixels().ToString();
    }
  } else {
    *str << surface_id;
  }
  *str << "\n";

  // If the current surface has references to children, sort children and print
  // references for each child.
  for (const SurfaceId& child_id : GetSurfacesReferencedByParent(surface_id))
    SurfaceReferencesToStringImpl(child_id, indent + "  ", str);
}
#endif  // DCHECK_IS_ON()

bool SurfaceManager::IsMarkedForDestruction(const SurfaceId& surface_id) {
  return surfaces_to_destroy_.count(surface_id) != 0;
}

void SurfaceManager::SurfaceWillBeDrawn(Surface* surface) {
  for (auto& observer : observer_list_)
    observer.OnSurfaceWillBeDrawn(surface);
}

}  // namespace viz
