// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_GAMEPAD_XINPUT_HAPTIC_GAMEPAD_WIN_
#define DEVICE_GAMEPAD_XINPUT_HAPTIC_GAMEPAD_WIN_

#include <Unknwn.h>
#include <XInput.h>

#include "device/gamepad/abstract_haptic_gamepad.h"

namespace device {

class XInputHapticGamepadWin : public AbstractHapticGamepad {
 public:
  typedef DWORD(WINAPI* XInputSetStateFunc)(DWORD dwUserIndex,
                                            XINPUT_VIBRATION* pVibration);

  XInputHapticGamepadWin(int pad_id, XInputSetStateFunc xinput_set_state);
  ~XInputHapticGamepadWin() override;

  void SetVibration(double strong_magnitude, double weak_magnitude) override;

 private:
  int pad_id_;
  XInputSetStateFunc xinput_set_state_;
};

}  // namespace device

#endif  // DEVICE_GAMEPAD_EVDEV_HAPTIC_GAMEPAD_WIN_
