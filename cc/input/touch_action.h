// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_INPUT_TOUCH_ACTION_H_
#define CC_INPUT_TOUCH_ACTION_H_

#include <cstdlib>
#include <string>

#include "base/logging.h"

namespace cc {

// The current touch action specifies what accelerated browser operations
// (panning and zooming) are currently permitted via touch input.
// See http://www.w3.org/TR/pointerevents/#the-touch-action-css-property.
// This is intended to be the single canonical definition of the enum, it's used
// elsewhere in both Blink and content since touch action logic spans those
// subsystems.
// TODO(crbug.com/720553): rework this enum to enum class.
const size_t kTouchActionBits = 6;

enum TouchAction {
  // No scrolling or zooming allowed.
  kTouchActionNone = 0x0,
  kTouchActionPanLeft = 0x1,
  kTouchActionPanRight = 0x2,
  kTouchActionPanX = kTouchActionPanLeft | kTouchActionPanRight,
  kTouchActionPanUp = 0x4,
  kTouchActionPanDown = 0x8,
  kTouchActionPanY = kTouchActionPanUp | kTouchActionPanDown,
  kTouchActionPan = kTouchActionPanX | kTouchActionPanY,
  kTouchActionPinchZoom = 0x10,
  kTouchActionManipulation = kTouchActionPan | kTouchActionPinchZoom,
  kTouchActionDoubleTapZoom = 0x20,
  kTouchActionAuto = kTouchActionManipulation | kTouchActionDoubleTapZoom,
  kTouchActionMax = (1 << 6) - 1
};

inline TouchAction operator|(TouchAction a, TouchAction b) {
  return static_cast<TouchAction>(int(a) | int(b));
}

inline TouchAction& operator|=(TouchAction& a, TouchAction b) {
  return a = a | b;
}

inline TouchAction operator&(TouchAction a, TouchAction b) {
  return static_cast<TouchAction>(int(a) & int(b));
}

inline TouchAction& operator&=(TouchAction& a, TouchAction b) {
  return a = a & b;
}

inline const char* TouchActionToString(TouchAction touch_action) {
  switch (static_cast<int>(touch_action)) {
    case 0:
      return "NONE";
    case 1:
      return "PAN_LEFT";
    case 2:
      return "PAN_RIGHT";
    case 3:
      return "PAN_X";
    case 4:
      return "PAN_UP";
    case 5:
      return "PAN_LEFT_PAN_UP";
    case 6:
      return "PAN_RIGHT_PAN_UP";
    case 7:
      return "PAN_X_PAN_UP";
    case 8:
      return "PAN_DOWN";
    case 9:
      return "PAN_LEFT_PAN_DOWN";
    case 10:
      return "PAN_RIGHT_PAN_DOWN";
    case 11:
      return "PAN_X_PAN_DOWN";
    case 12:
      return "PAN_Y";
    case 13:
      return "PAN_LEFT_PAN_Y";
    case 14:
      return "PAN_RIGHT_PAN_Y";
    case 15:
      return "PAN_X_PAN_Y";
    case 16:
      return "PINCH_ZOOM";
    case 17:
      return "PAN_LEFT_PINCH_ZOOM";
    case 18:
      return "PAN_RIGHT_PINCH_ZOOM";
    case 19:
      return "PAN_X_PINCH_ZOOM";
    case 20:
      return "PAN_UP_PINCH_ZOOM";
    case 21:
      return "PAN_LEFT_PAN_UP_PINCH_ZOOM";
    case 22:
      return "PAN_RIGHT_PAN_UP_PINCH_ZOOM";
    case 23:
      return "PAN_X_PAN_UP_PINCH_ZOOM";
    case 24:
      return "PAN_DOWN_PINCH_ZOOM";
    case 25:
      return "PAN_LEFT_PAN_DOWN_PINCH_ZOOM";
    case 26:
      return "PAN_RIGHT_PAN_DOWN_PINCH_ZOOM";
    case 27:
      return "PAN_X_PAN_DOWN_PINCH_ZOOM";
    case 28:
      return "PAN_Y_PINCH_ZOOM";
    case 29:
      return "PAN_LEFT_PAN_Y_PINCH_ZOOM";
    case 30:
      return "PAN_RIGHT_PAN_Y_PINCH_ZOOM";
    case 31:
      return "MANIPULATION";
    case 32:
      return "DOUBLE_TAP_ZOOM";
    case 33:
      return "PAN_LEFT_DOUBLE_TAP_ZOOM";
    case 34:
      return "PAN_RIGHT_DOUBLE_TAP_ZOOM";
    case 35:
      return "PAN_X_DOUBLE_TAP_ZOOM";
    case 36:
      return "PAN_UP_DOUBLE_TAP_ZOOM";
    case 37:
      return "PAN_LEFT_PAN_UP_DOUBLE_TAP_ZOOM";
    case 38:
      return "PAN_RIGHT_PAN_UP_DOUBLE_TAP_ZOOM";
    case 39:
      return "PAN_X_PAN_UP_DOUBLE_TAP_ZOOM";
    case 40:
      return "PAN_DOWN_DOUBLE_TAP_ZOOM";
    case 41:
      return "PAN_LEFT_PAN_DOWN_DOUBLE_TAP_ZOOM";
    case 42:
      return "PAN_RIGHT_PAN_DOWN_DOUBLE_TAP_ZOOM";
    case 43:
      return "PAN_X_PAN_DOWN_DOUBLE_TAP_ZOOM";
    case 44:
      return "PAN_Y_DOUBLE_TAP_ZOOM";
    case 45:
      return "PAN_LEFT_PAN_Y_DOUBLE_TAP_ZOOM";
    case 46:
      return "PAN_RIGHT_PAN_Y_DOUBLE_TAP_ZOOM";
    case 47:
      return "PAN_X_PAN_Y_DOUBLE_TAP_ZOOM";
    case 48:
      return "PINCH_ZOOM_DOUBLE_TAP_ZOOM";
    case 49:
      return "PAN_LEFT_PINCH_ZOOM_DOUBLE_TAP_ZOOM";
    case 50:
      return "PAN_RIGHT_PINCH_ZOOM_DOUBLE_TAP_ZOOM";
    case 51:
      return "PAN_X_PINCH_ZOOM_DOUBLE_TAP_ZOOM";
    case 52:
      return "PAN_UP_PINCH_ZOOM_DOUBLE_TAP_ZOOM";
    case 53:
      return "PAN_LEFT_PAN_UP_PINCH_ZOOM_DOUBLE_TAP_ZOOM";
    case 54:
      return "PAN_RIGHT_PAN_UP_PINCH_ZOOM_DOUBLE_TAP_ZOOM";
    case 55:
      return "PAN_X_PAN_UP_PINCH_ZOOM_DOUBLE_TAP_ZOOM";
    case 56:
      return "PAN_DOWN_PINCH_ZOOM_DOUBLE_TAP_ZOOM";
    case 57:
      return "PAN_LEFT_PAN_DOWN_PINCH_ZOOM_DOUBLE_TAP_ZOOM";
    case 58:
      return "PAN_RIGHT_PAN_DOWN_PINCH_ZOOM_DOUBLE_TAP_ZOOM";
    case 59:
      return "PAN_X_PAN_DOWN_PINCH_ZOOM_DOUBLE_TAP_ZOOM";
    case 60:
      return "PAN_Y_PINCH_ZOOM_DOUBLE_TAP_ZOOM";
    case 61:
      return "PAN_LEFT_PAN_Y_PINCH_ZOOM_DOUBLE_TAP_ZOOM";
    case 62:
      return "PAN_RIGHT_PAN_Y_PINCH_ZOOM_DOUBLE_TAP_ZOOM";
    case 63:
      return "AUTO";
  }
  NOTREACHED();
  return "";
}

}  // namespace cc

#endif  // CC_INPUT_TOUCH_ACTION_H_
