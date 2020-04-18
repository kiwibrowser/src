// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_GAMEPAD_DUALSHOCK4_CONTROLLER_MAC_H_
#define DEVICE_GAMEPAD_DUALSHOCK4_CONTROLLER_MAC_H_

#include "device/gamepad/dualshock4_controller_base.h"

#include <IOKit/hid/IOHIDManager.h>

namespace device {

class Dualshock4ControllerMac : public Dualshock4ControllerBase {
 public:
  Dualshock4ControllerMac(IOHIDDeviceRef device_ref);
  ~Dualshock4ControllerMac() override;

  void DoShutdown() override;

  size_t WriteOutputReport(void* report, size_t report_length) override;

 private:
  IOHIDDeviceRef device_ref_;
};

}  // namespace device

#endif  // DEVICE_GAMEPAD_DUALSHOCK4_CONTROLLER_MAC_H_
