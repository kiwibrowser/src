// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/frame_metadata_util.h"

#include "components/viz/common/quads/compositor_frame_metadata.h"

namespace {

// Used to accomodate finite precision when comparing scaled viewport and
// content widths. While this value may seem large, width=device-width on an N7
// V1 saw errors of ~0.065 between computed window and content widths.
const float kMobileViewportWidthEpsilon = 0.15f;

bool HasFixedPageScale(const viz::CompositorFrameMetadata& frame_metadata) {
  return frame_metadata.min_page_scale_factor ==
         frame_metadata.max_page_scale_factor;
}

bool HasMobileViewport(const viz::CompositorFrameMetadata& frame_metadata) {
  float window_width_dip =
      frame_metadata.page_scale_factor *
          frame_metadata.scrollable_viewport_size.width();
  float content_width_css = frame_metadata.root_layer_size.width();
  return content_width_css <= window_width_dip + kMobileViewportWidthEpsilon;
}

}  // namespace

namespace content {

bool IsMobileOptimizedFrame(
    const viz::CompositorFrameMetadata& frame_metadata) {
  bool has_mobile_viewport = HasMobileViewport(frame_metadata);
  bool has_fixed_page_scale = HasFixedPageScale(frame_metadata);
  return has_fixed_page_scale || has_mobile_viewport;
}

}  // namespace content
