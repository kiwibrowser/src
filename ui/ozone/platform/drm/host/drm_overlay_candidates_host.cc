// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/drm/host/drm_overlay_candidates_host.h"

#include <stddef.h>

#include <algorithm>

#include "ui/gfx/geometry/rect_conversions.h"
#include "ui/ozone/common/gpu/ozone_gpu_messages.h"
#include "ui/ozone/platform/drm/host/drm_overlay_manager.h"
#include "ui/ozone/platform/drm/host/drm_window_host.h"
#include "ui/ozone/public/overlay_manager_ozone.h"

namespace ui {

DrmOverlayCandidatesHost::DrmOverlayCandidatesHost(
    DrmOverlayManager* manager,
    gfx::AcceleratedWidget widget)
    : overlay_manager_(manager), widget_(widget) {}

DrmOverlayCandidatesHost::~DrmOverlayCandidatesHost() {}

void DrmOverlayCandidatesHost::CheckOverlaySupport(
    OverlaySurfaceCandidateList* candidates) {
  overlay_manager_->CheckOverlaySupport(candidates, widget_);
}


}  // namespace ui
