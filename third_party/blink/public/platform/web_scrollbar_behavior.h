// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_SCROLLBAR_BEHAVIOR_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_SCROLLBAR_BEHAVIOR_H_

#include "third_party/blink/public/platform/web_pointer_properties.h"

namespace blink {

struct WebPoint;
struct WebRect;

class WebScrollbarBehavior {
 public:
  virtual ~WebScrollbarBehavior() = default;
  virtual bool ShouldCenterOnThumb(WebPointerProperties::Button,
                                   bool shift_key_pressed,
                                   bool alt_key_pressed) {
    return false;
  }
  virtual bool ShouldSnapBackToDragOrigin(const WebPoint& event_point,
                                          const WebRect& scrollbar_rect,
                                          bool is_horizontal) {
    return false;
  }
};

}  // namespace blink

#endif
