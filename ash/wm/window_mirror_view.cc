// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/window_mirror_view.h"

#include "ash/wm/widget_finder.h"
#include "ash/wm/window_state.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/window.h"
#include "ui/compositor/layer.h"
#include "ui/compositor/layer_tree_owner.h"
#include "ui/views/widget/widget.h"
#include "ui/wm/core/window_util.h"

namespace ash {
namespace wm {
namespace {

void EnsureAllChildrenAreVisible(ui::Layer* layer) {
  std::list<ui::Layer*> layers;
  layers.push_back(layer);
  while (!layers.empty()) {
    for (auto* child : layers.front()->children())
      layers.push_back(child);
    layers.front()->SetVisible(true);
    layers.pop_front();
  }
}

}  // namespace

WindowMirrorView::WindowMirrorView(aura::Window* window,
                                   bool trilinear_filtering_on_init)
    : target_(window),
      trilinear_filtering_on_init_(trilinear_filtering_on_init) {
  DCHECK(window);
}

WindowMirrorView::~WindowMirrorView() {
  // Make sure |target_| has outlived |this|. See crbug.com/681207
  DCHECK(target_->layer());
  if (layer_owner_)
    target_->ClearProperty(aura::client::kMirroringEnabledKey);
}

gfx::Size WindowMirrorView::CalculatePreferredSize() const {
  return GetClientAreaBounds().size();
}

void WindowMirrorView::Layout() {
  // If |layer_owner_| hasn't been initialized (|this| isn't on screen), no-op.
  if (!layer_owner_)
    return;

  // Position at 0, 0.
  GetMirrorLayer()->SetBounds(gfx::Rect(GetMirrorLayer()->bounds().size()));

  gfx::Transform transform;
  gfx::Rect client_area_bounds = GetClientAreaBounds();
  // Scale down if necessary.
  if (size() != target_->bounds().size()) {
    const float scale =
        width() / static_cast<float>(client_area_bounds.width());
    transform.Scale(scale, scale);
  }
  // Reposition such that the client area is the only part visible.
  transform.Translate(-client_area_bounds.x(), -client_area_bounds.y());
  GetMirrorLayer()->SetTransform(transform);
}

bool WindowMirrorView::GetNeedsNotificationWhenVisibleBoundsChange() const {
  return true;
}

void WindowMirrorView::OnVisibleBoundsChanged() {
  if (!layer_owner_ && !GetVisibleBounds().IsEmpty())
    InitLayerOwner();
}

void WindowMirrorView::InitLayerOwner() {
  if (!layer_owner_)
    target_->SetProperty(aura::client::kMirroringEnabledKey, true);

  layer_owner_ = ::wm::MirrorLayers(target_, false /* sync_bounds */);

  SetPaintToLayer();

  ui::Layer* mirror_layer = GetMirrorLayer();
  layer()->Add(mirror_layer);
  // This causes us to clip the non-client areas of the window.
  layer()->SetMasksToBounds(true);

  // Some extra work is needed when the target window is minimized.
  if (wm::GetWindowState(target_)->IsMinimized()) {
    mirror_layer->SetOpacity(1);
    EnsureAllChildrenAreVisible(mirror_layer);
  }

  if (trilinear_filtering_on_init_) {
    mirror_layer->AddCacheRenderSurfaceRequest();
    mirror_layer->AddTrilinearFilteringRequest();
  }

  Layout();
}

ui::Layer* WindowMirrorView::GetMirrorLayer() {
  return layer_owner_->root();
}

gfx::Rect WindowMirrorView::GetClientAreaBounds() const {
  int inset = target_->GetProperty(aura::client::kTopViewInset);
  if (inset > 0) {
    gfx::Rect bounds(target_->bounds().size());
    bounds.Inset(0, inset, 0, 0);
    return bounds;
  }
  // The target window may not have a widget in unit tests.
  views::Widget* widget = GetInternalWidgetForWindow(target_);
  if (!widget)
    return gfx::Rect();
  views::View* client_view = widget->client_view();
  return client_view->ConvertRectToWidget(client_view->GetLocalBounds());
}

}  // namespace wm
}  // namespace ash
