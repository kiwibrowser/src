// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/input/touchscreen_tap_suppression_controller.h"

#include <utility>

#include "content/browser/renderer_host/input/gesture_event_queue.h"

using blink::WebInputEvent;

namespace content {

TouchscreenTapSuppressionController::TouchscreenTapSuppressionController(
    const TapSuppressionController::Config& config)
    : TapSuppressionController(config) {}

TouchscreenTapSuppressionController::~TouchscreenTapSuppressionController() {}

bool TouchscreenTapSuppressionController::FilterTapEvent(
    const GestureEventWithLatencyInfo& event) {
  switch (event.event.GetType()) {
    case WebInputEvent::kGestureTapDown:
      return ShouldSuppressTapDown();

    case WebInputEvent::kGestureShowPress:
    case WebInputEvent::kGestureLongPress:
    case WebInputEvent::kGestureTapUnconfirmed:
    case WebInputEvent::kGestureTapCancel:
    case WebInputEvent::kGestureTap:
    case WebInputEvent::kGestureDoubleTap:
    case WebInputEvent::kGestureLongTap:
    case WebInputEvent::kGestureTwoFingerTap:
      return ShouldSuppressTapEnd();

    default:
      break;
  }
  return false;
}

}  // namespace content
