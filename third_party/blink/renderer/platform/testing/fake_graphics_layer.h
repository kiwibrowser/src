// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_TESTING_FAKE_GRAPHICS_LAYER_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_TESTING_FAKE_GRAPHICS_LAYER_H_

namespace blink {

// A simple GraphicsLayer implementation suitable for use in unit tests.
class FakeGraphicsLayer : public GraphicsLayer {
 public:
  explicit FakeGraphicsLayer(GraphicsLayerClient& client)
      : GraphicsLayer(client) {}
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_TESTING_FAKE_GRAPHICS_LAYER_H_
