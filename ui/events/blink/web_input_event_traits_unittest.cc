// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/events/blink/web_input_event_traits.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/web_gesture_event.h"
#include "third_party/blink/public/platform/web_input_event.h"
#include "third_party/blink/public/platform/web_keyboard_event.h"
#include "third_party/blink/public/platform/web_mouse_wheel_event.h"
#include "third_party/blink/public/platform/web_touch_event.h"

using blink::WebGestureEvent;
using blink::WebInputEvent;
using blink::WebKeyboardEvent;
using blink::WebMouseEvent;
using blink::WebMouseWheelEvent;
using blink::WebTouchEvent;

namespace ui {
namespace {

using WebInputEventTraitsTest = testing::Test;

// Very basic smoke test to ensure stringification doesn't explode.
TEST_F(WebInputEventTraitsTest, ToString) {
  WebKeyboardEvent key(WebInputEvent::kRawKeyDown, WebInputEvent::kNoModifiers,
                       WebInputEvent::GetStaticTimeStampForTests());
  EXPECT_FALSE(WebInputEventTraits::ToString(key).empty());

  WebMouseEvent mouse(WebInputEvent::kMouseMove, WebInputEvent::kNoModifiers,
                      WebInputEvent::GetStaticTimeStampForTests());
  EXPECT_FALSE(WebInputEventTraits::ToString(mouse).empty());

  WebMouseWheelEvent mouse_wheel(WebInputEvent::kMouseWheel,
                                 WebInputEvent::kNoModifiers,
                                 WebInputEvent::GetStaticTimeStampForTests());
  EXPECT_FALSE(WebInputEventTraits::ToString(mouse_wheel).empty());

  WebGestureEvent gesture(WebInputEvent::kGesturePinchBegin,
                          WebInputEvent::kNoModifiers,
                          WebInputEvent::GetStaticTimeStampForTests());
  gesture.SetPositionInWidget(gfx::PointF(1, 1));
  EXPECT_FALSE(WebInputEventTraits::ToString(gesture).empty());

  WebTouchEvent touch(WebInputEvent::kTouchStart, WebInputEvent::kNoModifiers,
                      WebInputEvent::GetStaticTimeStampForTests());
  touch.touches_length = 1;
  EXPECT_FALSE(WebInputEventTraits::ToString(touch).empty());
}

}  // namespace
}  // namespace ui
