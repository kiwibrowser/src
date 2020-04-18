// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/picture_in_picture/overlay_surface_embedder.h"

#include "ui/compositor/layer.h"

namespace content {

OverlaySurfaceEmbedder::OverlaySurfaceEmbedder(OverlayWindow* window)
    : window_(window) {
  DCHECK(window_);
  video_layer_ = window_->GetVideoLayer();
  video_layer_->SetMasksToBounds(true);

  // The frame provided by the parent window's layer needs to show through
  // |video_layer_|.
  video_layer_->SetFillsBoundsOpaquely(false);
  // |video_layer_| bounds are set with the (0, 0) origin point. The
  // positioning of |window_| is dictated by itself.
  video_layer_->SetBounds(
      gfx::Rect(gfx::Point(0, 0), window_->GetBounds().size()));
  window_->GetLayer()->Add(video_layer_);

  AddControlsLayers();
}

OverlaySurfaceEmbedder::~OverlaySurfaceEmbedder() = default;

void OverlaySurfaceEmbedder::SetPrimarySurfaceId(
    const viz::SurfaceId& surface_id) {
  // SurfaceInfo has information about the embedded surface.
  video_layer_->SetShowPrimarySurface(
      surface_id, window_->GetBounds().size(), SK_ColorBLACK,
      cc::DeadlinePolicy::UseDefaultDeadline(),
      true /* stretch_content_to_fill_bounds */);
}

void OverlaySurfaceEmbedder::UpdateLayerBounds() {
  // Update the size and position of the video to stretch on the entire window.
  gfx::Size window_size = window_->GetBounds().size();
  gfx::Rect window_bounds = gfx::Rect(gfx::Point(0, 0), window_size);
  video_layer_->SetBounds(window_bounds);
  video_layer_->SetSurfaceSize(window_size);

  // Update the size and position of controls.
  controls_background_layer_->SetBounds(window_bounds);
  close_controls_layer_->SetBounds(window_->GetCloseControlsBounds());
  play_pause_controls_layer_->SetBounds(window_->GetPlayPauseControlsBounds());
}

void OverlaySurfaceEmbedder::AddControlsLayers() {
  // These control layers are expected to be set up by |window_|.
  controls_background_layer_ = window_->GetControlsBackgroundLayer();
  DCHECK(controls_background_layer_);
  controls_background_layer_->SetBounds(
      gfx::Rect(gfx::Point(0, 0), window_->GetBounds().size()));

  close_controls_layer_ = window_->GetCloseControlsLayer();
  DCHECK(close_controls_layer_);
  close_controls_layer_->SetFillsBoundsOpaquely(false);
  close_controls_layer_->SetBounds(window_->GetCloseControlsBounds());

  play_pause_controls_layer_ = window_->GetPlayPauseControlsLayer();
  DCHECK(play_pause_controls_layer_);
  play_pause_controls_layer_->SetFillsBoundsOpaquely(false);
  play_pause_controls_layer_->SetBounds(window_->GetPlayPauseControlsBounds());

  window_->GetLayer()->Add(controls_background_layer_);
  window_->GetLayer()->Add(close_controls_layer_);
  window_->GetLayer()->Add(play_pause_controls_layer_);
}

}  // namespace content
