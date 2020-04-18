// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/gamepad/dualshock4_controller_linux.h"

#include "base/posix/eintr_wrapper.h"

namespace device {

Dualshock4ControllerLinux::Dualshock4ControllerLinux(int fd) : fd_(fd) {}

Dualshock4ControllerLinux::~Dualshock4ControllerLinux() = default;

size_t Dualshock4ControllerLinux::WriteOutputReport(void* report,
                                                    size_t report_length) {
  ssize_t bytes_written = HANDLE_EINTR(write(fd_, report, report_length));
  return bytes_written < 0 ? 0 : static_cast<size_t>(bytes_written);
}

}  // namespace device
