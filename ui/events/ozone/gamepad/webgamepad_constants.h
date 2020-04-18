// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_EVENTS_OZONE_GAMEPAD_WEBGAMEPAD_CONSTANTS_H_
#define UI_EVENTS_OZONE_GAMEPAD_WEBGAMEPAD_CONSTANTS_H_

#include <stdint.h>

namespace ui {

// We care about three type of gamepad events.
enum class GamepadEventType { BUTTON, AXIS, FRAME };

// This WebGamepadButtonType matches the index of gamepad button defined in w3c
// standard gamepad.
enum WebGamepadButtonType {
  WG_BUTTON_A = 0,
  WG_BUTTON_B = 1,
  WG_BUTTON_X = 2,
  WG_BUTTON_Y = 3,
  WG_BUTTON_L1 = 4,
  WG_BUTTON_R1 = 5,
  WG_BUTTON_LT = 6,
  WG_BUTTON_RT = 7,
  WG_BUTTON_SELECT = 8,
  WG_BUTTON_START = 9,
  WG_BUTTON_THUMBL = 10,
  WG_BUTTON_THUMBR = 11,
  WG_BUTTON_DPAD_UP = 12,
  WG_BUTTON_DPAD_DOWN = 13,
  WG_BUTTON_DPAD_LEFT = 14,
  WG_BUTTON_DPAD_RIGHT = 15,
  WG_BUTTON_MODE = 16,
  WG_BUTTON_COUNT = 17
};

// This WebGamepadAbsType matches the index of gamepad abs defined in w3c
// standard gamepad.
enum WebGamepadAbsType {
  WG_ABS_X = 0,
  WG_ABS_Y = 1,
  WG_ABS_RX = 2,
  WG_ABS_RY = 3,
  WG_ABS_COUNT = 4
};

// Following constants are used to normalize abs values to web gamepad standard.
constexpr double kWebGamepadTriggerMin = 0.0;
constexpr double kWebGamepadTriggerMax = 1.0;
constexpr double kWebGamepadJoystickMin = -1.0;
constexpr double kWebGamepadJoystickMax = 1.0;

}  // namespace ui

#endif  // UI_EVENTS_OZONE_EVDEV_WEBGAMEPAD_CONSTANTS_H_
