// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_TOUCH_ACTION_RECT_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_TOUCH_ACTION_RECT_H_

#include "third_party/blink/renderer/platform/geometry/layout_rect.h"
#include "third_party/blink/renderer/platform/graphics/touch_action.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/hash_map.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace cc {
class TouchActionRegion;
}

namespace blink {

class PaintLayer;

struct PLATFORM_EXPORT TouchActionRect {
  LayoutRect rect;
  TouchAction whitelisted_touch_action;

  TouchActionRect(const LayoutRect& layout_rect, TouchAction action)
      : rect(layout_rect), whitelisted_touch_action(action) {}

  static cc::TouchActionRegion BuildRegion(const Vector<TouchActionRect>&);

  bool operator==(const TouchActionRect& rhs) const {
    return rect == rhs.rect &&
           whitelisted_touch_action == rhs.whitelisted_touch_action;
  }

  bool operator!=(const TouchActionRect& rhs) const { return !(*this == rhs); }
};

using LayerHitTestRects =
    WTF::HashMap<const PaintLayer*, Vector<TouchActionRect>>;

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_TOUCH_ACTION_RECT_H_
