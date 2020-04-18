// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/graphics/touch_action_rect.h"

#include "base/containers/flat_map.h"
#include "cc/base/region.h"
#include "cc/layers/touch_action_region.h"

namespace blink {

// static
cc::TouchActionRegion TouchActionRect::BuildRegion(
    const Vector<TouchActionRect>& touch_action_rects) {
  base::flat_map<TouchAction, cc::Region> region_map;
  region_map.reserve(touch_action_rects.size());
  for (const TouchActionRect& touch_action_rect : touch_action_rects) {
    TouchAction action = touch_action_rect.whitelisted_touch_action;
    const LayoutRect& rect = touch_action_rect.rect;
    region_map[action].Union(EnclosingIntRect(rect));
  }
  return cc::TouchActionRegion(std::move(region_map));
}

}  // namespace blink
