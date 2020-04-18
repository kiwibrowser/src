// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_HIT_TEST_DATA_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_HIT_TEST_DATA_H_

#include "third_party/blink/renderer/platform/geometry/float_rect.h"
#include "third_party/blink/renderer/platform/geometry/region.h"
#include "third_party/blink/renderer/platform/graphics/touch_action_rect.h"
#include "third_party/blink/renderer/platform/platform_export.h"

namespace blink {

struct PLATFORM_EXPORT HitTestData {
  FloatRect border_rect;
  TouchActionRect touch_action_rect =
      TouchActionRect(LayoutRect(), cc::kTouchActionNone);
  Region wheel_event_handler_region;
  Region non_fast_scrollable_region;

  HitTestData() = default;
  HitTestData(const HitTestData& other)
      : border_rect(other.border_rect),
        touch_action_rect(other.touch_action_rect),
        wheel_event_handler_region(other.wheel_event_handler_region),
        non_fast_scrollable_region(other.non_fast_scrollable_region) {}

  bool operator==(const HitTestData& rhs) const {
    return border_rect == rhs.border_rect &&
           touch_action_rect == rhs.touch_action_rect &&
           wheel_event_handler_region == rhs.wheel_event_handler_region &&
           non_fast_scrollable_region == rhs.non_fast_scrollable_region;
  }

  bool operator!=(const HitTestData& rhs) const { return !(*this == rhs); }
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_HIT_TEST_DATA_H_
