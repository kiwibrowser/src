// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/overview/overview_window_animation_observer.h"

#include "ui/compositor/layer.h"

namespace ash {

OverviewWindowAnimationObserver::OverviewWindowAnimationObserver()
    : weak_ptr_factory_(this) {}

OverviewWindowAnimationObserver::~OverviewWindowAnimationObserver() = default;

void OverviewWindowAnimationObserver::OnImplicitAnimationsCompleted() {
  for (const auto& layer_transform : layer_transform_map_) {
    ui::Layer* layer = layer_transform.first;
    layer->SetTransform(layer_transform.second);
    layer->RemoveObserver(this);
  }
  layer_transform_map_.clear();
  delete this;
}

void OverviewWindowAnimationObserver::LayerDestroyed(ui::Layer* layer) {
  auto iter = layer_transform_map_.find(layer);
  DCHECK(iter != layer_transform_map_.end());
  layer_transform_map_.erase(iter);
  layer->RemoveObserver(this);
}

void OverviewWindowAnimationObserver::AddLayerTransformPair(
    ui::Layer* layer,
    const gfx::Transform& transform) {
  // Should not have the same |layer|.
  DCHECK(layer_transform_map_.find(layer) == layer_transform_map_.end());
  layer_transform_map_.emplace(std::make_pair(layer, transform));
  layer->AddObserver(this);
}

base::WeakPtr<OverviewWindowAnimationObserver>
OverviewWindowAnimationObserver::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

}  // namespace ash
