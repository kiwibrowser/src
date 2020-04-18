// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/drm/host/drm_overlay_manager.h"

#include <stddef.h>

#include <algorithm>

#include "base/command_line.h"
#include "base/trace_event/trace_event.h"
#include "ui/gfx/geometry/rect_conversions.h"
#include "ui/ozone/platform/drm/common/drm_util.h"
#include "ui/ozone/platform/drm/host/drm_overlay_candidates_host.h"
#include "ui/ozone/platform/drm/host/drm_window_host.h"
#include "ui/ozone/platform/drm/host/drm_window_host_manager.h"
#include "ui/ozone/public/overlay_surface_candidate.h"
#include "ui/ozone/public/ozone_switches.h"

namespace ui {

using OverlaySurfaceCandidateList =
    OverlayCandidatesOzone::OverlaySurfaceCandidateList;

namespace {
const size_t kMaxCacheSize = 10;
// How many times a specific configuration of overlays should be requested
// before sending a GPU validation request to the GPU process.
const int kThrottleRequestSize = 3;
}  // namespace

DrmOverlayManager::DrmOverlayManager(GpuThreadAdapter* proxy,
                                     DrmWindowHostManager* window_manager)
    : proxy_(proxy), window_manager_(window_manager), cache_(kMaxCacheSize) {
  proxy_->RegisterHandlerForDrmOverlayManager(this);
}

DrmOverlayManager::~DrmOverlayManager() {
  proxy_->UnRegisterHandlerForDrmOverlayManager();
}

std::unique_ptr<OverlayCandidatesOzone>
DrmOverlayManager::CreateOverlayCandidates(gfx::AcceleratedWidget w) {
  return std::make_unique<DrmOverlayCandidatesHost>(this, w);
}

bool DrmOverlayManager::SupportsOverlays() const {
  return supports_overlays_;
}

void DrmOverlayManager::CheckOverlaySupport(
    OverlayCandidatesOzone::OverlaySurfaceCandidateList* candidates,
    gfx::AcceleratedWidget widget) {
  TRACE_EVENT0("hwoverlays", "DrmOverlayManager::CheckOverlaySupport");

  OverlaySurfaceCandidateList result_candidates;
  for (auto& candidate : *candidates) {
    // Reject candidates that don't fall on a pixel boundary.
    if (!gfx::IsNearestRectWithinDistance(candidate.display_rect, 0.01f)) {
      DCHECK(candidate.plane_z_order != 0);
      result_candidates.push_back(OverlaySurfaceCandidate());
      result_candidates.back().overlay_handled = false;
      continue;
    }

    result_candidates.push_back(OverlaySurfaceCandidate(candidate));
    // Start out hoping that we can have an overlay.
    result_candidates.back().overlay_handled = true;

    if (!CanHandleCandidate(candidate, widget)) {
      DCHECK(candidate.plane_z_order != 0);
      result_candidates.back().overlay_handled = false;
    }
  }

  size_t size = candidates->size();
  base::AutoLock lock(cache_lock_);
  auto iter = cache_.Get(result_candidates);
  if (iter == cache_.end()) {
    // We can skip GPU side validation in case all candidates are invalid.
    bool needs_gpu_validation = std::any_of(
        result_candidates.begin(), result_candidates.end(),
        [](OverlaySurfaceCandidate& c) { return c.overlay_handled; });
    OverlayValidationCacheValue value;
    value.request_num = 0;
    value.status.resize(result_candidates.size(), needs_gpu_validation
                                                      ? OVERLAY_STATUS_PENDING
                                                      : OVERLAY_STATUS_NOT);
    iter = cache_.Put(result_candidates, value);
  }

  OverlayValidationCacheValue& value = iter->second;
  if (value.request_num < kThrottleRequestSize) {
    value.request_num++;
  } else if (value.request_num == kThrottleRequestSize) {
    value.request_num++;
    if (value.status.back() == OVERLAY_STATUS_PENDING)
      SendOverlayValidationRequest(result_candidates, widget);
  } else {
    // We haven't received an answer yet.
    if (value.status.back() == OVERLAY_STATUS_PENDING)
      return;

    const std::vector<OverlayStatus>& status = value.status;
    DCHECK(size == status.size());
    for (size_t i = 0; i < size; i++) {
      DCHECK(status[i] == OVERLAY_STATUS_ABLE ||
             status[i] == OVERLAY_STATUS_NOT);
      candidates->at(i).overlay_handled = status[i] == OVERLAY_STATUS_ABLE;
    }
  }
}

void DrmOverlayManager::ResetCache() {
  base::AutoLock lock(cache_lock_);
  cache_.Clear();
}

DrmOverlayManager::OverlayValidationCacheValue::OverlayValidationCacheValue() =
    default;

DrmOverlayManager::OverlayValidationCacheValue::OverlayValidationCacheValue(
    const OverlayValidationCacheValue&) = default;
DrmOverlayManager::OverlayValidationCacheValue::~OverlayValidationCacheValue() =
    default;

void DrmOverlayManager::SendOverlayValidationRequest(
    const OverlaySurfaceCandidateList& candidates,
    gfx::AcceleratedWidget widget) const {
  if (!proxy_->IsConnected())
    return;
  TRACE_EVENT_ASYNC_BEGIN0(
      "hwoverlays", "DrmOverlayManager::SendOverlayValidationRequest", this);
  proxy_->GpuCheckOverlayCapabilities(widget, candidates);
}

void DrmOverlayManager::GpuSentOverlayResult(
    const gfx::AcceleratedWidget& widget,
    const OverlaySurfaceCandidateList& candidates,
    const OverlayStatusList& returns) {
  TRACE_EVENT_ASYNC_END0(
      "hwoverlays", "DrmOverlayManager::SendOverlayValidationRequest response",
      this);
  base::AutoLock lock(cache_lock_);
  auto iter = cache_.Peek(candidates);
  if (iter != cache_.end()) {
    iter->second.status = returns;
  }
}

bool DrmOverlayManager::CanHandleCandidate(
    const OverlaySurfaceCandidate& candidate,
    gfx::AcceleratedWidget widget) const {
  if (candidate.buffer_size.IsEmpty())
    return false;

  if (candidate.transform == gfx::OVERLAY_TRANSFORM_INVALID)
    return false;

  if (candidate.plane_z_order != 0) {
    // It is possible that the cc rect we get actually falls off the edge of
    // the screen. Usually this is prevented via things like status bars
    // blocking overlaying or cc clipping it, but in case it wasn't properly
    // clipped (since GL will render this situation fine) just ignore it
    // here. This should be an extremely rare occurrance.
    DrmWindowHost* window = window_manager_->GetWindow(widget);
    if (!window->GetBounds().Contains(
            gfx::ToNearestRect(candidate.display_rect))) {
      return false;
    }
  }

  if (candidate.is_clipped && !candidate.clip_rect.Contains(
                                  gfx::ToNearestRect(candidate.display_rect))) {
    return false;
  }

  return true;
}

}  // namespace ui
