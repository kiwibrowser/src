// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_STUB_CHROME_CLIENT_FOR_SPV2_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_STUB_CHROME_CLIENT_FOR_SPV2_H_

#include "third_party/blink/renderer/core/loader/empty_clients.h"
#include "third_party/blink/renderer/platform/testing/web_layer_tree_view_impl_for_testing.h"

namespace cc {
class Layer;
}

namespace blink {

// A simple ChromeClient implementation which forwards painted artifacts to a
// PaintArtifactCompositor attached to a testing WebLayerTreeView, and permits
// simple analysis of the results.
class StubChromeClientForSPv2 : public EmptyChromeClient {
 public:
  StubChromeClientForSPv2() : layer_tree_view_() {}

  bool HasLayer(const cc::Layer& layer) {
    return layer_tree_view_.HasLayer(layer);
  }

  void AttachRootLayer(scoped_refptr<cc::Layer> layer,
                       LocalFrame* local_root) override {
    layer_tree_view_.SetRootLayer(std::move(layer));
  }

 private:
  WebLayerTreeViewImplForTesting layer_tree_view_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_STUB_CHROME_CLIENT_FOR_SPV2_H_
