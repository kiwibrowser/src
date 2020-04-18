// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/drm/gpu/overlay_plane.h"

#include <stddef.h>

#include "ui/ozone/platform/drm/gpu/scanout_buffer.h"

namespace ui {

OverlayPlane::OverlayPlane(const scoped_refptr<ScanoutBuffer>& buffer,
                           gfx::GpuFence* gpu_fence)
    : buffer(buffer),
      plane_transform(gfx::OVERLAY_TRANSFORM_NONE),
      display_bounds(gfx::Point(), buffer->GetSize()),
      crop_rect(0, 0, 1, 1),
      enable_blend(false),
      gpu_fence(gpu_fence) {}

OverlayPlane::OverlayPlane(const scoped_refptr<ScanoutBuffer>& buffer,
                           int z_order,
                           gfx::OverlayTransform plane_transform,
                           const gfx::Rect& display_bounds,
                           const gfx::RectF& crop_rect,
                           bool enable_blend,
                           gfx::GpuFence* gpu_fence)
    : buffer(buffer),
      z_order(z_order),
      plane_transform(plane_transform),
      display_bounds(display_bounds),
      crop_rect(crop_rect),
      enable_blend(enable_blend),
      gpu_fence(gpu_fence) {}

OverlayPlane::OverlayPlane(const OverlayPlane& other) = default;

OverlayPlane::~OverlayPlane() {
}

bool OverlayPlane::operator<(const OverlayPlane& plane) const {
  return std::tie(z_order, display_bounds, crop_rect, plane_transform) <
         std::tie(plane.z_order, plane.display_bounds, plane.crop_rect,
                  plane.plane_transform);
}

// static
const OverlayPlane* OverlayPlane::GetPrimaryPlane(
    const OverlayPlaneList& overlays) {
  for (size_t i = 0; i < overlays.size(); ++i) {
    if (overlays[i].z_order == 0)
      return &overlays[i];
  }

  return nullptr;
}

}  // namespace ui
