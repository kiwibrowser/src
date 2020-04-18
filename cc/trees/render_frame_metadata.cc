// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/trees/render_frame_metadata.h"

namespace cc {

RenderFrameMetadata::RenderFrameMetadata() = default;

RenderFrameMetadata::RenderFrameMetadata(const RenderFrameMetadata& other) =
    default;

RenderFrameMetadata::RenderFrameMetadata(RenderFrameMetadata&& other) = default;

RenderFrameMetadata::~RenderFrameMetadata() {}

// static
bool RenderFrameMetadata::HasAlwaysUpdateMetadataChanged(
    const RenderFrameMetadata& rfm1,
    const RenderFrameMetadata& rfm2) {
  return rfm1.root_background_color != rfm2.root_background_color ||
         rfm1.is_scroll_offset_at_top != rfm2.is_scroll_offset_at_top ||
         rfm1.selection != rfm2.selection ||
         rfm1.is_mobile_optimized != rfm2.is_mobile_optimized ||
         rfm1.device_scale_factor != rfm2.device_scale_factor ||
         rfm1.viewport_size_in_pixels != rfm2.viewport_size_in_pixels ||
         rfm1.local_surface_id != rfm2.local_surface_id ||
         rfm1.top_controls_height != rfm2.top_controls_height ||
         rfm1.top_controls_shown_ratio != rfm2.top_controls_shown_ratio ||
         rfm1.bottom_controls_height != rfm2.bottom_controls_height ||
         rfm1.bottom_controls_shown_ratio != rfm2.bottom_controls_shown_ratio;
}

RenderFrameMetadata& RenderFrameMetadata::operator=(
    const RenderFrameMetadata&) = default;

RenderFrameMetadata& RenderFrameMetadata::operator=(
    RenderFrameMetadata&& other) = default;

bool RenderFrameMetadata::operator==(const RenderFrameMetadata& other) const {
  return root_scroll_offset == other.root_scroll_offset &&
         root_background_color == other.root_background_color &&
         is_scroll_offset_at_top == other.is_scroll_offset_at_top &&
         selection == other.selection &&
         is_mobile_optimized == other.is_mobile_optimized &&
         device_scale_factor == other.device_scale_factor &&
         viewport_size_in_pixels == other.viewport_size_in_pixels &&
         local_surface_id == other.local_surface_id &&
         top_controls_height == other.top_controls_height &&
         top_controls_shown_ratio == other.top_controls_shown_ratio &&
         bottom_controls_height == other.bottom_controls_height &&
         bottom_controls_shown_ratio == other.bottom_controls_shown_ratio;
}

bool RenderFrameMetadata::operator!=(const RenderFrameMetadata& other) const {
  return !operator==(other);
}

}  // namespace cc
