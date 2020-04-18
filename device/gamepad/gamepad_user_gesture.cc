// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/gamepad/gamepad_user_gesture.h"

#include <math.h>

#include <algorithm>

#include "device/gamepad/public/cpp/gamepads.h"

namespace {
// A big enough deadzone to detect accidental presses.
const float kAxisMoveAmountThreshold = 0.5;
}

namespace device {

bool GamepadsHaveUserGesture(const Gamepads& gamepads) {
  for (unsigned int i = 0; i < Gamepads::kItemsLengthCap; i++) {
    const Gamepad& pad = gamepads.items[i];

    // If the device is physically connected, then check the buttons and axes
    // to see if there is currently an intentional user action.
    if (pad.connected) {
      for (unsigned int button_index = 0; button_index < pad.buttons_length;
           button_index++) {
        if (pad.buttons[button_index].pressed)
          return true;
      }

      for (unsigned int axes_index = 0; axes_index < pad.axes_length;
           axes_index++) {
        if (fabs(pad.axes[axes_index]) > kAxisMoveAmountThreshold)
          return true;
      }
    }
  }
  return false;
}

}  // namespace device
