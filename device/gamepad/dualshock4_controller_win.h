// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_GAMEPAD_DUALSHOCK4_CONTROLLER_WIN_
#define DEVICE_GAMEPAD_DUALSHOCK4_CONTROLLER_WIN_

#include "base/win/scoped_handle.h"
#include "device/gamepad/dualshock4_controller_base.h"

namespace device {

class Dualshock4ControllerWin : public Dualshock4ControllerBase {
 public:
  explicit Dualshock4ControllerWin(HANDLE device_handle);
  ~Dualshock4ControllerWin() override;

  // Close the HID handle.
  void DoShutdown() override;

  size_t WriteOutputReport(void* report, size_t report_length) override;

 private:
  base::win::ScopedHandle hid_handle_;
};

}  // namespace device

#endif  // DEVICE_GAMEPAD_DUALSHOCK4_CONTROLLER_WIN_
