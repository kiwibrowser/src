// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/public/platform/web_surface_layer_bridge.h"

#include <memory>
#include "third_party/blink/renderer/platform/graphics/surface_layer_bridge.h"

namespace blink {

std::unique_ptr<WebSurfaceLayerBridge> WebSurfaceLayerBridge::Create(
    WebLayerTreeView* layer_tree_view,
    WebSurfaceLayerBridgeObserver* observer) {
  return std::make_unique<SurfaceLayerBridge>(layer_tree_view, observer);
}

WebSurfaceLayerBridge::~WebSurfaceLayerBridge() = default;

}  // namespace blink
