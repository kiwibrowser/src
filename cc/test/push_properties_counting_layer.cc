// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/test/push_properties_counting_layer.h"

#include "cc/test/push_properties_counting_layer_impl.h"
#include "cc/trees/layer_tree_host.h"

namespace cc {

// static
scoped_refptr<PushPropertiesCountingLayer>
PushPropertiesCountingLayer::Create() {
  return new PushPropertiesCountingLayer();
}

PushPropertiesCountingLayer::PushPropertiesCountingLayer()
    : push_properties_count_(0), persist_needs_push_properties_(false) {
  SetBounds(gfx::Size(1, 1));
}

PushPropertiesCountingLayer::~PushPropertiesCountingLayer() = default;

void PushPropertiesCountingLayer::PushPropertiesTo(LayerImpl* layer) {
  Layer::PushPropertiesTo(layer);
  AddPushPropertiesCount();
}

std::unique_ptr<LayerImpl> PushPropertiesCountingLayer::CreateLayerImpl(
    LayerTreeImpl* tree_impl) {
  return PushPropertiesCountingLayerImpl::Create(tree_impl, Layer::id());
}

void PushPropertiesCountingLayer::MakePushProperties() {
  SetContentsOpaque(!contents_opaque());
}

void PushPropertiesCountingLayer::AddPushPropertiesCount() {
  push_properties_count_++;
  if (persist_needs_push_properties_) {
    layer_tree_host()->AddLayerShouldPushProperties(this);
  }
}

}  // namespace cc
