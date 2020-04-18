// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_LINK_HIGHLIGHT_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_LINK_HIGHLIGHT_H_

#include "third_party/blink/renderer/platform/platform_export.h"

namespace cc {
class Layer;
}

namespace blink {

class PLATFORM_EXPORT LinkHighlight {
 public:
  virtual void Invalidate() = 0;
  virtual void ClearCurrentGraphicsLayer() = 0;
  virtual cc::Layer* Layer() = 0;

 protected:
  virtual ~LinkHighlight() = default;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_LINK_HIGHLIGHT_H_
