// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/webscrollbarbehavior_impl_mac.h"

namespace content {

WebScrollbarBehaviorImpl::WebScrollbarBehaviorImpl()
    : jump_on_track_click_(false) {
}

bool WebScrollbarBehaviorImpl::ShouldCenterOnThumb(
    blink::WebPointerProperties::Button mouseButton,
    bool shiftKeyPressed,
    bool altKeyPressed) {
  return (mouseButton == blink::WebPointerProperties::Button::kLeft) &&
         (jump_on_track_click_ != altKeyPressed);
}

}  // namespace content
