// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/gamepad/dualshock4_controller_mac.h"

#include <CoreFoundation/CoreFoundation.h>

namespace device {

Dualshock4ControllerMac::Dualshock4ControllerMac(IOHIDDeviceRef device_ref)
    : device_ref_(device_ref) {}

Dualshock4ControllerMac::~Dualshock4ControllerMac() = default;

void Dualshock4ControllerMac::DoShutdown() {
  device_ref_ = nullptr;
}

size_t Dualshock4ControllerMac::WriteOutputReport(void* report,
                                                  size_t report_length) {
  if (!device_ref_)
    return 0;

  const unsigned char* report_data = static_cast<unsigned char*>(report);
  IOReturn success =
      IOHIDDeviceSetReport(device_ref_, kIOHIDReportTypeOutput, report_data[0],
                           report_data, report_length);
  return (success == kIOReturnSuccess) ? report_length : 0;
}

}  // namespace device
