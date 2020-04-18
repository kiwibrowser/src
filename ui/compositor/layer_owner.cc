// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/compositor/layer_owner.h"

#include <utility>


namespace ui {

LayerOwner::LayerOwner() : layer_(NULL) {}

LayerOwner::~LayerOwner() {
}

void LayerOwner::SetLayer(std::unique_ptr<Layer> layer) {
  DCHECK(!OwnsLayer());
  layer_owner_ = std::move(layer);
  layer_ = layer_owner_.get();
  layer_->owner_ = this;
}

std::unique_ptr<Layer> LayerOwner::AcquireLayer() {
  if (layer_owner_)
    layer_owner_->owner_ = NULL;
  return std::move(layer_owner_);
}

std::unique_ptr<Layer> LayerOwner::RecreateLayer() {
  std::unique_ptr<ui::Layer> old_layer(AcquireLayer());
  if (!old_layer)
    return old_layer;

  LayerDelegate* old_delegate = old_layer->delegate();
  old_layer->set_delegate(NULL);

  SetLayer(old_layer->Clone());

  if (old_layer->parent()) {
    // Install new layer as a sibling of the old layer, stacked below it.
    old_layer->parent()->Add(layer_);
    old_layer->parent()->StackBelow(layer_, old_layer.get());
  } else if (old_layer->GetCompositor()) {
    // If old_layer was the layer tree root then we need to move the Compositor
    // over to the new root.
    old_layer->GetCompositor()->SetRootLayer(layer_);
  }

  // Migrate all the child layers over to the new layer. Copy the list because
  // the items are removed during iteration.
  std::vector<ui::Layer*> children_copy = old_layer->children();
  for (std::vector<ui::Layer*>::const_iterator it = children_copy.begin();
       it != children_copy.end();
       ++it) {
    ui::Layer* child = *it;
    layer_->Add(child);
  }

  // Install the delegate last so that the delegate isn't notified as we copy
  // state to the new layer.
  layer_->set_delegate(old_delegate);

  return old_layer;
}

void LayerOwner::DestroyLayer() {
  layer_ = NULL;
  layer_owner_.reset();
}

bool LayerOwner::OwnsLayer() const {
  return !!layer_owner_;
}

}  // namespace ui
