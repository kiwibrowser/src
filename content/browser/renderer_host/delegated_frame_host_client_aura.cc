// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/delegated_frame_host_client_aura.h"

#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/browser/renderer_host/render_widget_host_view_aura.h"
#include "content/common/view_messages.h"
#include "ui/aura/window.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/aura/window_tree_host.h"
#include "ui/compositor/layer.h"

namespace content {

DelegatedFrameHostClientAura::DelegatedFrameHostClientAura(
    RenderWidgetHostViewAura* render_widget_host_view)
    : render_widget_host_view_(render_widget_host_view) {}

DelegatedFrameHostClientAura::~DelegatedFrameHostClientAura() {}

ui::Layer* DelegatedFrameHostClientAura::DelegatedFrameHostGetLayer() const {
  return render_widget_host_view_->window_->layer();
}

bool DelegatedFrameHostClientAura::DelegatedFrameHostIsVisible() const {
  return !render_widget_host_view_->host_->is_hidden();
}

SkColor DelegatedFrameHostClientAura::DelegatedFrameHostGetGutterColor() const {
  // When making an element on the page fullscreen the element's background
  // may not match the page's, so use black as the gutter color to avoid
  // flashes of brighter colors during the transition.
  if (render_widget_host_view_->host_->delegate() &&
      render_widget_host_view_->host_->delegate()
          ->IsFullscreenForCurrentTab()) {
    return SK_ColorBLACK;
  }
  if (render_widget_host_view_->GetBackgroundColor())
    return *render_widget_host_view_->GetBackgroundColor();
  return SK_ColorWHITE;
}

void DelegatedFrameHostClientAura::OnFirstSurfaceActivation(
    const viz::SurfaceInfo& surface_info) {}

void DelegatedFrameHostClientAura::OnBeginFrame(base::TimeTicks frame_time) {
  render_widget_host_view_->OnBeginFrame(frame_time);
}

void DelegatedFrameHostClientAura::OnFrameTokenChanged(uint32_t frame_token) {
  render_widget_host_view_->OnFrameTokenChangedForView(frame_token);
}

void DelegatedFrameHostClientAura::DidReceiveFirstFrameAfterNavigation() {
  render_widget_host_view_->host_->DidReceiveFirstFrameAfterNavigation();
}

}  // namespace content
