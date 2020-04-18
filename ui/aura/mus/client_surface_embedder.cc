// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/mus/client_surface_embedder.h"

#include "ui/aura/window.h"
#include "ui/gfx/geometry/dip_util.h"

namespace aura {

ClientSurfaceEmbedder::ClientSurfaceEmbedder(
    Window* window,
    bool inject_gutter,
    const gfx::Insets& client_area_insets)
    : window_(window),
      inject_gutter_(inject_gutter),
      client_area_insets_(client_area_insets) {
  surface_layer_ = std::make_unique<ui::Layer>(ui::LAYER_TEXTURED);
  surface_layer_->SetMasksToBounds(true);
  // The frame provided by the parent window->layer() needs to show through
  // the surface layer.
  surface_layer_->SetFillsBoundsOpaquely(false);

  window_->layer()->Add(surface_layer_.get());

  // Window's layer may contain content from this client (the embedder), e.g.
  // this is the case with window decorations provided by Window Manager.
  // This content should appear underneath the content of the embedded client.
  window_->layer()->StackAtTop(surface_layer_.get());
}

ClientSurfaceEmbedder::~ClientSurfaceEmbedder() = default;

void ClientSurfaceEmbedder::SetPrimarySurfaceId(
    const viz::SurfaceId& surface_id) {
  surface_layer_->SetShowPrimarySurface(
      surface_id, window_->bounds().size(), SK_ColorWHITE,
      cc::DeadlinePolicy::UseDefaultDeadline(),
      false /* stretch_content_to_fill_bounds */);
}

void ClientSurfaceEmbedder::SetFallbackSurfaceInfo(
    const viz::SurfaceInfo& surface_info) {
  fallback_surface_info_ = surface_info;
  surface_layer_->SetFallbackSurfaceId(surface_info.id());
  UpdateSizeAndGutters();
}

void ClientSurfaceEmbedder::UpdateSizeAndGutters() {
  surface_layer_->SetBounds(gfx::Rect(window_->bounds().size()));
  if (!inject_gutter_)
    return;

  gfx::Size fallback_surface_size_in_dip;
  if (fallback_surface_info_.is_valid()) {
    float fallback_device_scale_factor =
        fallback_surface_info_.device_scale_factor();
    fallback_surface_size_in_dip = gfx::ConvertSizeToDIP(
        fallback_device_scale_factor, fallback_surface_info_.size_in_pixels());
  }
  gfx::Rect window_bounds(window_->bounds());
  if (!window_->transparent() &&
      fallback_surface_size_in_dip.width() < window_bounds.width()) {
    right_gutter_ = std::make_unique<ui::Layer>(ui::LAYER_SOLID_COLOR);
    // TODO(fsamuel): Use the embedded client's background color.
    right_gutter_->SetColor(SK_ColorWHITE);
    int width = window_bounds.width() - fallback_surface_size_in_dip.width();
    // The right gutter also includes the bottom-right corner, if necessary.
    int height = window_bounds.height() - client_area_insets_.height();
    right_gutter_->SetBounds(gfx::Rect(
        client_area_insets_.left() + fallback_surface_size_in_dip.width(),
        client_area_insets_.top(), width, height));
    window_->layer()->Add(right_gutter_.get());
  } else {
    right_gutter_.reset();
  }

  // Only create a bottom gutter if a fallback surface is available. Otherwise,
  // the right gutter will fill the whole window until a fallback is available.
  if (!window_->transparent() && !fallback_surface_size_in_dip.IsEmpty() &&
      fallback_surface_size_in_dip.height() < window_bounds.height()) {
    bottom_gutter_ = std::make_unique<ui::Layer>(ui::LAYER_SOLID_COLOR);
    // TODO(fsamuel): Use the embedded client's background color.
    bottom_gutter_->SetColor(SK_ColorWHITE);
    int width = fallback_surface_size_in_dip.width();
    int height = window_bounds.height() - fallback_surface_size_in_dip.height();
    bottom_gutter_->SetBounds(
        gfx::Rect(0, fallback_surface_size_in_dip.height(), width, height));
    window_->layer()->Add(bottom_gutter_.get());
  } else {
    bottom_gutter_.reset();
  }
  window_->layer()->StackAtTop(surface_layer_.get());
}

const viz::SurfaceId& ClientSurfaceEmbedder::GetPrimarySurfaceIdForTesting()
    const {
  return *surface_layer_->GetPrimarySurfaceId();
}

}  // namespace aura
