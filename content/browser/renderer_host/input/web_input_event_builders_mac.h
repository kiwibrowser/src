// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_INPUT_WEB_INPUT_EVENT_BUILDERS_MAC_H_
#define CONTENT_BROWSER_RENDERER_HOST_INPUT_WEB_INPUT_EVENT_BUILDERS_MAC_H_

#include "content/common/content_export.h"
#include "third_party/blink/public/platform/web_gesture_event.h"
#include "third_party/blink/public/platform/web_input_event.h"
#include "third_party/blink/public/platform/web_keyboard_event.h"
#include "third_party/blink/public/platform/web_mouse_wheel_event.h"

@class NSEvent;
@class NSView;

namespace content {

class CONTENT_EXPORT WebKeyboardEventBuilder {
 public:
  static blink::WebKeyboardEvent Build(NSEvent* event);
};

class CONTENT_EXPORT WebMouseEventBuilder {
 public:
  static blink::WebMouseEvent Build(
      NSEvent* event,
      NSView* view,
      blink::WebPointerProperties::PointerType pointerType =
          blink::WebPointerProperties::PointerType::kMouse);
};

class CONTENT_EXPORT WebMouseWheelEventBuilder {
 public:
  static blink::WebMouseWheelEvent Build(NSEvent* event,
                                         NSView* view);
};

class CONTENT_EXPORT WebGestureEventBuilder {
 public:
  static blink::WebGestureEvent Build(NSEvent*, NSView*);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_INPUT_WEB_INPUT_EVENT_BUILDERS_MAC_H_
