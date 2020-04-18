// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_GAMEPAD_DUALSHOCK4_CONTROLLER_LINUX_
#define DEVICE_GAMEPAD_DUALSHOCK4_CONTROLLER_LINUX_

#include "device/gamepad/dualshock4_controller_base.h"

namespace device {

class Dualshock4ControllerLinux : public Dualshock4ControllerBase {
 public:
  Dualshock4ControllerLinux(int fd);
  ~Dualshock4ControllerLinux() override;

  size_t WriteOutputReport(void* report, size_t report_length) override;

 private:
  int fd_;
};

}  // namespace device

#endif  // DEVICE_GAMEPAD_DUALSHOCK4_CONTROLLER_LINUX_
