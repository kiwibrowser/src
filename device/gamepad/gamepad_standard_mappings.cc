// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/gamepad/gamepad_standard_mappings.h"

namespace device {

GamepadButton AxisToButton(float input) {
  float value = (input + 1.f) / 2.f;
  bool pressed = value > kDefaultButtonPressedThreshold;
  bool touched = value > 0.0f;
  return GamepadButton(pressed, touched, value);
}

GamepadButton AxisNegativeAsButton(float input) {
  float value = (input < -0.5f) ? 1.f : 0.f;
  bool pressed = value > kDefaultButtonPressedThreshold;
  bool touched = value > 0.0f;
  return GamepadButton(pressed, touched, value);
}

GamepadButton AxisPositiveAsButton(float input) {
  float value = (input > 0.5f) ? 1.f : 0.f;
  bool pressed = value > kDefaultButtonPressedThreshold;
  bool touched = value > 0.0f;
  return GamepadButton(pressed, touched, value);
}

GamepadButton ButtonFromButtonAndAxis(GamepadButton button, float axis) {
  float value = (axis + 1.f) / 2.f;
  return GamepadButton(button.pressed, button.touched, value);
}

GamepadButton NullButton() {
  return GamepadButton(false, false, 0.0);
}

void DpadFromAxis(Gamepad* mapped, float dir) {
  bool up = false;
  bool right = false;
  bool down = false;
  bool left = false;

  // Dpad is mapped as a direction on one axis, where -1 is up and it
  // increases clockwise to 1, which is up + left. It's set to a large (> 1.f)
  // number when nothing is depressed, except on start up, sometimes it's 0.0
  // for no data, rather than the large number.
  if (dir != 0.0f) {
    up = (dir >= -1.f && dir < -0.7f) || (dir >= .95f && dir <= 1.f);
    right = dir >= -.75f && dir < -.1f;
    down = dir >= -.2f && dir < .45f;
    left = dir >= .4f && dir <= 1.f;
  }

  mapped->buttons[BUTTON_INDEX_DPAD_UP].pressed = up;
  mapped->buttons[BUTTON_INDEX_DPAD_UP].touched = up;
  mapped->buttons[BUTTON_INDEX_DPAD_UP].value = up ? 1.f : 0.f;
  mapped->buttons[BUTTON_INDEX_DPAD_RIGHT].pressed = right;
  mapped->buttons[BUTTON_INDEX_DPAD_RIGHT].touched = right;
  mapped->buttons[BUTTON_INDEX_DPAD_RIGHT].value = right ? 1.f : 0.f;
  mapped->buttons[BUTTON_INDEX_DPAD_DOWN].pressed = down;
  mapped->buttons[BUTTON_INDEX_DPAD_DOWN].touched = down;
  mapped->buttons[BUTTON_INDEX_DPAD_DOWN].value = down ? 1.f : 0.f;
  mapped->buttons[BUTTON_INDEX_DPAD_LEFT].pressed = left;
  mapped->buttons[BUTTON_INDEX_DPAD_LEFT].touched = left;
  mapped->buttons[BUTTON_INDEX_DPAD_LEFT].value = left ? 1.f : 0.f;
}

float RenormalizeAndClampAxis(float value, float min, float max) {
  value = (2.f * (value - min) / (max - min)) - 1.f;
  return value < -1.f ? -1.f : (value > 1.f ? 1.f : value);
}

}  // namespace device
