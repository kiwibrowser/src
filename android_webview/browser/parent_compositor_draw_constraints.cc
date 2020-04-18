// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/parent_compositor_draw_constraints.h"

#include "android_webview/browser/child_frame.h"

namespace android_webview {

ParentCompositorDrawConstraints::ParentCompositorDrawConstraints()
    : is_layer(false), surface_rect_empty(false) {
}

ParentCompositorDrawConstraints::ParentCompositorDrawConstraints(
    bool is_layer,
    const gfx::Transform& transform,
    bool surface_rect_empty)
    : is_layer(is_layer),
      transform(transform),
      surface_rect_empty(surface_rect_empty) {}

bool ParentCompositorDrawConstraints::NeedUpdate(
    const ChildFrame& frame) const {
  if (is_layer != frame.is_layer ||
      transform != frame.transform_for_tile_priority) {
    return true;
  }

  // Viewport for tile priority does not depend on surface rect in this case.
  if (frame.offscreen_pre_raster || is_layer)
    return false;

  // Workaround for corner case. See crbug.com/417479.
  return frame.viewport_rect_for_tile_priority_empty && !surface_rect_empty;
}

}  // namespace android_webview
